#include "irods/rodsServer.hpp"

#include "irods/client_api_allowlist.hpp"
#include "irods/client_connection.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_buffer_encryption.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_configuration_parser.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/json_events.hpp"
#include "irods/sharedmemory.hpp"
#include "irods/initServer.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_server_state.hpp"
#include "irods/irods_client_server_negotiation.hpp"
#include "irods/irods_network_factory.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_server_control_plane.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/locks.hpp"
#include "irods/sockCommNetworkInterface.hpp"
#include "irods/irods_random.hpp"
#include "irods/replica_access_table.hpp"
#include "irods/irods_logger.hpp"
#include "irods/hostname_cache.hpp"
#include "irods/dns_cache.hpp"
#include "irods/server_utilities.hpp"
#include "irods/process_manager.hpp"
#include "irods/irods_default_paths.hpp"
#include "irods/irods_signal.hpp"
#include "irods/irods_server_api_table.hpp"
#include "irods/irods_client_api_table.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/rodsAgent.hpp"
#include "irods/physPath.hpp"

#include "irods/rcMisc.h"
#include "irods/rodsErrorTable.h"
#include "irods/getRodsEnv.h"
#include "irods/procLog.h"
#include "irods/plugins/api/grid_configuration_types.h"
#include "irods/get_grid_configuration_value.h"
#include "irods/set_delay_server_migration_info.h"

#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <zmq.hpp>
#include <avro/Encoder.hh>
#include <avro/Decoder.hh>
#include <avro/Specific.hh>

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>
#include <boost/stacktrace.hpp>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <regex>
#include <algorithm>
#include <optional>
#include <iterator>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>

// __has_feature is a Clang specific feature.
// The preprocessor code below exists so that other compilers can be used (e.g. GCC).
#ifndef __has_feature
#  define __has_feature(feature) 0
#endif

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#  include <sanitizer/lsan_interface.h>

// Defines default options for running iRODS with Address Sanitizer enabled.
// This is a convenience function which allows the iRODS server to start without
// having to specify options via environment variables.
extern "C" const char* __asan_default_options()
{
    // See root CMakeLists.txt file for definition.
    return IRODS_ADDRESS_SANITIZER_DEFAULT_OPTIONS;
} // __asan_default_options
#endif

#define LOCK_FILE_PURGE_TIME 7200 // Purge lock files every 2 hr.

// clang-format off
namespace ix   = irods::experimental;
// clang-format on

using log_server = irods::experimental::log::server;

struct sockaddr_un local_addr{};
int agent_conn_socket{};
bool connected_to_agent{};

pid_t agent_spawning_pid{};
char unix_domain_socket_directory[] = "/tmp/irods_sockets_XXXXXX";
char agent_factory_socket_file[sizeof(sockaddr_un::sun_path)]{};

unsigned int ServerBootTime;
int SvrSock;

std::atomic<bool> is_control_plane_accepting_requests = false;
int failed_delay_server_migration_communication_attempts = 0;

std::atomic<bool> reload_server_config = false;

agentProc_t* ConnectedAgentHead{};
agentProc_t* ConnReqHead{};
agentProc_t* SpawnReqHead{};
agentProc_t* BadReqHead{};

boost::mutex              ConnectedAgentMutex;
boost::mutex              BadReqMutex;
boost::thread*            ReadWorkerThread[NUM_READ_WORKER_THR];
boost::thread*            SpawnManagerThread;

boost::thread*            PurgeLockFileThread;

boost::mutex              ReadReqCondMutex;
boost::mutex              SpawnReqCondMutex;
boost::condition_variable ReadReqCond;
boost::condition_variable SpawnReqCond;

namespace
{
    // We incorporate the cache salt into the rule engine's named_mutex and shared memory object.
    // This prevents (most of the time) an orphaned mutex from halting server standup. Issue most often seen
    // when a running iRODS installation is uncleanly killed (leaving the file system object used to implement
    // boost::named_mutex e.g. in /var/run/shm) and then the iRODS user account is recreated, yielding a different
    // UID. The new iRODS user account is then unable to unlock or remove the existing mutex, blocking the server.
    irods::error createAndSetRECacheSalt()
    {
        // Should only ever set the cache salt once.
        try {
            const auto& existing_salt = irods::get_server_property<const std::string>(irods::KW_CFG_RE_CACHE_SALT);
            log_server::debug("createAndSetRECacheSalt: Cache salt already set [{}]", existing_salt.c_str());
            return ERROR(SYS_ALREADY_INITIALIZED, "createAndSetRECacheSalt: Cache salt already set");
        }
        catch (const irods::exception&) {
            irods::buffer_crypt::array_t buf;
            irods::error ret = irods::buffer_crypt::generate_key(buf, RE_CACHE_SALT_NUM_RANDOM_BYTES);
            if (!ret.ok()) {
                log_server::critical("createAndSetRECacheSalt: failed to generate random bytes");
                return PASS(ret);
            }

            std::string cache_salt_random;
            ret = irods::buffer_crypt::hex_encode(buf, cache_salt_random);
            if (!ret.ok()) {
                log_server::critical("createAndSetRECacheSalt: failed to hex encode random bytes");
                return PASS(ret);
            }

            const auto cache_salt = fmt::format("pid{}_{}", static_cast<std::intmax_t>(getpid()), cache_salt_random);

            try {
                irods::set_server_property<std::string>(irods::KW_CFG_RE_CACHE_SALT, cache_salt);
            }
            catch (const nlohmann::json::exception& e) {
                log_server::critical("createAndSetRECacheSalt: failed to set server_properties");
                return ERROR(SYS_INVALID_INPUT_PARAM, e.what());
            }
            catch (const std::exception&) {}

            if (setenv(SP_RE_CACHE_SALT, cache_salt.c_str(), 1) != 0) {
                log_server::critical("createAndSetRECacheSalt: failed to set environment variable");
                return ERROR(SYS_SETENV_ERR, "createAndSetRECacheSalt: failed to set environment variable");
            }

            return SUCCESS();
        }
    }

    void get64RandomBytes(char* buf)
    {
        const int num_random_bytes = 32;
        unsigned char random_bytes[num_random_bytes];
        irods::getRandomBytes(random_bytes, sizeof(random_bytes));

        std::stringstream ss;
        for (std::size_t i = 0; i < sizeof(random_bytes); ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) random_bytes[i];
        }

        const int num_hex_bytes = 2 * num_random_bytes;
        std::snprintf(buf, num_hex_bytes + 1, "%s", ss.str().c_str());
    } // get64RandomBytes

    bool init_shared_memory_for_plugin(const nlohmann::json& _plugin_object)
    {
        const auto itr = _plugin_object.find(irods::KW_CFG_SHARED_MEMORY_INSTANCE);

        if (_plugin_object.end() != itr) {
            const auto& mem_name = itr->get_ref<const std::string&>();
            prepareServerSharedMemory(mem_name);
            return true;
        }

        return false;
    } // init_shared_memory_for_plugin

    irods::error init_shared_memory_for_plugins()
    {
        try {
            const auto& config = irods::server_properties::instance().map();

            for (const auto& item : config.at(irods::KW_CFG_PLUGIN_CONFIGURATION).items()) {
                for (const auto& plugin : item.value().items()) {
                    init_shared_memory_for_plugin(plugin.value());
                }
            }
        }
        catch (const irods::exception& e) {
            return irods::error(e);
        }
        catch (const std::exception& e) {
            return ERROR(SYS_INTERNAL_ERR, e.what());
        }

        return SUCCESS();
    } // init_shared_memory_for_plugins

    bool deinit_shared_memory_for_plugin(const nlohmann::json& _plugin_object)
    {
        const auto itr = _plugin_object.find(irods::KW_CFG_SHARED_MEMORY_INSTANCE);

        if (_plugin_object.end() != itr) {
            const auto& mem_name = itr->get_ref<const std::string&>();
            removeSharedMemory(mem_name);
            resetMutex(mem_name.c_str());
            return true;
        }

        return false;
    } // deinit_shared_memory_for_plugin

    irods::error deinit_shared_memory_for_plugins()
    {
        try {
            const auto& config = irods::server_properties::instance().map();

            for (const auto& item : config.at(irods::KW_CFG_PLUGIN_CONFIGURATION).items()) {
                for (const auto& plugin : item.value().items()) {
                    deinit_shared_memory_for_plugin(plugin.value());
                }
            }
        }
        catch (const irods::exception& e) {
            return irods::error(e);
        }
        catch (const std::exception& e) {
            return ERROR(SYS_INTERNAL_ERR, e.what());
        }

        return SUCCESS();
    } // deinit_shared_memory_for_plugins

    std::optional<nlohmann::json> load_server_config()
    {
        try {
            // Find the server configuration file.
            std::string config_path;

            if (const auto err = irods::get_full_path_for_config_file("server_config.json", config_path); !err.ok()) {
                return std::nullopt;
            }

            // Load the server configuration file in as JSON.
            nlohmann::json config;

            if (std::ifstream in{config_path}; in) {
                in >> config;
            }
            else {
                return std::nullopt;
            }

            return config;
        }
        catch (...) {
        }

        return std::nullopt;
    } // load_server_config

    void
    init_logger(const nlohmann::json& _server_config, bool _write_to_stdout = false, bool _enable_test_mode = false)
    {
        ix::log::init(_write_to_stdout, _enable_test_mode);
        irods::server_properties::instance().capture();
        log_server::set_level(ix::log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_SERVER));
        ix::log::set_server_type("server");
        ix::log::set_server_zone(_server_config.at(irods::KW_CFG_ZONE_NAME).get<std::string>());

        if (char hostname[HOST_NAME_MAX + 1]{}; gethostname(hostname, sizeof(hostname)) == 0) {
            ix::log::set_server_hostname(hostname);
        }
    }

    void remove_leftover_rulebase_pid_files(const nlohmann::json& _server_config) noexcept
    {
        namespace fs = boost::filesystem;

        try {
            // Find the NREP.
            const auto& plugin_config = _server_config.at(irods::KW_CFG_PLUGIN_CONFIGURATION);
            const auto& rule_engines = plugin_config.at(irods::KW_CFG_PLUGIN_TYPE_RULE_ENGINE);

            const auto end = std::end(rule_engines);
            const auto nrep = std::find_if(std::begin(rule_engines), end, [](const nlohmann::json& _object) {
                return _object.at(irods::KW_CFG_PLUGIN_NAME).get<std::string>() == "irods_rule_engine_plugin-irods_rule_language";
            });
            if (nrep == end) {
                return;
            }

            // Get the rulebase set.
            const auto& plugin_specific_config = nrep->at(irods::KW_CFG_PLUGIN_SPECIFIC_CONFIGURATION);
            const auto& rulebase_set = plugin_specific_config.at(irods::KW_CFG_RE_RULEBASE_SET);

            // Iterate over the list of rulebases and remove the leftover PID files.
            for (const auto& rb : rulebase_set) {
                // Create a pattern based on the rulebase's filename. The pattern will have the following format:
                //
                //    .+/<rulebase_name>\.re\.\d+
                //
                // Where <rulebase_name> is a placeholder for the target rulebase.
                std::string pattern_string = ".+/";
                pattern_string += rb.get<std::string>();
                pattern_string += R"_(\.re\.\d+)_";

                const std::regex pattern{pattern_string};

                for (const auto& p : fs::directory_iterator{irods::get_irods_config_directory()}) {
                    if (std::regex_match(p.path().c_str(), pattern)) {
                        try {
                            fs::remove(p);
                        }
                        catch (...) {}
                    }
                }
            }
        }
        catch (...) {}
    } // remove_leftover_rulebase_pid_files

    void create_stacktrace_directory()
    {
        namespace fs = boost::filesystem;
        boost::system::error_code ec;
        fs::create_directories(irods::get_irods_stacktrace_directory(), ec);
    } // create_stacktrace_directory

    int get_stacktrace_file_processor_sleep_time()
    {
        try {
            const auto adv_settings = irods::server_properties::instance().map()
                .at(irods::KW_CFG_ADVANCED_SETTINGS);

            const auto iter = adv_settings
                .find(irods::KW_CFG_STACKTRACE_FILE_PROCESSOR_SLEEP_TIME_IN_SECONDS);

            if (iter != std::end(adv_settings)) {
                if (const auto seconds = iter->get<int>(); seconds > 0) {
                    return seconds;
                }
            }
        }
        catch (...) {}

        return 10; // The default sleep time.
    } // get_stacktrace_file_processor_sleep_time

    void log_stacktrace_files()
    {
        namespace fs = boost::filesystem;

        const auto stacktrace_directory = irods::get_irods_stacktrace_directory();

        for (auto&& entry : fs::directory_iterator{stacktrace_directory}) {
            // Expected filename format:
            //
            //     <epoch_seconds>.<epoch_milliseconds>.<agent_pid>
            //
            // 1. Extract the timestamp from the filename and convert it to ISO8601 format.
            // 2. Extract the agent pid from the filename.
            const auto p = entry.path().generic_string();

            // TODO std::string::rfind can be replaced with std::string::ends_with in C++20.
            if (p.rfind(irods::STACKTRACE_NOT_READY_FOR_LOGGING_SUFFIX) != std::string::npos) {
                log_server::trace("Skipping [{}] ...", p);
                continue;
            }

            auto slash_pos = p.rfind("/");

            if (slash_pos == std::string::npos) {
                log_server::trace("Skipping [{}]. No forward slash separator found.", p);
                continue;
            }

            ++slash_pos;
            const auto first_dot_pos = p.find(".", slash_pos);

            if (first_dot_pos == std::string::npos) {
                log_server::trace("Skipping [{}]. No dot separator found.", p);
                continue;
            }

            const auto last_dot_pos = p.rfind(".");

            if (last_dot_pos == std::string::npos || last_dot_pos == first_dot_pos) {
                log_server::trace("Skipping [{}]. No dot separator found.", p);
                continue;
            }

            const auto epoch_seconds = p.substr(slash_pos, first_dot_pos - slash_pos);
            const auto remaining_millis = p.substr(first_dot_pos + 1, last_dot_pos - (first_dot_pos + 1));
            const auto pid = p.substr(last_dot_pos + 1);
            log_server::trace(
                "epoch seconds = [{}], remaining millis = [{}], agent pid = [{}]",
                epoch_seconds,
                remaining_millis,
                pid);

            try {
                // Convert the epoch value to ISO8601 format.
                log_server::trace("Converting epoch seconds to UTC timestamp ...");
                using boost::chrono::system_clock;
                using boost::chrono::time_fmt;
                const auto tp = system_clock::from_time_t(std::stoll(epoch_seconds));
                std::ostringstream utc_ss;
                utc_ss << time_fmt(boost::chrono::timezone::utc, "%FT%T") << tp;

                // Read the contents of the file.
                std::ifstream file{p};
                const auto stacktrace = boost::stacktrace::stacktrace::from_dump(file);
                file.close();

                // 3. Write the contents of the stacktrace file to syslog.
                irods::experimental::log::server::critical({
                    {"log_message", boost::stacktrace::to_string(stacktrace)},
                    {"stacktrace_agent_pid", pid},
                    {"stacktrace_timestamp_utc", fmt::format("{}.{}Z", utc_ss.str(), remaining_millis)},
                    {"stacktrace_timestamp_epoch_seconds", epoch_seconds},
                    {"stacktrace_timestamp_epoch_milliseconds", remaining_millis}
                });

                // 4. Delete the stacktrace file.
                //
                // We don't want the stacktrace files to go away without making it into the log.
                // We can't rely on the log invocation above because of syslog.
                // We don't want these files to accumulate for long running servers.
                log_server::trace("Removing stacktrace file from disk ...");
                fs::remove(entry);
            }
            catch (...) {
                // Something happened while logging the stacktrace file.
                // Leaving the stacktrace file in-place for processing later.
                log_server::trace("Caught exception while processing stacktrace file.");
            }
        }
    } // log_stacktrace_files

    void set_reload_server_config_flag(int sig)
    {
        reload_server_config = true;
    } // set_reload_server_config_flag

    void setup_signal_handlers() noexcept
    {
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_DFL); // Setting SIGCHLD to SIG_IGN is not portable.
        signal(SIGPIPE, SIG_IGN);
#ifdef osx_platform
        signal(SIGINT, (sig_t) serverExit);
        signal(SIGHUP, (sig_t) set_reload_server_config_flag);
        signal(SIGTERM, (sig_t) serverExit);
#else
        signal(SIGINT, serverExit);
        signal(SIGHUP, set_reload_server_config_flag);
        signal(SIGTERM, serverExit);
#endif
    } // setup_signal_handlers

    std::optional<std::string> get_grid_configuration_option_value(RcComm& _comm,
                                                                   const std::string_view _namespace,
                                                                   const std::string_view _option_name)
    {
        GridConfigurationInput input{};
        std::strcpy(input.name_space, _namespace.data());
        std::strcpy(input.option_name, _option_name.data());

        GridConfigurationOutput* output{};
        irods::at_scope_exit free_output{[&output] { std::free(output); }};

        if (const auto ec = rc_get_grid_configuration_value(&_comm, &input, &output); ec != 0) {
            log_server::error(
                "Failed to get option value from r_grid_configuration "
                "[error_code={}, namespace={}, option_name={}].",
                ec,
                _namespace,
                _option_name);
            return std::nullopt;
        }

        return output->option_value;
    } // get_grid_configuration_option_value

    void set_delay_server_migration_info(RcComm& _comm,
                                         const std::string_view _leader,
                                         const std::string_view _successor)
    {
        DelayServerMigrationInput input{};
        std::strcpy(input.leader, _leader.data());
        std::strcpy(input.successor, _successor.data());

        if (const auto ec = rc_set_delay_server_migration_info(&_comm, &input); ec != 0) {
            log_server::error(
                "Failed to set delay server migration info in r_grid_configuration "
                "[error_code={}, leader={}, successor={}].",
                ec,
                _leader,
                _successor);
        }
    } // set_delay_server_migration_info

    std::optional<bool> is_delay_server_running_on_remote_host(const std::string_view _hostname)
    {
        try {
            irods::control_plane_command cpc_cmd;
            cpc_cmd.command = irods::SERVER_CONTROL_STATUS;
            cpc_cmd.options[irods::SERVER_CONTROL_OPTION_KW] = irods::SERVER_CONTROL_HOSTS_OPT;
            cpc_cmd.options[irods::SERVER_CONTROL_HOST_KW + "0"] = _hostname;
            log_server::trace("Configured control plane command.");

            rodsEnv env;
            _getRodsEnv(env);
            log_server::trace("Captured local environment information.");

            // Build an encryption context.
            std::string_view encryption_key = env.irodsCtrlPlaneKey;
            irods::buffer_crypt crypt(encryption_key.size(),
                                      0,
                                      env.irodsCtrlPlaneEncryptionNumHashRounds,
                                      env.irodsCtrlPlaneEncryptionAlgorithm);
            log_server::trace("Constructed irods::buffer_crypt instance.");

            // Serialize using the generated avro class.
            auto out = avro::memoryOutputStream();
            avro::EncoderPtr e = avro::binaryEncoder();
            e->init(*out);
            avro::encode(*e, cpc_cmd);
            std::shared_ptr<std::vector<std::uint8_t>> data = avro::snapshot(*out);
            log_server::trace("Serialized control plane command using Avro encoder.");

            // Encrypt outgoing request.
            std::vector<std::uint8_t> enc_data(std::begin(*data), std::end(*data));
            irods::buffer_crypt::array_t iv;
            irods::buffer_crypt::array_t in_buf(std::begin(enc_data), std::end(enc_data));
            irods::buffer_crypt::array_t shared_secret(std::begin(encryption_key), std::end(encryption_key));

            irods::buffer_crypt::array_t data_to_send;

            if (const auto err = crypt.encrypt(shared_secret, iv, in_buf, data_to_send); !err.ok()) {
                THROW(SYS_INTERNAL_ERR, err.result());
            }

            log_server::trace("Encrypted request.");

            const auto zmq_server_target = fmt::format("tcp://{}:{}", env.rodsHost, env.irodsCtrlPlanePort);
            log_server::trace("Connecting to local control plane [{}] ...", zmq_server_target);

            if (!is_control_plane_accepting_requests.load()) {
                log_server::info(
                    "Cannot determine if delay server on remote host [{}] is running at this time. "
                    "Control plane is not accepting requests.",
                    _hostname);

                // The following only applies to delay server migration.
                // Return true so that the successor does not attempt to launch the delay server.
                // Returning std::nullopt or false can possibly lead to the delay server being launched.
                return true;
            }

            zmq::context_t zmq_ctx;
            zmq::socket_t zmq_socket{zmq_ctx, zmq::socket_type::req};
            zmq_socket.set(zmq::sockopt::sndtimeo, 500);
            zmq_socket.set(zmq::sockopt::rcvtimeo, 5000);
            zmq_socket.set(zmq::sockopt::linger, 0);
            zmq_socket.connect(zmq_server_target);
            log_server::trace("Connection established.");

            log_server::trace("Sending request ...");
            zmq::message_t request(data_to_send.size());
            std::memcpy(request.data(), data_to_send.data(), data_to_send.size());

            if (const auto res = zmq_socket.send(request, zmq::send_flags::none); !res || *res == 0) {
                THROW(SYS_LIBRARY_ERROR, "zmq::socket_t::send");
            }

            log_server::trace("Sent request. Waiting for response ...");

            zmq::message_t response;

            if (const auto res = zmq_socket.recv(response); !res || *res == 0) {
                THROW(SYS_LIBRARY_ERROR, "zmq::socket_t::recv");
            }

            log_server::trace("Response received. Processing response ...");

            // Decrypt the response.
            const auto* data_ptr = static_cast<const std::uint8_t*>(response.data());
            in_buf.clear();
            in_buf.assign(data_ptr, data_ptr + response.size());

            irods::buffer_crypt::array_t dec_data;

            if (const auto err = crypt.decrypt(shared_secret, iv, in_buf, dec_data); !err.ok()) {
                THROW(SYS_INTERNAL_ERR, err.result());
            }

            log_server::trace("Decrypted response.");

            std::string response_string(dec_data.data(), dec_data.data() + dec_data.size());
            log_server::trace("Control plane response ==> [{}]", response_string);

            if (const auto pos = response_string.find_last_of(","); pos != std::string::npos) {
                response_string[pos] = ' ';
            }

            const auto grid_status = nlohmann::json::parse(response_string);
            log_server::trace("Control plane response ==> [{}]", grid_status.dump());

            return grid_status.at("delay_server_pid").get<int>() > 0;
        }
        catch (const zmq::error_t& e) {
            log_server::error("ZMQ error: {}", e.what());
        }
        catch (const std::exception& e) {
            log_server::error("General exception: {}", e.what());
        }

        return std::nullopt;
    } // is_delay_server_running_on_remote_host

    void launch_delay_server(bool _enable_test_mode, bool _write_to_stdout)
    {
        log_server::info("Forking Delay Server (irodsDelayServer) ...");

        // If we're planning on calling one of the functions from the exec-family,
        // then we're only allowed to use async-signal-safe functions following the call
        // to fork(). To avoid potential issues, we build up the argument list before
        // doing the fork-exec.

        std::vector<char*> args{"irodsDelayServer"};

        if (_enable_test_mode) {
            args.push_back("-t");
        }

        if (_write_to_stdout) {
            args.push_back("-u");
        }

        args.push_back(nullptr);

        if (fork() == 0) {
            execv(args[0], args.data());

            // If execv() fails, the POSIX standard recommends using _exit() instead of exit() to avoid
            // flushing stdio buffers and handlers registered by the parent.
            //
            // In the case of C++, this is necessary to avoid triggering destructors. Triggering a destructor
            // could result in assertions made by the struct/class being violatied. For some data types,
            // violating an assertion results in program termination (i.e. SIGABRT).
            _exit(1);
        }
    } // launch_delay_server

    std::string get_preferred_hostname(const std::string_view _hostname)
    {
        if (const auto hn = resolve_hostname(_hostname, hostname_resolution_scheme::match_preferred); hn) {
            return *hn;
        }

        return std::string{_hostname};
    } // get_preferred_hostname

    void migrate_delay_server(bool _enable_test_mode, bool _write_to_stdout)
    {
        try {
            if (!is_control_plane_accepting_requests.load()) {
                return;
            }

            // At this point, we don't have a database connection because grandpa never needed
            // one. initServer() calls initServerInfo() which attempts to establish a database connection.
            // initServer() immediately tears down the database connection. We can only guess that this is
            // done because grandpa never accesses the database.
            //
            // The lines that follow temporarily re-establish a database connection only if the local
            // server is a catalog provider. This is necessary because none of the APIs invoked after this
            // database code results in the creation of a database connection.

            std::string service_role;
            if (const auto err = get_catalog_service_role(service_role); !err.ok()) {
                log_server::error("Could not retrieve server role [error_code={}].", err.code());
                return;
            }

            const auto is_provider = (irods::KW_CFG_SERVICE_ROLE_PROVIDER == service_role);

            irods::at_scope_exit disconnect_from_database{[is_provider] {
                if (is_provider) {
                    if (const auto ec = chlClose(); ec != 0) {
                        log_server::error("Could not disconnect from database [error_code={}].", ec);
                    }
                }
            }};

            if (is_provider) {
                // Assume this server has database credentials and attempt to connect to the database.
                if (const auto ec = chlOpen(); ec != 0) {
                    log_server::error("Could not connect to database [error_code={}].", ec);
                    return;
                }
            }

            std::string hostname;

            {
                char hn[HOST_NAME_MAX + 1]{};

                if (gethostname(hn, sizeof(hn)) != 0) {
                    log_server::error("Failed to retrieve local server's hostname for delay server migration.");
                    return;
                }

                hostname = get_preferred_hostname(hn);
            }

            log_server::trace("Local server's hostname = [{}]", hostname);

            std::string leader;
            std::string successor;

            {
                namespace ss = irods::server_state;

                // Make sure the server is accepting requests before attempting to establish a connection.
                // The delay server migration logic runs in a separate thread. When the server is instructed
                // to shutdown, it's possible for the delay server migration logic to run during that time.
                // This can lead to errors appearing in the log file. The errors aren't harmful to the system,
                // but we want to avoid there if possible. This check helps this situation by shrinking the window.
                if (const auto state = ss::get_state(); state != ss::server_state::running) {
                    log_server::trace("Cannot establish client connection to local server at this time. "
                                      "Server is either shutting down or is paused.");
                    return;
                }

                ix::client_connection comm;

                auto result = get_grid_configuration_option_value(comm, "delay_server", "leader");
                if (!result) {
                    return;
                }

                leader = get_preferred_hostname(*result);
                log_server::trace("Leader's hostname = [{}]", leader);

                result = get_grid_configuration_option_value(comm, "delay_server", "successor");
                if (!result) {
                    return;
                }

                successor = get_preferred_hostname(*result);
                log_server::trace("Successor's hostname = [{}]", successor);
            }

            if (hostname == leader) {
                log_server::trace("Local server [{}] is the leader.", hostname);

                if (!successor.empty() && hostname != successor) {
                    log_server::trace("New successor detected!");

                    const auto pid = irods::get_pid_from_file(irods::PID_FILENAME_DELAY_SERVER);

                    if (!pid) {
                        log_server::trace("Could not get delay server PID.");
                        return;
                    }

                    log_server::trace("Delay server PID = [{}]", *pid);
                    log_server::info("Sending shutdown signal to delay server.");

                    // If the delay server is running locally, send SIGTERM to it and wait
                    // for graceful shutdown. The call to waitpid() will cause the CRON manager
                    // to stop making progress until it returns because it is single threaded.
                    kill(*pid, SIGTERM);

                    int child_status = 0;
                    waitpid(*pid, &child_status, 0);
                    log_server::info("Delay server has completed shutdown [exit_code={}].", WEXITSTATUS(child_status));
                }
                else {
                    log_server::trace("No successor detected. Checking if delay server needs to be launched ...");

                    // If the delay server is not running, launch it!
                    if (const auto pid = irods::get_pid_from_file(irods::PID_FILENAME_DELAY_SERVER); !pid) {
                        launch_delay_server(_enable_test_mode, _write_to_stdout);
                    }
                }
            }
            else if (hostname == successor) {
                log_server::trace("Local server [{}] is the successor.", hostname);

                const auto promote_to_leader_and_launch_delay_server = [_enable_test_mode, _write_to_stdout, &hostname] {
                    failed_delay_server_migration_communication_attempts = 0;

                    {
                        namespace ss = irods::server_state;

                        // Check the server's state before attempting to establish a client connection.
                        // See identical check above for more information on why this is necessary.
                        if (const auto state = ss::get_state(); state != ss::server_state::running) {
                            log_server::trace("Cannot establish client connection to local server at this time. "
                                              "Server is either shutting down or is paused.");
                            return;
                        }

                        ix::client_connection comm;
                        set_delay_server_migration_info(comm, hostname, "");
                    }

                    launch_delay_server(_enable_test_mode, _write_to_stdout);
                };

                // Wait for the leader (remote host) to shutdown its delay server.
                //
                // Because we're relying on the network to determine when it is safe to launch our own
                // delay server, we have to consider the case where network communication fails.
                //
                // For this situation, we keep a count of the number of failed communication attempts.
                // When the threshold for failed attempts is reached, the local server will be promoted
                // to the leader.
                if (const auto res = is_delay_server_running_on_remote_host(leader); res && !*res) {
                    promote_to_leader_and_launch_delay_server();
                }
                // Automatically promote this server to the leader after three failed communication attempts.
                else if (++failed_delay_server_migration_communication_attempts == 3) {
                    promote_to_leader_and_launch_delay_server();
                }
            }
        }
        catch (const std::exception& e) {
            log_server::error("Caught exception in migrate_delay_server(): {}", e.what());
        }
        catch (...) {
            log_server::error("Caught unknown exception in migrate_delay_server()");
        }
    } // migrate_delay_server

    void daemonize()
    {
        if (fork()) {
            // End the parent process immediately.
            exit(0);
        }

        if (setsid() < 0) {
            log_server::error("daemonize: setsid failed, errno = {}\n", errno);
            exit(1);
        }

        close(0);
        close(1);
        close(2);

        open("/dev/null", O_RDONLY);
        open("/dev/null", O_WRONLY);
        open("/dev/null", O_RDWR);
    } // daemonize

    void print_usage(const char* prog)
    {
        fmt::print("Usage: {} [-uvVqs]\n", prog);
        fmt::print(" -u  user command level, remain attached to the tty (foreground)\n");
        fmt::print(" -v  verbose (LOG_NOTICE)\n");
        fmt::print(" -V  very verbose (LOG_DEBUG10)\n");
        fmt::print(" -q  quiet (LOG_ERROR)\n");
        fmt::print(" -s  log SQL commands\n");
    }

    int purgeLockFileDir(int chkLockFlag)
    {
        char lockFilePath[MAX_NAME_LEN * 2];
        struct dirent* myDirent;
        struct stat statbuf;
        int status;
        int savedStatus = 0;
        struct flock myflock;
        unsigned int purgeTime;

        std::string lock_dir;
        irods::error ret = irods::get_full_path_for_unmoved_configs(LOCK_FILE_DIR, lock_dir);
        if (!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        DIR* dirPtr = opendir(lock_dir.c_str());
        if (dirPtr == NULL) {
            log_server::error("purgeLockFileDir: opendir error for [{}], errno = [{}]", lock_dir.c_str(), errno);
            return UNIX_FILE_OPENDIR_ERR - errno;
        }

        std::memset(&myflock, 0, sizeof(myflock));
        myflock.l_whence = SEEK_SET;
        purgeTime = time(0) - LOCK_FILE_PURGE_TIME;
        while ((myDirent = readdir(dirPtr)) != NULL) {
            if (strcmp(myDirent->d_name, ".") == 0 || strcmp(myDirent->d_name, "..") == 0) {
                continue;
            }
            snprintf(lockFilePath, MAX_NAME_LEN, "%-s/%-s", lock_dir.c_str(), myDirent->d_name);
            if (chkLockFlag) {
                int myFd;
                myFd = open(lockFilePath, O_RDWR | O_CREAT, 0644);
                if (myFd < 0) {
                    savedStatus = FILE_OPEN_ERR - errno;
                    rodsLogError(LOG_ERROR, savedStatus, "purgeLockFileDir: open error for [{}]", lockFilePath);
                    continue;
                }
                myflock.l_type = F_WRLCK;
                int error_code = fcntl(myFd, F_GETLK, &myflock);
                if (error_code != 0) {
                    log_server::error("fcntl failed in purgeLockFileDir with error code [{}]", error_code);
                }
                close(myFd);
                /* some process is locking it */
                if (myflock.l_type != F_UNLCK) {
                    continue;
                }
            }
            status = stat(lockFilePath, &statbuf);

            if (status != 0) {
                log_server::error("purgeLockFileDir: stat error for [{}], errno = [{}]", lockFilePath, errno);
                savedStatus = UNIX_FILE_STAT_ERR - errno;
                boost::system::error_code err;
                boost::filesystem::remove(boost::filesystem::path(lockFilePath), err);
                continue;
            }
            if (chkLockFlag && (int) purgeTime < statbuf.st_mtime) {
                continue;
            }
            if ((statbuf.st_mode & S_IFREG) == 0) {
                continue;
            }
            boost::system::error_code err;
            boost::filesystem::remove(boost::filesystem::path(lockFilePath), err);
        }
        closedir(dirPtr);

        return savedStatus;
    } // purgeLockFileDir

    void task_purge_lock_file()
    {
        std::size_t wait_time_ms = 0;
        const std::size_t purge_time_ms = LOCK_FILE_PURGE_TIME * 1000; // s to ms

        while (true) {
            const auto state = irods::server_state::get_state();

            if (irods::server_state::server_state::stopped == state ||
                irods::server_state::server_state::exited == state) {
                break;
            }

            rodsSleep(0, irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC * 1000); // second, microseconds
            wait_time_ms += irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC;

            if (wait_time_ms >= purge_time_ms) {
                wait_time_ms = 0;
                int status = purgeLockFileDir(1);
                if (status < 0) {
                    rodsLogError(LOG_ERROR, status, "task_purge_lock_file: purgeLockFileDir failed");
                }
            }
        }
    } // task_purge_lock_file

    int queueAgentProc(agentProc_t* agentProc, agentProc_t** agentProcHead, irodsPosition_t position)
    {
        if (agentProc == NULL || agentProcHead == NULL) {
            return USER__NULL_INPUT_ERR;
        }

        if (*agentProcHead == NULL) {
            *agentProcHead = agentProc;
            agentProc->next = NULL;
            return 0;
        }

        if (position == TOP_POS) {
            agentProc->next = *agentProcHead;
            *agentProcHead = agentProc;
        }
        else {
            agentProc_t* tmpAgentProc = *agentProcHead;
            while (tmpAgentProc->next != NULL) {
                tmpAgentProc = tmpAgentProc->next;
            }
            tmpAgentProc->next = agentProc;
            agentProc->next = NULL;
        }
        return 0;
    } // queueAgentProc

    void launch_read_worker_threads()
    {
        for (int i = 0; i < NUM_READ_WORKER_THR; ++i) {
            try {
                ReadWorkerThread[i] = new boost::thread(readWorkerTask);
            }
            catch (const boost::thread_resource_error&) {
                THROW(SYS_THREAD_RESOURCE_ERR, "Error launching read worker threads.");
            }
        }
    } // launch_read_worker_threads

    void launch_spawn_manager_thread()
    {
        try {
            SpawnManagerThread = new boost::thread(task_spawn_manager);
        }
        catch (const boost::thread_resource_error&) {
            THROW(SYS_THREAD_RESOURCE_ERR, "Error launching spawn management thread.");
        }
    } // launch_spawn_manager_thread

    void launch_purge_lock_file_thread(std::string_view _server_role)
    {
        // Start purge lock file thread
        if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == _server_role) {
            try {
                PurgeLockFileThread = new boost::thread(task_purge_lock_file);
            }
            catch (const boost::thread_resource_error&) {
                log_server::error("{}: Error launching thread.", __func__);
            }
        }
    } // launch_purge_lock_file_thread

    void join_purge_lock_file_thread(std::string_view _server_role)
    {
        if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == _server_role) {
            try {
                PurgeLockFileThread->join();
            }
            catch (const boost::thread_resource_error&) {
                log_server::error("{}: Error joining thread.", __func__);
            }
        }
    } // join_purge_lock_file_thread

    void join_spawn_manager_thread()
    {
        SpawnReqCond.notify_all();

        try {
            SpawnManagerThread->join();
        }
        catch (const boost::thread_resource_error&) {
            log_server::error("{}: Error joining thread.", __func__);
        }
    } // join_spawn_manager_thread

    void join_read_worker_threads()
    {
        for (int i = 0; i < NUM_READ_WORKER_THR; ++i) {
            ReadReqCond.notify_all();

            try {
                ReadWorkerThread[i]->join();
            }
            catch (const boost::thread_resource_error&) {
                log_server::error("{}: Error joining thread.", __func__);
            }
        }
    } // join_read_worker_threads
} // anonymous namespace

int main(int argc, char** argv)
{
    int c;
    char tmpStr1[100];
    char tmpStr2[100];
    bool write_to_stdout = false;
    bool enable_test_mode = false;

    ProcessType = SERVER_PT; // This process identifies itself as a server.

    if (const char* log_level = getenv(SP_LOG_LEVEL); log_level) {
        rodsLogLevel(atoi(log_level));
    }
    else {
        rodsLogLevel(LOG_NOTICE);
    }

    if (const char* sql_log_level = std::getenv(SP_LOG_SQL); sql_log_level) {
    	rodsLogSqlReq(1);
    }

    ServerBootTime = std::time(nullptr);

    while ((c = getopt(argc, argv, "tuvVqsh")) != EOF) {
        switch (c) {
            case 't':
                enable_test_mode = true;
                break;

            // Write to stdout - do not daemonize process.
            case 'u':
                write_to_stdout = true;
                break;

            // Enable verbose logging.
            case 'v':
                std::snprintf(tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_NOTICE);
                putenv(tmpStr1);
                rodsLogLevel(LOG_NOTICE);
                break;

            // Enable very verbose logging.
            case 'V':
                std::snprintf(tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_DEBUG10);
                putenv(tmpStr1);
                rodsLogLevel(LOG_DEBUG10);
                break;

            // Quiet (only log errors and above).
            case 'q':
                std::snprintf(tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_ERROR);
                putenv(tmpStr1);
                rodsLogLevel(LOG_ERROR);
                break;

            // Log SQL commands.
            case 's':
                std::snprintf(tmpStr2, 100, "%s=%d", SP_LOG_SQL, 1);
                putenv(tmpStr2);
                break;

            // Help
            case 'h':
                print_usage(argv[0]);
                exit(0);

            default:
                print_usage(argv[0]);
                exit(1);
        }
    }

    if (!write_to_stdout) {
        daemonize();
    }

    auto server_config = load_server_config();
    if (!server_config) {
        return 1;
    }

    init_logger(*server_config, write_to_stdout, enable_test_mode);

    log_server::info("Initializing server ...");

    const auto pid_file_fd = irods::create_pid_file(irods::PID_FILENAME_MAIN_SERVER);
    if (pid_file_fd == -1) {
        return 1;
    }

    irods::client_api_allowlist::init();

    irods::at_scope_exit deinit_server_state{[] { irods::server_state::deinit(); }};
    irods::server_state::init();
    irods::server_state::set_state(irods::server_state::server_state::running);

    create_stacktrace_directory();

    namespace hnc = irods::experimental::net::hostname_cache;
    hnc::init("irods_hostname_cache", irods::get_hostname_cache_shared_memory_size());
    irods::at_scope_exit deinit_hostname_cache{[] { hnc::deinit(); }};

    namespace dnsc = irods::experimental::net::dns_cache;
    dnsc::init("irods_dns_cache", irods::get_dns_cache_shared_memory_size());
    irods::at_scope_exit deinit_dns_cache{[] { dnsc::deinit(); }};

    ix::replica_access_table::init();
    irods::at_scope_exit deinit_replica_access_table{[] { ix::replica_access_table::deinit(); }};

    remove_leftover_rulebase_pid_files(*server_config);

    // Clear the temporary server config object used for initialization.
    // After this point, everything will rely on the irods::server_properties interface for
    // information defined in server_config.json.
    server_config.reset();

    irods::server_properties::instance().capture();

    using key_path_t = irods::configuration_parser::key_path_t;

    // Set the default value for evicting DNS cache entries.
    irods::set_server_property(
        key_path_t{irods::KW_CFG_ADVANCED_SETTINGS, irods::KW_CFG_DNS_CACHE, irods::KW_CFG_EVICTION_AGE_IN_SECONDS},
        irods::get_dns_cache_eviction_age());

    // Set the default value for evicting hostname cache entries.
    irods::set_server_property(
        key_path_t{irods::KW_CFG_ADVANCED_SETTINGS, irods::KW_CFG_HOSTNAME_CACHE, irods::KW_CFG_EVICTION_AGE_IN_SECONDS},
        irods::get_hostname_cache_eviction_age());

    setup_signal_handlers();

    // Create a directory for IPC related sockets. This directory protects IPC sockets from
    // external applications.
    if (!mkdtemp(unix_domain_socket_directory)) {
        log_server::error("Error creating temporary directory for iRODS sockets, mkdtemp errno [{}]: [{}].",
                          errno,
                          strerror(errno));
        return SYS_INTERNAL_ERR;
    }

    log_server::info("Setting up UNIX domain socket for agent factory ...");
    char random_suffix[65];
    get64RandomBytes(random_suffix);
    std::snprintf(agent_factory_socket_file, sizeof(agent_factory_socket_file), "%s/irods_factory_%s", unix_domain_socket_directory, random_suffix);

    // Set up local_addr for socket communication.
    local_addr.sun_family = AF_UNIX;
    std::snprintf(local_addr.sun_path, sizeof(local_addr.sun_path), "%s", agent_factory_socket_file);

    const auto launch_agent_factory = [&] {
        try {
            // Return immediately if the agent factory exists.
            // When the server is starting up, agent_spawning_pid will be zero. Therefore,
            // we skip checking for the agent factory on startup.
            if (agent_spawning_pid > 0 && waitpid(agent_spawning_pid, nullptr, WNOHANG) != -1) {
                return;
            }

            log_server::info("Forking agent factory ...");

            agent_spawning_pid = fork();

            if (agent_spawning_pid == 0) {
                close(pid_file_fd);

                ProcessType = AGENT_PT;

                try {
                    const auto ec = runIrodsAgentFactory(local_addr);

                    log_server::critical("Agent factory returned with error code [{}].", ec);

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
                    // This function must be called here due to the use of _exit() (just below). Address Sanitizer
                    // (ASan) relies on std::atexit handlers to report its findings. _exit() does not trigger any
                    // of the handlers registered by ASan, therefore, we manually run ASan just before the agent
                    // factory exits.
                    __lsan_do_leak_check();
#endif

                    // Collapse non-zero error codes into one.
                    //
                    // The agent factory is normally shut down via the SIGTERM signal. However, if the agent factory
                    // fails to fork an agent, it will return SYS_FORK_ERROR here.
                    //
                    // If the agent factory terminates for whatever reason, is respawned by the main server process,
                    // and then fails to fork an agent, the agent factory will most likely exit with a SIGABRT due to
                    // a failed assertion in boost::mutex or boost::condition_variable. Destructing a mutex while it
                    // is locked is not allowed by the Boost.Thread library.
                    //
                    // The only way to avoid this situation is to use _exit(). Using _exit() is safe here because this
                    // is the final step in shutting down the agent factory process.
                    _exit(ec == 0 ? 0 : 1);
                }
                catch (...) {
                    // If an exception is thrown for any reason, terminate the agent factory.
                    _exit(1);
                }
            }
            else if (agent_spawning_pid > 0) {
                // If the agent factory is no longer available (e.g. it crashed or something), close the
                // socket associated with the crashed agent factory.
                close(agent_conn_socket);

                log_server::info("Connecting to agent factory [agent_factory_pid={}] ...", agent_spawning_pid);

                agent_conn_socket = socket(AF_UNIX, SOCK_STREAM, 0);

                const auto start_time = std::time(nullptr);

                // Loop until grandpa establishes a connection to the new agent factory.
                // If more than five seconds have elapsed, perhaps something bad has happened.
                // In this case, just terminate the program.
                while (true) {
                    if (connect(agent_conn_socket, (const struct sockaddr*) &local_addr, sizeof(local_addr)) >= 0) {
                        break;
                    }

                    const auto saved_errno = errno;

                    if ((std::time(nullptr) - start_time) > 5) {
                        log_server::error("Error connecting to agent factory socket, errno = [{}]: {}",
                                          saved_errno,
                                          strerror(saved_errno));
                        exit(SYS_SOCK_CONNECT_ERR);
                    }
                }
            }
        }
        catch (...) {
            // Do not allow exceptions to escape the CRON task!
            // Failing to do so can cause the CRON manager thread to crash the server.
        }
    }; // launch_agent_factory

    launch_agent_factory();
    ix::cron::cron_builder agent_watcher;
    agent_watcher.interval(5).task(launch_agent_factory);
    ix::cron::cron::instance().add_task(agent_watcher.build());

    ix::cron::cron_builder cache_clearer;
    cache_clearer.interval(600).task([] {
        log_server::trace("Expiring old cache entries");
        irods::experimental::net::hostname_cache::erase_expired_entries();
        irods::experimental::net::dns_cache::erase_expired_entries();
    });
    ix::cron::cron::instance().add_task(cache_clearer.build());

    int acceptErrCnt = 0;

    // set re cache salt here
    irods::error ret = createAndSetRECacheSalt();
    if (!ret.ok()) {
        log_server::error("{}: createAndSetRECacheSalt error.\n{}", __func__, ret.result());
        exit(1);
    }

    ret = init_shared_memory_for_plugins();
    if (!ret.ok()) {
        irods::log(PASS(ret));
    }

    irods::at_scope_exit remove_shared_memory{[] { deinit_shared_memory_for_plugins(); }};

    irods::re_plugin_globals.reset(new irods::global_re_plugin_mgr);

    rsComm_t svrComm;
    int status = initServerMain(&svrComm, enable_test_mode, write_to_stdout);
    if (status < 0) {
        log_server::error("{}: initServerMain error. status = {}", __func__, status);
        exit(1);
    }

    std::string svc_role;
    ret = get_catalog_service_role(svc_role);
    if (!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    // Add the stacktrace file processor task to the CRON manager.
    {
        ix::cron::cron_builder task_builder;
        task_builder
            .interval(get_stacktrace_file_processor_sleep_time())
            .task(log_stacktrace_files);
        ix::cron::cron::instance().add_task(task_builder.build());
    }

    {
        ix::cron::cron_builder configuration_reloader;
        configuration_reloader.interval(0).task([]() {
            if (reload_server_config) {
                reload_server_config = false;
                auto diff = irods::server_properties::instance().reload();
                ix::json_events::run_hooks(diff);
                log_server::info("Configuration Reloaded.");
            }
        });
    }

    std::thread cron_manager_thread{[] {
        while (true) {
            try {
                const auto state = irods::server_state::get_state();

                if (irods::server_state::server_state::stopped == state ||
                    irods::server_state::server_state::exited == state)
                {
                    break;
                }

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100ms);

                ix::cron::cron::instance().run();
            }
            catch (...) {}
        }
    }};

    irods::at_scope_exit join_cron_manager_thread{[&] { cron_manager_thread.join(); }};

    std::uint64_t return_code = 0;

    try {
        // Launch the control plane.
        irods::server_control_plane ctrl_plane(irods::KW_CFG_SERVER_CONTROL_PLANE_PORT, is_control_plane_accepting_requests);

        launch_read_worker_threads();
        launch_spawn_manager_thread();
        launch_purge_lock_file_thread(svc_role);

        fd_set sockMask;
        FD_ZERO(&sockMask);
        SvrSock = svrComm.sock;

        while (true) {
            const auto state = irods::server_state::get_state();

            if (irods::server_state::server_state::stopped == state) {
                procChildren(&ConnectedAgentHead);

                // Shut down the agent factory.
                kill(agent_spawning_pid, SIGTERM);
                int agent_factory_status = 0;
                waitpid(agent_spawning_pid, &agent_factory_status, 0);
                log_server::info(
                    "Agent factory has completed shutdown [exit_code={}].", WEXITSTATUS(agent_factory_status));

                log_server::info("iRODS Server is exiting with state [{}].", to_string(state));
                break;
            }

            if (irods::server_state::server_state::paused == state) {
                procChildren(&ConnectedAgentHead);
                rodsSleep(0, irods::SERVER_CONTROL_FWD_SLEEP_TIME_MILLI_SEC * 1000);
                continue;
            }

            if (irods::server_state::server_state::running != state) {
                log_server::info("Invalid iRODS server state [{}].", to_string(state));
            }
            FD_SET(svrComm.sock, &sockMask);

            int numSock = 0;
            struct timeval time_out;
            time_out.tv_sec  = 0;
            time_out.tv_usec = irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC * 1000;

            while ((numSock = select(svrComm.sock + 1, &sockMask, nullptr, nullptr, &time_out)) < 0) {
                if (errno == EINTR) {
                    log_server::info("{}: select() interrupted", __func__);
                    FD_SET(svrComm.sock, &sockMask);
                    continue;
                }

                log_server::info("{}: select() error, errno = {}", __func__, errno);
                return -1;
            }

            if (reload_server_config) {
                // As a side effect of reading the server properties object, we acquire a lock
                // that cannot be had while the configuration is in the middle of reloading.
                //
                // This is important because otherwise the server may respond to a new request
                // before loading the new settings.
                irods::server_properties::instance().contains("");
            }

            procChildren(&ConnectedAgentHead);

            if (0 == numSock) {
                continue;
            }

            const int newSock = rsAcceptConn(&svrComm);
            if (newSock < 0) {
                if (++acceptErrCnt > MAX_ACCEPT_ERR_CNT) {
                    log_server::error("{}: Too many socket accept error. Exiting", __func__);
                    break;
                }

                log_server::info("{}: acceptConn() error, errno = {}", __func__, errno);
                continue;
            }
            else {
                acceptErrCnt = 0;
            }

            status = chkAgentProcCnt();
            if (status < 0) {
                log_server::info("{}: chkAgentProcCnt failed status = {}", __func__, status);

                // =-=-=-=-=-=-=-
                // create network object to communicate to the network
                // plugin interface.  repave with newSock as that is the
                // operational socket at this point

                irods::network_object_ptr net_obj;
                irods::error ret = irods::network_factory( &svrComm, net_obj );

                if (!ret.ok()) {
                    irods::log(PASS(ret));
                }
                else {
                    ret = sendVersion(net_obj, status, 0, nullptr, 0);
                    if (!ret.ok()) {
                        irods::log(PASS(ret));
                    }
                }

                status = mySockClose(newSock);
                printf("close status = %d\n", status);
                continue;
            }

            addConnReqToQueue(&svrComm, newSock);
        }

        join_purge_lock_file_thread(svc_role);

        procChildren(&ConnectedAgentHead);

        join_spawn_manager_thread();
        join_read_worker_threads();

        irods::server_state::set_state(irods::server_state::server_state::exited);
    }
    catch (const irods::exception& e) {
        log_server::error("Exception caught in server loop\n{}", e.what());
        return_code = e.code();
    }

    close(agent_conn_socket);
    unlink(agent_factory_socket_file);
    rmdir(unix_domain_socket_directory);

    log_server::info("iRODS Server is done.");

    return return_code;
}

void serverExit(int _signal)
{
    recordServerProcess(nullptr); // Unlink the process id file.

    close(agent_conn_socket);
    unlink(agent_factory_socket_file);
    rmdir(unix_domain_socket_directory);

    // Wake and terminate agent spawning process
    kill(agent_spawning_pid, SIGTERM);

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    // Calling this function is likely not async-signal-safe, but it is okay because
    // the code has been compiled with Address Sanitizer enabled. For that reason,
    // we can assume that the binary is not running in a production environment.
    __lsan_do_leak_check();
#endif

    _exit(_signal);
}

int procChildren(agentProc_t** agentProcHead)
{
    boost::unique_lock<boost::mutex> con_agent_lock(ConnectedAgentMutex);

    agentProc_t* tmpAgentProc = *agentProcHead;
    agentProc_t* finishedAgentProc{};
    agentProc_t* prevAgentProc{};

    while (tmpAgentProc) {
        // Check if pid is still an active process
        if (kill(tmpAgentProc->pid, 0)) {
            finishedAgentProc = tmpAgentProc;

            if (!prevAgentProc) {
                *agentProcHead = tmpAgentProc->next;
            }
            else {
                prevAgentProc->next = tmpAgentProc->next;
            }

            tmpAgentProc = tmpAgentProc->next;
            std::free(finishedAgentProc);
        }
        else {
            prevAgentProc = tmpAgentProc;
            tmpAgentProc = tmpAgentProc->next;
        }
    }

    con_agent_lock.unlock();

    return 0;
}

agentProc_t* getAgentProcByPid(int childPid, agentProc_t** agentProcHead)
{
    boost::unique_lock<boost::mutex> con_agent_lock(ConnectedAgentMutex);

    agentProc_t* tmpAgentProc = *agentProcHead;
    agentProc_t* prevAgentProc{};

    while (tmpAgentProc) {
        if (childPid == tmpAgentProc->pid) {
            if (!prevAgentProc) {
                *agentProcHead = tmpAgentProc->next;
            }
            else {
                prevAgentProc->next = tmpAgentProc->next;
            }

            break;
        }

        prevAgentProc = tmpAgentProc;
        tmpAgentProc = tmpAgentProc->next;
    }

    con_agent_lock.unlock();

    return tmpAgentProc;
}

int queueConnectedAgentProc(int childPid, agentProc_t* connReq, agentProc_t** agentProcHead)
{
    if (!connReq) {
        return USER__NULL_INPUT_ERR;
    }

    connReq->pid = childPid;

    boost::unique_lock<boost::mutex> con_agent_lock(ConnectedAgentMutex);
    queueAgentProc(connReq, agentProcHead, TOP_POS);
    con_agent_lock.unlock();

    return 0;
}

int spawnAgent(agentProc_t* connReq, agentProc_t** agentProcHead)
{
    if (!connReq) {
        return USER__NULL_INPUT_ERR;
    }

    const int newSock = connReq->sock;
    startupPack_t* startupPack = &connReq->startupPack;

    const int childPid = execAgent(newSock, startupPack);
    if (childPid > 0) {
        queueConnectedAgentProc(childPid, connReq, agentProcHead);
    }

    return childPid;
}

int sendEnvironmentVarIntToSocket(const char* var, int val, int socket)
{
    const auto msg = fmt::format("{}={};", var, val);
    const ssize_t status = send(socket, msg.c_str(), msg.size(), 0);

    if (status < 0) {
        log_server::error("Error in sendEnvironmentVarIntToSocket, errno = [{}]: {}", errno, strerror(errno));
    }
    else if (static_cast<std::size_t>(status) != msg.size()) {
        log_server::debug("Failed to send entire message in sendEnvironmentVarIntToSocket - msg [{}] is [{}] "
                          "bytes long, sent [{}] bytes",
                          msg.c_str(),
                          msg.size(),
                          status);
    }

    return status;
}

int sendEnvironmentVarStrToSocket(const char* var, const char* val, int socket)
{
    const auto msg = fmt::format("{}={};", var, val);
    const ssize_t status = send(socket, msg.c_str(), msg.size(), 0);

    if (status < 0) {
        log_server::error("Error in sendEnvironmentVarIntToSocket, errno = [{}]: {}", errno, strerror(errno));
    }
    else if (static_cast<std::size_t>(status) != msg.size()) {
        log_server::debug("Failed to send entire message in sendEnvironmentVarIntToSocket - msg [{}] is [{}] "
                          "bytes long, sent [{}] bytes",
                          msg.c_str(),
                          msg.size(),
                          status);
    }

    return status;
}

ssize_t sendSocketOverSocket(int writeFd, int socket)
{
    struct msghdr msg;
    struct iovec iov[1];

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr *cmptr;

    std::memset(control_un.control, 0, sizeof(control_un.control));
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int *) CMSG_DATA(cmptr)) = socket;

    msg.msg_name = nullptr;
    msg.msg_namelen = 0;

    iov[0].iov_base = (void*) "i";
    iov[0].iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    return sendmsg(writeFd, &msg, 0);
}

int execAgent(int newSock, startupPack_t* startupPack)
{
    // Create unique socket for each call to exec agent
    char random_suffix[65]{};
    get64RandomBytes(random_suffix);

    sockaddr_un tmp_socket_addr{};
    char tmp_socket_file[sizeof(tmp_socket_addr.sun_path)]{};
    std::snprintf(tmp_socket_file,
                  sizeof(tmp_socket_file),
                  "%s/irods_agent_%s",
                  unix_domain_socket_directory,
                  random_suffix);

    ssize_t status = send(agent_conn_socket, tmp_socket_file, strlen(tmp_socket_file), 0);
    if (status < 0) {
        log_server::error("Error sending socket to agent factory process, errno = [{}]: {}", errno, strerror(errno));
    }
    else if (static_cast<std::size_t>(status) < std::strlen(tmp_socket_file)) {
        log_server::debug("Failed to send entire message - msg [{}] is [{}] bytes long, sent [{}] bytes",
                          tmp_socket_file,
                          strnlen(tmp_socket_file, sizeof(tmp_socket_file)),
                          status);
    }

    tmp_socket_addr.sun_family = AF_UNIX;
    strncpy(tmp_socket_addr.sun_path, tmp_socket_file, sizeof(tmp_socket_addr.sun_path));

    int tmp_socket{socket(AF_UNIX, SOCK_STREAM, 0)};
    if (tmp_socket < 0) {
        log_server::error("Unable to create socket in execAgent, errno = [{}]: {}", errno, strerror(errno));
    }

    const auto cleanup_sockets{[&]() {
        if (close(tmp_socket) < 0) {
            log_server::error("close(tmp_socket) failed with errno = [{}]: {}", errno, strerror(errno));
        }

        if (unlink(tmp_socket_file) < 0) {
            log_server::error("unlink(tmp_socket_file) failed with errno = [{}]: {}", errno, strerror(errno));
        }
    }};

    // Wait until receiving acknowledgement that socket has been created
    char in_buf[1024]{};
    status = recv(agent_conn_socket, &in_buf, sizeof(in_buf), 0);
    if (status < 0) {
        log_server::error("Error in recv acknowledgement from agent factory process, errno = [{}]: {}",
                          errno,
                          strerror(errno));
        status = SYS_SOCK_READ_ERR;
    }
    else if (0 != strcmp(in_buf, "OK")) {
        log_server::error("Bad acknowledgement from agent factory process, message = [{}]", in_buf);
        status = SYS_SOCK_READ_ERR;
    }
    else {
        status = connect(tmp_socket, (const struct sockaddr*) &tmp_socket_addr, sizeof(local_addr));
        if (status < 0) {
            log_server::error("Unable to connect to socket in agent factory process, errno = [{}]: {}",
                              errno,
                              strerror(errno));
            status = SYS_SOCK_CONNECT_ERR;
        }
    }

    if (status < 0) {
        // Agent factory expects a message about connection to the agent - send failure
        const std::string failure_message{"spawn_failure"};
        send(agent_conn_socket, failure_message.c_str(), failure_message.length() + 1, 0);
        cleanup_sockets();
        return status;
    }
    else {
        // Notify agent factory of success and send data to agent process
        const std::string connection_successful{"connection_successful"};
        send(agent_conn_socket, connection_successful.c_str(), connection_successful.length() + 1, 0);
    }

    status = sendEnvironmentVarStrToSocket(SP_RE_CACHE_SALT,
                                           irods::get_server_property<const std::string>(irods::KW_CFG_RE_CACHE_SALT).c_str(),
                                           tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_RE_CACHE_SALT to agent");
    }

    status = sendEnvironmentVarIntToSocket(SP_CONNECT_CNT, startupPack->connectCnt, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_CONNECT_CNT to agent");
    }

    status = sendEnvironmentVarStrToSocket(SP_PROXY_RODS_ZONE, startupPack->proxyRodsZone, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_PROXY_RODS_ZONE to agent");
    }

    status = sendEnvironmentVarIntToSocket(SP_NEW_SOCK, newSock, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_NEW_SOCK to agent");
    }

    status = sendEnvironmentVarIntToSocket(SP_PROTOCOL, startupPack->irodsProt, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_PROTOCOL to agent");
    }

    status = sendEnvironmentVarIntToSocket(SP_RECONN_FLAG, startupPack->reconnFlag, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_RECONN_FLAG to agent");
    }

    status = sendEnvironmentVarStrToSocket(SP_PROXY_USER, startupPack->proxyUser, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_PROXY_USER to agent");
    }

    status = sendEnvironmentVarStrToSocket(SP_CLIENT_USER, startupPack->clientUser, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_CLIENT_USER to agent");
    }

    status = sendEnvironmentVarStrToSocket(SP_CLIENT_RODS_ZONE, startupPack->clientRodsZone, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_CLIENT_RODS_ZONE to agent");
    }

    status = sendEnvironmentVarStrToSocket(SP_REL_VERSION, startupPack->relVersion, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_REL_VERSION to agent");
    }

    status = sendEnvironmentVarStrToSocket(SP_API_VERSION, startupPack->apiVersion, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SP_API_VERSION to agent");
    }

    // If the client-server negotiation request is in the option variable, set that env var
    // and strip it out
    std::string opt_str(startupPack->option);
    std::size_t pos = opt_str.find(REQ_SVR_NEG);
    if (std::string::npos != pos) {
        std::string trunc_str = opt_str.substr(0, pos);

        status = sendEnvironmentVarStrToSocket(SP_OPTION, trunc_str.c_str(), tmp_socket);
        if (status < 0) {
            log_server::error("Failed to send SP_OPTION to agent");
        }

        status = sendEnvironmentVarStrToSocket(irods::RODS_CS_NEG, REQ_SVR_NEG, tmp_socket);
        if (status < 0) {
            log_server::error("Failed to send irods::RODS_CS_NEG to agent");
        }
    }
    else {
        status = sendEnvironmentVarStrToSocket(SP_OPTION, startupPack->option, tmp_socket);
        if (status < 0) {
            log_server::error("Failed to send SP_OPTION to agent");
        }
    }

    status = sendEnvironmentVarIntToSocket(SERVER_BOOT_TIME, ServerBootTime, tmp_socket);
    if (status < 0) {
        log_server::error("Failed to send SERVER_BOOT_TIME to agent");
    }

    status = send(tmp_socket, "end_of_vars", 12, 0);
    if (status <= 0) {
        log_server::error("Failed to send \"end_of_vars;\" to agent");
    }

    status = recv(tmp_socket, &in_buf, sizeof(in_buf), 0);
    if (status < 0) {
        log_server::error("Error in recv acknowledgement from agent factory process, errno = [{}]: {}",
                          errno,
                          strerror(errno));
        cleanup_sockets();
        return SYS_SOCK_READ_ERR;
    }

    if (std::strcmp(in_buf, "OK") != 0) {
        log_server::error("Bad acknowledgement from agent factory process, message = [{}]", in_buf);
        cleanup_sockets();
        return SYS_SOCK_READ_ERR;
    }

    sendSocketOverSocket(tmp_socket, newSock);
    status = recv(tmp_socket, &in_buf, sizeof(in_buf), 0);
    if (status < 0) {
        log_server::error("Error in recv child_pid from agent factory process, errno = [{}]: {}",
                          errno,
                          strerror(errno));
        cleanup_sockets();
        return SYS_SOCK_READ_ERR;
    }

    cleanup_sockets();

    return std::atoi(in_buf);
}

int getAgentProcCnt()
{
    boost::unique_lock<boost::mutex> con_agent_lock(ConnectedAgentMutex);

    agentProc_t* tmpAgentProc = ConnectedAgentHead;
    int count = 0;

    while (tmpAgentProc) {
        ++count;
        tmpAgentProc = tmpAgentProc->next;
    }

    con_agent_lock.unlock();

    return count;
}

int getAgentProcPIDs(std::vector<int>& _pids)
{
    boost::unique_lock<boost::mutex> con_agent_lock(ConnectedAgentMutex);

    agentProc_t* tmp_proc = ConnectedAgentHead;
    int count = 0;

    while (tmp_proc) {
        count++;
        _pids.push_back(tmp_proc->pid);
        tmp_proc = tmp_proc->next;
    }

    con_agent_lock.unlock();

    return count;
} // getAgentProcPIDs

int chkAgentProcCnt()
{
    int maximum_connections = NO_MAX_CONNECTION_LIMIT;

    try {
        if (irods::server_property_exists("maximum_connections")) {
            maximum_connections = irods::get_server_property<const int>("maximum_connections");
            int count = getAgentProcCnt();

            if (count >= maximum_connections) {
                chkConnectedAgentProcQueue();
                count = getAgentProcCnt();

                if (count >= maximum_connections) {
                    return SYS_MAX_CONNECT_COUNT_EXCEEDED;
                }
            }
        }

    }
    catch (const nlohmann::json::exception& e) {
        log_server::error("%s failed with message [{}]", e.what());
        return SYS_INTERNAL_ERR;
    }

    return 0;
}

int chkConnectedAgentProcQueue()
{
    boost::unique_lock<boost::mutex> con_agent_lock(ConnectedAgentMutex);
    agentProc_t* tmpAgentProc = ConnectedAgentHead;
    agentProc_t* prevAgentProc{};

    while (tmpAgentProc) {
        char procPath[MAX_NAME_LEN];

        std::snprintf(procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, tmpAgentProc->pid);

        if (!boost::filesystem::exists(procPath)) {
            // The agent proc is gone.
            auto* unmatchedAgentProc = tmpAgentProc;
            log_server::debug("Agent process {} in Connected queue but not in ProcLogDir", tmpAgentProc->pid);

            if (!prevAgentProc) {
                ConnectedAgentHead = tmpAgentProc->next;
            }
            else {
                prevAgentProc->next = tmpAgentProc->next;
            }

            tmpAgentProc = tmpAgentProc->next;
            std::free(unmatchedAgentProc);
        }
        else {
            prevAgentProc = tmpAgentProc;
            tmpAgentProc = tmpAgentProc->next;
        }
    }

    con_agent_lock.unlock();

    return 0;
}

int initServer(rsComm_t* svrComm)
{
    int status = initServerInfo(0, svrComm);
    if (status < 0) {
        log_server::info("initServer: initServerInfo error, status = {}", status);
        return status;
    }

    resc_mgr.print_local_resources();

    printZoneInfo();

    rodsServerHost_t* rodsServerHost{};
    status = getRcatHost(PRIMARY_RCAT, nullptr, &rodsServerHost);

    if (status < 0 || !rodsServerHost) {
        return status;
    }

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if (!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }


    if (LOCAL_HOST == rodsServerHost->localFlag) {
        if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
            disconnectRcat();
        }
    }
    else if (rodsServerHost->conn) {
        rcDisconnect(rodsServerHost->conn);
        rodsServerHost->conn = nullptr;
    }

    if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
        purgeLockFileDir(0);
    }

    return status;
}

// record the server process number and other information into
// a well-known file. If svrComm is Null and this has created a file
// before, just unlink the file.
int recordServerProcess(rsComm_t* svrComm)
{
    static char filePath[100] = "";

    if (!svrComm) {
        if (filePath[0] != '\0') {
            unlink(filePath);
        }

        return 0;
    }

    rodsEnv* myEnv = &svrComm->myEnv;

    // Use /usr/tmp if it exists, /tmp otherwise.
    char* tmp;
    if (DIR* dirp = opendir("/usr/tmp"); dirp) {
        tmp = "/usr/tmp";
        closedir(dirp);
    }
    else {
        tmp = "/tmp";
    }

    char stateFile[] = "irodsServer";
    std::sprintf(filePath, "%s/%s.%d", tmp, stateFile, myEnv->rodsPort);
    unlink(filePath);

    char cwd[1000];

    if (char* cp = getcwd(cwd, 1000); cp) {
        auto* fd = std::fopen(filePath, "w");
        if (fd) {
            std::fprintf(fd, "%d %s\n", getpid(), cwd);
            std::fclose(fd);

            if (const int ec = chmod(filePath, 0664); ec != 0) {
                log_server::error("chmod failed in recordServerProcess on [{}] with error code {}", filePath, ec);
            }
        }
    }

    return 0;
}

int initServerMain(rsComm_t *svrComm,
                   const bool enable_test_mode = false,
                   const bool write_to_stdout = false)
{
    std::memset(svrComm, 0, sizeof(*svrComm));
    int status = getRodsEnv(&svrComm->myEnv);
    if (status < 0) {
        log_server::error("{}: getRodsEnv error. status = {}", __func__, status);
        return status;
    }
    initAndClearProcLog();

    setRsCommFromRodsEnv(svrComm);

    // Load server API table so that API plugins which are needed to stand up the server are
    // available for use.
    irods::api_entry_table& RsApiTable = irods::get_server_api_table();
    irods::pack_entry_table& ApiPackTable = irods::get_pack_table();
    if (const auto err = irods::init_api_table(RsApiTable, ApiPackTable, false); !err.ok()) {
        irods::log(PASS(err));
        return err.code();
    }

    // If this is a catalog service consumer, the client API table should be loaded so that
    // client calls can be made to the catalog service provider as part of the server
    // initialization process.
    irods::api_entry_table& RcApiTable = irods::get_client_api_table();
    if (const auto err = irods::init_api_table(RcApiTable, ApiPackTable, false); !err.ok()) {
        irods::log(PASS(err));
        return err.code();
    }

    status = initServer(svrComm);
    if (status < 0) {
        log_server::error("{}: initServer error. status = {}", __func__, status);
        exit(1);
    }

    int zone_port;
    try {
        zone_port = irods::get_server_property<const int>(irods::KW_CFG_ZONE_PORT);
    }
    catch (const irods::exception& e) {
        irods::log(irods::error(e));
        return e.code();
    }

    svrComm->sock = sockOpenForInConn(svrComm, &zone_port, nullptr, SOCK_STREAM);
    if (svrComm->sock < 0) {
        log_server::error("{}: sockOpenForInConn error. status = {}", __func__, svrComm->sock);
        return svrComm->sock;
    }

    if (listen(svrComm->sock, MAX_LISTEN_QUE) < 0) {
        log_server::error("{}: listen failed, errno: {}", __func__, errno);
        return SYS_SOCK_LISTEN_ERR;
    }

    log_server::info("rodsServer Release version {} - API Version {} is up", RODS_REL_VERSION, RODS_API_VERSION);

    // Record port, PID, and CWD into a well-known file.
    recordServerProcess(svrComm);

    // Setup the delay server CRON task.
    // The delay server will launch just before we enter the server's main loop.
    ix::cron::cron_builder delay_server;
    delay_server
        .interval(5)
        .task([enable_test_mode, write_to_stdout] {
            migrate_delay_server(enable_test_mode, write_to_stdout);
        });
    ix::cron::cron::instance().add_task(delay_server.build());

    return 0;
}

// Add incoming connection request to the bottom of the link list.
int addConnReqToQueue(rsComm_t* rsComm, int sock)
{
    boost::unique_lock<boost::mutex> read_req_lock(ReadReqCondMutex);
    auto* myConnReq = (agentProc_t*) std::calloc(1, sizeof(agentProc_t));

    myConnReq->sock = sock;
    myConnReq->remoteAddr = rsComm->remoteAddr;
    queueAgentProc(myConnReq, &ConnReqHead, BOTTOM_POS);

    ReadReqCond.notify_all(); // NOTE: Check all vs one.
    read_req_lock.unlock();

    return 0;
}

agentProc_t* getConnReqFromQueue()
{
    agentProc_t* myConnReq{};

    while (true) {
        const auto state = irods::server_state::get_state();

        if (irods::server_state::server_state::stopped == state ||
            irods::server_state::server_state::exited == state ||
            myConnReq)
        {
            break;
        }

        boost::unique_lock<boost::mutex> read_req_lock(ReadReqCondMutex);
        if (ConnReqHead) {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
            read_req_lock.unlock();
            break;
        }

        ReadReqCond.wait(read_req_lock);
        if (!ConnReqHead) {
            read_req_lock.unlock();
            continue;
        }

        myConnReq = ConnReqHead;
        ConnReqHead = ConnReqHead->next;
        read_req_lock.unlock();
        break;
    }

    return myConnReq;
}

int process_bad_request()
{
    boost::unique_lock<boost::mutex> bad_req_lock(BadReqMutex);

    agentProc_t* tmpConnReq = BadReqHead;
    agentProc_t* nextConnReq{};

    // Just free them for now.

    while (tmpConnReq) {
        nextConnReq = tmpConnReq->next;
        std::free(tmpConnReq);
        tmpConnReq = nextConnReq;
    }

    BadReqHead = nullptr;
    bad_req_lock.unlock();

    return 0;
}

void task_spawn_manager()
{
    unsigned int agentQueChkTime = 0;

    while (true) {
        const auto state = irods::server_state::get_state();

        if (irods::server_state::server_state::stopped == state || irods::server_state::server_state::exited == state) {
            break;
        }

        boost::unique_lock<boost::mutex> spwn_req_lock(SpawnReqCondMutex);
        SpawnReqCond.wait(spwn_req_lock);

        while (SpawnReqHead) {
            agentProc_t* mySpawnReq = SpawnReqHead;
            SpawnReqHead = SpawnReqHead->next;

            spwn_req_lock.unlock();
            auto status = spawnAgent(mySpawnReq, &ConnectedAgentHead);
            close(mySpawnReq->sock);

            if (status < 0) {
                log_server::info(
                    "spawnAgent error for puser=[{}] and cuser=[{}] from [{}], stat=[{}]",
                    mySpawnReq->startupPack.proxyUser,
                    mySpawnReq->startupPack.clientUser,
                    inet_ntoa(mySpawnReq->remoteAddr.sin_addr),
                    status);
                std::free(mySpawnReq);
            }
            else {
                log_server::debug(
                    "Agent process [{}] started for puser=[{}] and cuser=[{}] from [{}]",
                    mySpawnReq->pid,
                    mySpawnReq->startupPack.proxyUser,
                    mySpawnReq->startupPack.clientUser,
                    inet_ntoa(mySpawnReq->remoteAddr.sin_addr));
            }

            spwn_req_lock.lock();
        }

        spwn_req_lock.unlock();

        if (unsigned int curTime = std::time(nullptr); curTime > agentQueChkTime + AGENT_QUE_CHK_INT) {
            agentQueChkTime = curTime;
            process_bad_request();
        }
    }
}

void readWorkerTask()
{
    // Artificially create a conn object in order to create a network object.
    // This is gratuitous but necessary to maintain the consistent interface.
    RcComm tmp_comm{};

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory(&tmp_comm, net_obj);

    if (!ret.ok() || !net_obj.get()) {
        irods::log(PASS(ret));
        return;
    }

    while (true) {
        const auto state = irods::server_state::get_state();

        if (irods::server_state::server_state::stopped == state ||
            irods::server_state::server_state::exited == state)
        {
            break;
        }

        agentProc_t* myConnReq = getConnReqFromQueue();
        if (!myConnReq) {
            // Someone else took care of it.
            continue;
        }

        int newSock = myConnReq->sock;

        // Repave the socket handle with the new socket for this connection.
        net_obj->socket_handle(newSock);
        startupPack_t* startupPack = nullptr;
        struct timeval tv;
        tv.tv_sec = READ_STARTUP_PACK_TOUT_SEC;
        tv.tv_usec = 0;
        irods::error ret = readStartupPack(net_obj, &startupPack, &tv);

        if (!ret.ok()) {
            log_server::error("readWorkerTask - readStartupPack failed. {}", ret.code());
            sendVersion(net_obj, ret.code(), 0, nullptr, 0);
            boost::unique_lock<boost::mutex> bad_req_lock(BadReqMutex);
            queueAgentProc(myConnReq, &BadReqHead, TOP_POS);
            bad_req_lock.unlock();
            mySockClose(newSock);
        }
        else if (std::strcmp(startupPack->option, RODS_HEARTBEAT_T) == 0) {
            const char* heartbeat = RODS_HEARTBEAT_T;
            const int heartbeat_length = std::strlen(heartbeat);
            int bytes_to_send = heartbeat_length;

            while (bytes_to_send) {
                const int bytes_sent = send(newSock, &(heartbeat[heartbeat_length - bytes_to_send]), bytes_to_send, 0);
                const int errsav = errno;
                if (bytes_sent > 0) {
                    bytes_to_send -= bytes_sent;
                }
                else if (errsav != EINTR) {
                    log_server::error("Socket error encountered during heartbeat; socket returned {}",
                                      strerror(errsav));
                    break;
                }
            }

            mySockClose(newSock);
            std::free(myConnReq);
            std::free(startupPack);
        }
        else if (startupPack->connectCnt > MAX_SVR_SVR_CONNECT_CNT) {
            sendVersion(net_obj, SYS_EXCEED_CONNECT_CNT, 0, nullptr, 0);
            mySockClose(newSock);
            std::free(myConnReq);
            std::free(startupPack);
        }
        else {
            if (startupPack->clientUser[0] != '\0') {
                if (const auto status = chkAllowedUser(startupPack->clientUser, startupPack->clientRodsZone); status < 0) {
                    sendVersion(net_obj, status, 0, nullptr, 0);
                    mySockClose(newSock);
                    std::free(myConnReq);
                    continue;
                }
            }

            myConnReq->startupPack = *startupPack;
            std::free(startupPack);

            boost::unique_lock<boost::mutex> spwn_req_lock(SpawnReqCondMutex);

            queueAgentProc(myConnReq, &SpawnReqHead, BOTTOM_POS);

            SpawnReqCond.notify_all(); // NOTE:: look into notify_one vs notify_all
        }
    }
} // readWorkerTask

int procSingleConnReq(agentProc_t* connReq)
{
    if (!connReq) {
        return USER__NULL_INPUT_ERR;
    }

    int newSock = connReq->sock;

    // =-=-=-=-=-=-=-
    // artificially create a conn object in order to
    // create a network object.  this is gratuitous
    // but necessary to maintain the consistent interface.
    rcComm_t tmp_comm;
    std::memset(&tmp_comm, 0, sizeof(rcComm_t));

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory(&tmp_comm, net_obj);
    if (!ret.ok()) {
        irods::log(PASS(ret));
        return -1;
    }

    net_obj->socket_handle(newSock);

    startupPack_t* startupPack;
    ret = readStartupPack(net_obj, &startupPack, nullptr);

    if (!ret.ok()) {
        log_server::info("readStartupPack error from {}, status = {}",
                         inet_ntoa(connReq->remoteAddr.sin_addr),
                         ret.code());
        sendVersion(net_obj, ret.code(), 0, nullptr, 0);
        mySockClose(newSock);
        return ret.code();
    }

    if (startupPack->connectCnt > MAX_SVR_SVR_CONNECT_CNT) {
        sendVersion(net_obj, SYS_EXCEED_CONNECT_CNT, 0, nullptr, 0);
        mySockClose(newSock);
        return SYS_EXCEED_CONNECT_CNT;
    }

    connReq->startupPack = *startupPack;
    std::free(startupPack);

    int status = spawnAgent(connReq, &ConnectedAgentHead);

    close(newSock);

    if (status < 0) {
        log_server::info("spawnAgent error for puser={} and cuser={} from {}, status = {}",
                         connReq->startupPack.proxyUser,
                         connReq->startupPack.clientUser,
                         inet_ntoa(connReq->remoteAddr.sin_addr),
                         status);
    }
    else {
        log_server::debug("Agent process {} started for puser={} and cuser={} from {}",
                          connReq->pid,
                          connReq->startupPack.proxyUser,
                          connReq->startupPack.clientUser,
                          inet_ntoa(connReq->remoteAddr.sin_addr));
    }

    return status;
}
