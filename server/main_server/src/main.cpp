#include "irods/access_time_queue.hpp"
#include "irods/catalog.hpp"
#include "irods/chrono.hpp"
#include "irods/client_connection.hpp"
#include "irods/dns_cache.hpp"
#include "irods/getRodsEnv.h"
#include "irods/get_grid_configuration_value.h"
#include "irods/hostname_cache.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_client_api_table.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_default_paths.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_server_api_table.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_signal.hpp"
#include "irods/irods_version.h"
#include "irods/notify_service_manager.hpp"
#include "irods/plugins/api/delay_server_migration_types.h"
#include "irods/plugins/api/grid_configuration_types.h"
#include "irods/rcConnect.h" // For RcComm
#include "irods/rcGlobalExtern.h" // For ProcessType
#include "irods/rcMisc.h"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/update_replica_access_time.h"
#include "irods/set_delay_server_migration_info.h"

#include <boost/any.hpp>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios> // For std::streamsize
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>

#ifdef __cpp_lib_filesystem
#  include <filesystem>
#else
#  include <boost/filesystem.hpp>
#endif

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

#if __has_feature(undefined_behavior_sanitizer)
extern "C" const char* __ubsan_default_options()
{
    // See root CMakeLists.txt file for definition.
    return IRODS_UNDEFINED_BEHAVIOR_SANITIZER_DEFAULT_OPTIONS;
} // __ubsan_default_options
#endif

namespace
{
#ifdef __cpp_lib_filesystem
    namespace fs = std::filesystem;
#else
    namespace fs = boost::filesystem;
#endif

    namespace log_ns = irods::experimental::log;

    using log_server = irods::experimental::log::server;

    volatile std::sig_atomic_t g_terminate = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    volatile std::sig_atomic_t g_terminate_graceful = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    volatile std::sig_atomic_t g_reload_config = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    // This global variable MUST always hold the max number of child processes forkable
    // by the main server process. It guarantees that the main server process reaps children
    // it's lost due to desync issues stemming from fork(), exec() and kill().
    const int g_max_number_of_child_processes = 2;

    pid_t g_pid_af = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    pid_t g_pid_ds = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    // Indicates whether the logging system has been initialized. This is meant to give
    // systems, which run before and after the logging system is ready, a way to determine
    // when use of the logging system is safe.
    bool g_logger_initialized = false; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    // Stores the grid configuration value of access_time.resolution_in_seconds. This
    // global variable's only purpose is to allow the main server process to pass the
    // configured value to the agent factory.
    std::string g_atime_resolution_in_seconds; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    // Stores the grid configuration value of access_time.batch_size. This global variable
    // is used by the main server process to limit the number of atime updates that can be
    // applied in one SQL batch update.
    std::size_t g_atime_batch_size = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    bool g_atime_invalid_batch_size_detected = false; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    auto print_usage() -> void;
    auto print_version_info() -> void;

    auto validate_configuration() -> bool;
    auto daemonize() -> void;
    auto create_pid_file(const std::string& _pid_file) -> int;
    auto init_logger(pid_t _pid, bool _write_to_stdout, bool _enable_test_mode) -> void;
    auto check_catalog_schema_version() -> std::pair<bool, int>;
    auto setup_signal_handlers() -> int;
    auto wait_for_external_dependent_servers_to_start() -> int;

    auto handle_shutdown() -> void;
    auto handle_shutdown_graceful() -> void;

    auto set_delay_server_migration_info(RcComm& _comm, std::string_view _leader, std::string_view _successor) -> void;
    auto get_delay_server_leader(RcComm& _comm) -> std::optional<std::string>;
    auto get_delay_server_successor(RcComm& _comm) -> std::optional<std::string>;
    auto launch_agent_factory(const char* _src_func, bool _write_to_stdout, bool _enable_test_mode) -> bool;
    auto handle_configuration_reload(bool _write_to_stdout, bool _enable_test_mode) -> void;
    auto launch_delay_server(bool _write_to_stdout, bool _enable_test_mode) -> void;
    auto get_preferred_host(const std::string_view _host) -> std::string;
    auto migrate_and_launch_delay_server(bool _write_to_stdout,
                                         bool _enable_test_mode,
                                         std::chrono::steady_clock::time_point& _time_start) -> void;
    auto log_stacktrace_files(std::chrono::steady_clock::time_point& _time_start) -> void;
    auto remove_leftover_agent_info_files_for_ips() -> void;

    auto init_access_time_queue() -> bool;
    auto apply_access_time_updates() -> void;

    auto is_server_listening_for_connections(const std::string& _host, const std::string& _port) -> int;
} // anonymous namespace

auto main(int _argc, char* _argv[]) -> int
{
    [[maybe_unused]] const auto boot_time = std::chrono::system_clock::now();

    bool write_to_stdout = false;
    bool enable_test_mode = false;

    ProcessType = SERVER_PT; // This process identifies itself as a server.

    set_ips_display_name("irodsServer");

    namespace po = boost::program_options;

    po::options_description opts_desc{""};

    // clang-format off
    opts_desc.add_options()
        ("daemonize,d", "")
        ("help,h", "")
        ("pid-file,p", po::value<std::string>(), "")
        ("stdout", po::bool_switch(&write_to_stdout), "")
        ("test-mode,t", po::bool_switch(&enable_test_mode), "")
        ("validate-config,c", "")
        ("version", "");
    // clang-format on

    try {
        po::variables_map vm;
        po::store(po::command_line_parser(_argc, _argv).options(opts_desc).run(), vm);
        po::notify(vm);

        if (vm.count("help") > 0) {
            print_usage();
            return 0;
        }

        if (vm.count("version") > 0) {
            print_version_info();
            return 0;
        }

        if (vm.count("validate-config") > 0) {
            return validate_configuration() ? 0 : 1;
        }

        if (!validate_configuration()) {
            return 1;
        }

        if (vm.count("daemonize") > 0) {
            if (write_to_stdout) {
                fmt::print(stderr, "Error: --daemonize and --stdout are incompatible.\n");
                return 1;
            }

            daemonize();
        }

        std::string pid_file = (irods::get_irods_runstate_directory() / "irods/irods-server.pid").string();
        if (const auto iter = vm.find("pid-file"); std::end(vm) != iter) {
            pid_file = std::move(iter->second.as<std::string>());
        }

        if (create_pid_file(pid_file) != 0) {
            fmt::print(stderr, "Error: could not create PID file [{}].\n", pid_file);
            return 1;
        }
    }
    catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }

    try {
        const auto config_file_path = irods::get_irods_config_directory() / "server_config.json";
        irods::server_properties::instance().init(config_file_path.c_str());

        // Configure the legacy rodsLog API so messages are written to the legacy log category
        // provided by the new logging API.
        rodsLogLevel(LOG_NOTICE);
        rodsLogSqlReq(0);

        init_logger(getpid(), write_to_stdout, enable_test_mode);

        // Setting up signal handlers here removes the need for reacting to shutdown signals
        // such as SIGINT and SIGTERM during the startup sequence.
        if (setup_signal_handlers() == -1) {
            log_server::error("{}: Error setting up signal handlers for main server process.", __func__);
            return 1;
        }

        if (wait_for_external_dependent_servers_to_start() != 0) {
            if (g_terminate || g_terminate_graceful) {
                return 0;
            }

            return 1;
        }

        if (const auto [up_to_date, db_vers] = check_catalog_schema_version(); !up_to_date) {
            const auto msg = fmt::format("Catalog schema version mismatch: expected [{}], found [{}] in catalog; Did "
                                         "you run 'python3 scripts/upgrade_irods.py'?",
                                         IRODS_CATALOG_SCHEMA_VERSION,
                                         db_vers);
            log_server::critical(msg);
            fmt::print(fmt::runtime(msg + '\n'));
            return 1;
        }

        log_server::info("{}: Initializing shared memory for main server process.", __func__);

        namespace hnc = irods::experimental::net::hostname_cache;
        hnc::init("irods_hostname_cache", irods::get_hostname_cache_shared_memory_size());
        irods::at_scope_exit deinit_hostname_cache{[] { hnc::deinit(); }};

        namespace dnsc = irods::experimental::net::dns_cache;
        dnsc::init("irods_dns_cache", irods::get_dns_cache_shared_memory_size());
        irods::at_scope_exit deinit_dns_cache{[] { dnsc::deinit(); }};

        // TODO(#8014): These directories should be created by the packaging.
        fs::create_directories(irods::get_irods_stacktrace_directory().c_str());
        fs::create_directories(irods::get_irods_proc_directory().c_str());

        // Load server API table so that API plugins which are needed to stand up the server are
        // available for use.
        auto& server_api_table = irods::get_server_api_table();
        auto& pack_table = irods::get_pack_table();
        if (const auto res = irods::init_api_table(server_api_table, pack_table, false); !res.ok()) {
            log_server::error("{}: {}", __func__, res.result());
            return 1;
        }

        // If this is a catalog service consumer, the client API table should be loaded so that
        // client calls can be made to the catalog service provider as part of the server
        // initialization process.
        auto& client_api_table = irods::get_client_api_table();
        if (const auto res = irods::init_api_table(client_api_table, pack_table, false); !res.ok()) {
            log_server::error("{}: {}", __func__, res.result());
            return 1;
        }

        // The access time queue must be initialized after the API tables are loaded.
        // This is required because the server may use the grid configuration API to fetch access
        // time configuration from the R_GRID_CONFIGURATION table.
        log_server::info("{}: Initializing access time queue for main server process.", __func__);
        if (!init_access_time_queue()) {
            return 1;
        }
        irods::at_scope_exit deinit_access_time_queue{[] { irods::access_time_queue::deinit(); }};

        if (!launch_agent_factory(__func__, write_to_stdout, enable_test_mode)) {
            return 1;
        }

        irods::notify_service_manager("READY=1");

        // Enter parent process main loop.
        //
        // This process should never introduce threads. Everything it cares about must be handled
        // within the loop. This keeps things simple and straight forward.
        //
        // THE PARENT PROCESS IS THE ONLY PROCESS THAT SHOULD/CAN REACT TO SIGNALS!
        // EVERYTHING IS PROPAGATED THROUGH/FROM THE PARENT PROCESS!

        // dsm = Short for delay server migration
        // This is used to control the frequency of the delay server migration logic.
        auto dsm_time_start = std::chrono::steady_clock::now();

        // stfp = Short for stacktrace file processor
        // This is used to control the frequency of the stacktrace file processing logic.
        auto stfp_time_start = dsm_time_start;

        while (true) {
            if (g_terminate) {
                log_server::info("{}: Received shutdown instruction. Exiting server main loop.", __func__);
                handle_shutdown();
                break;
            }

            if (g_terminate_graceful) {
                log_server::info("{}: Received graceful shutdown instruction. Exiting server main loop.", __func__);
                handle_shutdown_graceful();
                break;
            }

            if (g_reload_config) {
                handle_configuration_reload(write_to_stdout, enable_test_mode);
            }

            // Clean up any zombie child processes if they exist. These appear following a configuration
            // reload. We call waitpid() multiple times because the main server processes may have multiple
            // child processes.
            for (int i = 0; i < g_max_number_of_child_processes; ++i) {
                waitpid(-1, nullptr, WNOHANG);
            }

            log_stacktrace_files(stfp_time_start);
            remove_leftover_agent_info_files_for_ips();
            launch_agent_factory(__func__, write_to_stdout, enable_test_mode);
            migrate_and_launch_delay_server(write_to_stdout, enable_test_mode, dsm_time_start);
            apply_access_time_updates();

            std::this_thread::sleep_for(std::chrono::seconds{1});
        }

        log_server::info("{}: Server shutdown complete.", __func__);

        return 0;
    }
    catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
} // main

namespace
{
    auto print_usage() -> void
    {
        fmt::print(stderr,
                   R"__(irodsServer - Launch an iRODS server

Usage: irodsServer [OPTION]...

The Integrated Rule-Oriented Data System (iRODS) is open source data management
software used by research, commercial, and governmental organizations worldwide.

It virtualizes data storage resources, so users can take control of their data,
regardless of where and on what device the data is stored.

If launched without any options, the server will run in the foreground and write
activity to syslog.

Mandatory arguments to long options are mandatory for short options too.

Options:
  -c, --validate-config
                 Check the configuration file against the JSON schema and exit.
                 Returns 0 if the configuration file's structure is correct.
                 Returns 1 if the configuration file's structure is incorrect.
  -d, --daemonize
                 Run server instance in the background as a service. This option
                 is incompatible with --stdout.
  -h, --help     Display this help message and exit.
  -p, --pid-file FILE
                 The absolute path to FILE, which will be used as the PID file
                 for the server instance.
      --stdout   Write all log messages to stdout. This option is incompatible
                 with --daemonize.
  -t, --test-mode
                 Write log messages to normal location and
                 log/test_mode_output.log.
      --version  Display version information and exit.

Environment Variables:
  spLogSql       Set to 1 to log SQL generated by GenQuery1.

Signals:
  SIGTERM        Shutdown the server. Agents will complete the active request
                 and terminate.
  SIGQUIT        Shutdown the server gracefully. Agents will continue to
                 service requests for a limited amount of time. Once the time
                 limit is reached, agents will perform the normal shutdown
                 routine as if SIGTERM was received by the server.
  SIGINT         Same as SIGQUIT.
)__");
    } // print_usage

    auto print_version_info() -> void
    {
        constexpr const auto commit = std::string_view{IRODS_GIT_COMMIT}.substr(0, 7);
        fmt::print(
            "irodsServer v{}.{}.{}-{}\n", IRODS_VERSION_MAJOR, IRODS_VERSION_MINOR, IRODS_VERSION_PATCHLEVEL, commit);
    } // print_version_info

    auto validate_configuration() -> bool
    {
        try {
            namespace jsonschema = jsoncons::jsonschema;

            std::ifstream config_file{(irods::get_irods_config_directory() / "server_config.json").c_str()};
            if (!config_file) {
                return false;
            }
            const auto config = jsoncons::json::parse(config_file);

            // NOLINTNEXTLINE(bugprone-lambda-function-name)
            const auto do_validate = [fn = __func__](const auto& _config, const std::string& _schema_file) {
                const auto resolver = [](const jsoncons::uri& _uri) {
                    std::ifstream in{(irods::get_irods_home_directory() / _uri.path()).c_str()};
                    if (!in) {
                        return jsoncons::json::null();
                    }

                    return jsoncons::json::parse(in);
                };

                std::ifstream in{_schema_file};
                if (!in) {
                    return false;
                }
                const auto schema = jsoncons::json::parse(in); // The stream object cannot be instantiated inline.
                const auto compiled = jsonschema::make_json_schema(schema, resolver);

                jsoncons::json_decoder<jsoncons::ojson> decoder;
                compiled.validate(_config, decoder);
                const auto json_result = decoder.get_result();

                if (!json_result.empty()) {
                    std::ostringstream out;
                    out << pretty_print(json_result);

                    if (g_logger_initialized) {
                        log_server::error("{}: {}", fn, out.str());
                    }
                    else {
                        fmt::print(stderr, "{}: {}\n", fn, out.str());
                    }

                    return false;
                }

                return true;
            }; // do_validate

            // Validate the server configuration.
            const auto path_suffix = fmt::format("configuration_schemas/v{}", IRODS_CONFIGURATION_SCHEMA_VERSION);
            const auto schema_dir = irods::get_irods_home_directory() / path_suffix;
            const auto server_config_schema = schema_dir / "server_config.json";
            return do_validate(config, server_config_schema.c_str());
        }
        catch (const std::exception& e) {
            if (g_logger_initialized) {
                log_server::error("{}: {}", __func__, e.what());
            }
            else {
                fmt::print(stderr, "{}: {}\n", __func__, e.what());
            }
        }

        return false;
    } // validate_configuration

    inline auto daemonize_fork() -> void
    {
        pid_t child_pid = fork();

        if (child_pid > 0) {
            irods::notify_service_manager("MAINPID={}", child_pid);
            _exit(0);
        }

        if (child_pid < 0) {
            _exit(1);
        }
    } // daemonize_fork

    auto daemonize() -> void
    {
        // Become a background process.
        daemonize_fork();

        // Become session leader.
        if (setsid() == -1) {
            _exit(1);
        }

        // Make sure we aren't the session leader.
        daemonize_fork();

        umask(0);

        // Change the working directory to the root directory. Using the root directory is
        // common and protects the OS by allowing it to unmount file systems. If the working
        // directory of the daemon existed on a file system other than the one containing "/",
        // the OS would not be able to unmount it.
        //
        // It's also possible to change the working directory to other directories, for example,
        // the iRODS home directory. However, that directory normally exists under /var.
        chdir("/");

        // Get max number of open file descriptors.
        auto max_fd = sysconf(_SC_OPEN_MAX);
        if (-1 == max_fd) {
            // Indeterminate, so take a guess.
            max_fd = 8192;
        }

        // Close open file descriptors.
        for (auto fd = 0; fd < max_fd; ++fd) {
            close(fd);
        }

        // clang-format off
        constexpr auto fd_stdin  = 0;
        constexpr auto fd_stdout = 1;
        constexpr auto fd_stderr = 2;
        // clang-format on

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        auto fd = open("/dev/null", O_RDWR);
        if (fd_stdin != fd) {
            _exit(1);
        }

        if (dup2(fd, fd_stdout) != fd_stdout) {
            _exit(1);
        }

        if (dup2(fd, fd_stderr) != fd_stderr) {
            _exit(1);
        }
    } // daemonize

    auto create_pid_file(const std::string& _pid_file) -> int
    {
        // Open the PID file. If it does not exist, create it and give the owner
        // permission to read and write to it.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-signed-bitwise)
        const auto fd = open(_pid_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            fmt::print("Could not open PID file.\n");
            return 1;
        }

        // Get the current open flags for the open file descriptor.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        const auto flags = fcntl(fd, F_GETFD);
        if (flags == -1) {
            fmt::print("Could not retrieve open flags for PID file.\n");
            return 1;
        }

        // Enable the FD_CLOEXEC option for the open file descriptor.
        // This option will cause successful calls to exec() to close the file descriptor.
        // Keep in mind that record locks are NOT inherited by forked child processes.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-signed-bitwise)
        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
            fmt::print("Could not set FD_CLOEXEC on PID file.\n");
            return 1;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        struct flock input;
        input.l_type = F_WRLCK;
        input.l_whence = SEEK_SET;
        input.l_start = 0;
        input.l_len = 0;

        // Try to acquire the write lock on the PID file. If we cannot get the lock,
        // another instance of the application must already be running or something
        // weird is going on.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        if (fcntl(fd, F_SETLK, &input) == -1) {
            if (EAGAIN == errno || EACCES == errno) {
                fmt::print("Could not acquire write lock for PID file. Another instance "
                           "could be running already.\n");
                return 1;
            }
        }

        if (ftruncate(fd, 0) == -1) {
            fmt::print("Could not truncate PID file's contents.\n");
            return 1;
        }

        const auto contents = fmt::format("{}\n", getpid());
        // NOLINTNEXTLINE(google-runtime-int)
        if (write(fd, contents.data(), contents.size()) != static_cast<long>(contents.size())) {
            fmt::print("Could not write PID to PID file.\n");
            return 1;
        }

        return 0;
    } // create_pid_file

    auto init_logger(pid_t _pid, bool _write_to_stdout, bool _enable_test_mode) -> void
    {
        log_ns::init(_pid, _write_to_stdout, _enable_test_mode);
        log_server::set_level(log_ns::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_SERVER));
        log_ns::set_server_type("server");
        log_ns::set_server_zone(irods::get_server_property<std::string>(irods::KW_CFG_ZONE_NAME));
        log_ns::set_server_hostname(irods::get_server_property<std::string>(irods::KW_CFG_HOST));

        g_logger_initialized = true;
    } // init_logger

    auto check_catalog_schema_version() -> std::pair<bool, int>
    {
        try {
            const auto role = irods::get_server_property<std::string>(irods::KW_CFG_CATALOG_SERVICE_ROLE);

            if (role == irods::KW_CFG_SERVICE_ROLE_PROVIDER) {
                auto [db_instance, db_conn] = irods::experimental::catalog::new_database_connection();

                auto row = nanodbc::execute(db_conn,
                                            "select option_value from R_GRID_CONFIGURATION where namespace = "
                                            "'database' and option_name = 'schema_version'");
                if (!row.next()) {
                    return {false, -1};
                }

                const auto vers = row.get<int>(0);
                return {vers == IRODS_CATALOG_SCHEMA_VERSION, vers};
            }

            if (role == irods::KW_CFG_SERVICE_ROLE_CONSUMER) {
                return {true, IRODS_CATALOG_SCHEMA_VERSION};
            }
        }
        catch (const irods::exception& e) {
            log_server::error("{}: Could not verify catalog schema version: {}", __func__, e.client_display_what());
        }
        catch (const std::exception& e) {
            log_server::error("{}: Could not verify catalog schema version. Is the catalog service role defined in "
                              "server_config.json?",
                              __func__);
        }

        return {false, -1};
    } // check_catalog_schema_version

    auto setup_signal_handlers() -> int
    {
        // DO NOT memset sigaction structures!

        std::signal(SIGUSR1, SIG_IGN); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)

        // SIGINT
        struct sigaction sa_terminate; // NOLINT(cppcoreguidelines-pro-type-member-init)
        sigemptyset(&sa_terminate.sa_mask);
        sa_terminate.sa_flags = 0;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        sa_terminate.sa_handler = [](int) {
            // Only respond if the server hasn't been instructed to terminate already.
            if (0 == g_terminate_graceful) {
                g_terminate = 1;
            }
        };
        if (sigaction(SIGINT, &sa_terminate, nullptr) == -1) {
            return -1;
        }

        // SIGTERM
        if (sigaction(SIGTERM, &sa_terminate, nullptr) == -1) {
            return -1;
        }

        // SIGQUIT (graceful shutdown)
        struct sigaction sa_terminate_graceful; // NOLINT(cppcoreguidelines-pro-type-member-init)
        sigemptyset(&sa_terminate_graceful.sa_mask);
        sa_terminate_graceful.sa_flags = 0;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        sa_terminate_graceful.sa_handler = [](int) {
            // Only respond if the server hasn't been instructed to terminate already.
            if (0 == g_terminate) {
                g_terminate_graceful = 1;
            }
        };
        if (sigaction(SIGQUIT, &sa_terminate_graceful, nullptr) == -1) {
            return -1;
        }

        // SIGHUP
        struct sigaction sa_sighup; // NOLINT(cppcoreguidelines-pro-type-member-init)
        sigemptyset(&sa_sighup.sa_mask);
        sa_sighup.sa_flags = 0;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        sa_sighup.sa_handler = [](int) { g_reload_config = 1; };
        if (sigaction(SIGHUP, &sa_sighup, nullptr) == -1) {
            return -1;
        }

        irods::setup_unrecoverable_signal_handlers();

        return 0;
    } // setup_signal_handlers

    auto wait_for_external_dependent_servers_to_start() -> int
    {
        std::string role;
        try {
            role = irods::get_server_property<std::string>(irods::KW_CFG_CATALOG_SERVICE_ROLE);
        }
        catch (const irods::exception& e) {
            log_server::error("{}: Could not get catalog service role: {}", __func__, e.client_display_what());
        }
        catch (const std::exception& e) {
            log_server::error(
                "{}: Could not get catalog service role. Is the catalog service role defined in server_config.json?",
                __func__);
        }

        if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == role) {
            // Wait for the catalog to accept connections.
            while (true) {
                if (g_terminate) {
                    log_server::info("{}: Received shutdown instruction. Exiting server main loop.", __func__);
                    return -1;
                }

                if (g_terminate_graceful) {
                    log_server::info("{}: Received graceful shutdown instruction. Exiting server main loop.", __func__);
                    return -1;
                }

                try {
                    // Try to connect to the catalog. If the catalog isn't accepting connections or
                    // the server configuration is incorrect, this function will throw an exception.
                    irods::experimental::catalog::new_database_connection();

                    // Connection was successful. The catalog is ready.
                    return 0;
                }
                catch (const std::exception& e) {
                    log_server::debug("{}: Catalog not ready to accept connections.", __func__);
                }

                log_server::info("{}: Waiting for catalog to complete startup and accept connections.", __func__);
                std::this_thread::sleep_for(std::chrono::seconds{1});
            }
        }
        else if (irods::KW_CFG_SERVICE_ROLE_CONSUMER == role) {
            // Wait for the provider to accept connections.
            while (true) {
                log_server::debug("{}: Checking if Agent Factory is ready to accept client requests before launching "
                                  "the Delay Server.",
                                  __func__);

                if (g_terminate) {
                    log_server::info("{}: Received shutdown instruction. Exiting server main loop.", __func__);
                    return -1;
                }

                if (g_terminate_graceful) {
                    log_server::info("{}: Received graceful shutdown instruction. Exiting server main loop.", __func__);
                    return -1;
                }

                try {
                    const auto config_handle = irods::server_properties::instance().map();
                    const auto& config = config_handle.get_json();

                    const nlohmann::json::json_pointer json_path{"/catalog_provider_hosts/0"};
                    const auto provider_host = get_preferred_host(config.at(json_path).get_ref<const std::string&>());
                    const auto zone_port = config.at(irods::KW_CFG_ZONE_PORT).get<int>();
                    const auto zone_port_string = std::to_string(zone_port);

                    if (is_server_listening_for_connections(provider_host, zone_port_string) == 0) {
                        return 0;
                    }
                }
                catch (const std::exception& e) {
                    log_server::debug("{}: Catalog Provider not ready to accept connections.", __func__);
                }

                log_server::info(
                    "{}: Waiting for Catalog Provider to complete startup and accept connections.", __func__);
                std::this_thread::sleep_for(std::chrono::seconds{1});
            }
        }
        else {
            log_server::error(
                "{}: Invalid catalog service role: expected [provider] or [consumer], found [{}]", __func__, role);
        }

        return -1;
    } // wait_for_external_dependent_servers_to_start

    auto handle_shutdown() -> void
    {
        irods::notify_service_manager("STOPPING=1");

        kill(g_pid_af, SIGTERM);

        if (g_pid_ds > 0) {
            kill(g_pid_ds, SIGTERM);
        }

        waitpid(g_pid_af, nullptr, 0);
        log_server::info("{}: Agent Factory shutdown complete.", __func__);

        if (g_pid_ds > 0) {
            waitpid(g_pid_ds, nullptr, 0);
            log_server::info("{}: Delay Server shutdown complete.", __func__);
        }
    } // handle_shutdown

    auto handle_shutdown_graceful() -> void
    {
        irods::notify_service_manager("STOPPING=1");

        kill(g_pid_af, SIGQUIT);

        if (g_pid_ds > 0) {
            kill(g_pid_ds, SIGTERM);
        }

        waitpid(g_pid_af, nullptr, 0);
        log_server::info("{}: Agent Factory shutdown complete.", __func__);

        if (g_pid_ds > 0) {
            waitpid(g_pid_ds, nullptr, 0);
            log_server::info("{}: Delay Server shutdown complete.", __func__);
        }
    } // handle_shutdown_graceful

    auto get_delay_server_leader(RcComm& _comm) -> std::optional<std::string>
    {
        GridConfigurationInput input{};
        std::strcpy(input.name_space, "delay_server");
        std::strcpy(input.option_name, "leader");

        GridConfigurationOutput* output{};
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
        irods::at_scope_exit free_output{[&output] { std::free(output); }};

        if (const auto ec = rc_get_grid_configuration_value(&_comm, &input, &output); ec < 0) {
            log_server::error("Could not retrieve delay server migration information from catalog "
                              "[error_code={}, namespace=delay_server, option_name=leader].",
                              ec);
            return std::nullopt;
        }

        return output->option_value;
    } // get_delay_server_leader

    auto get_delay_server_successor(RcComm& _comm) -> std::optional<std::string>
    {
        GridConfigurationInput input{};
        std::strcpy(input.name_space, "delay_server");
        std::strcpy(input.option_name, "successor");

        GridConfigurationOutput* output{};
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
        irods::at_scope_exit free_output{[&output] { std::free(output); }};

        if (const auto ec = rc_get_grid_configuration_value(&_comm, &input, &output); ec < 0) {
            log_server::error("Could not retrieve delay server migration information from catalog "
                              "[error_code={}, namespace=delay_server, option_name=successor].",
                              ec);
            return std::nullopt;
        }

        return output->option_value;
    } // get_delay_server_successor

    auto set_delay_server_migration_info(RcComm& _comm, std::string_view _leader, std::string_view _successor) -> void
    {
        DelayServerMigrationInput input{};
        _leader.copy(input.leader, sizeof(DelayServerMigrationInput::leader) - 1);
        _successor.copy(input.successor, sizeof(DelayServerMigrationInput::successor) - 1);

        if (const auto ec = rc_set_delay_server_migration_info(&_comm, &input); ec < 0) {
            log_server::error("Failed to set delay server migration info in R_GRID_CONFIGURATION "
                              "[error_code={}, leader={}, successor={}].",
                              ec,
                              _leader,
                              _successor);
        }
    } // set_delay_server_migration_info

    auto launch_agent_factory(const char* _src_func, bool _write_to_stdout, bool _enable_test_mode) -> bool
    {
        auto launch = (0 == g_pid_af);

        if (g_pid_af > 0) {
            if (const auto ec = kill(g_pid_af, 0); ec == -1) {
                if (EPERM == errno || ESRCH == errno) {
                    launch = true;
                    g_pid_af = 0;
                }
            }
        }

        if (!launch) {
            return false;
        }

        log_server::info("{}: Launching Agent Factory.", _src_func);

        // If we're planning on calling one of the functions from the exec-family,
        // then we're only allowed to use async-signal-safe functions following the call
        // to fork(). To avoid potential issues, we build up the argument list before
        // doing the fork-exec.

        auto binary = (irods::get_irods_sbin_directory() / "irodsAgent").string();
        std::string hn_shm_name{irods::experimental::net::hostname_cache::shared_memory_name()};
        std::string dns_shm_name{irods::experimental::net::dns_cache::shared_memory_name()};
        std::string atime_queue_shm_name{irods::access_time_queue::shared_memory_name()};

        // The access time resolution requires special care because it must accept negative integer
        // values. However, Boost.Program_options will not accept negative integer values as positional
        // arguments because it treats anything with a leading hyphen as an option name. To get around
        // this, we have to pass the access time resolution via a non-positional option.
        std::string atime_res_opt = "--atime-resolution";

        std::vector<char*> args{binary.data(),
                                hn_shm_name.data(),
                                dns_shm_name.data(),
                                atime_queue_shm_name.data(),
                                atime_res_opt.data(),
                                g_atime_resolution_in_seconds.data()};

        std::string stdout_opt = "--stdout";
        if (_write_to_stdout) {
            args.push_back(stdout_opt.data());
        }

        std::string test_mode_opt = "--test-mode";
        if (_enable_test_mode) {
            args.push_back(test_mode_opt.data());
        }

        args.push_back(nullptr);

        g_pid_af = fork();

        if (0 == g_pid_af) {
            execv(args[0], args.data());

            // If execv() fails, the POSIX standard recommends using _exit() instead of exit() to avoid
            // flushing stdio buffers and handlers registered by the parent.
            //
            // In the case of C++, this is necessary to avoid triggering destructors. Triggering a destructor
            // could result in assertions made by the struct/class being violatied. For some data types,
            // violating an assertion results in program termination (i.e. SIGABRT).
            _exit(1);
        }
        else if (-1 == g_pid_af) {
            g_pid_af = 0;
            log_server::error("{}: Could not launch agent factory.", __func__);
            return false;
        }

        log_server::info("{}: Agent Factory PID = [{}].", __func__, g_pid_af);
        return true;
    } // launch_agent_factory

    auto handle_configuration_reload(bool _write_to_stdout, bool _enable_test_mode) -> void
    {
        irods::at_scope_exit reset_reload_flag{[] { g_reload_config = 0; }};

        irods::notify_service_manager("RELOADING=1\nMONOTONIC_USEC={}", irods::get_monotonic_usec());

        log_server::info("{}: Received configuration reload instruction. Reloading configuration.", __func__);

        if (!validate_configuration()) {
            log_server::error("{}: Invalid configuration. Server will continue with existing configuration.", __func__);
            irods::notify_service_manager("READY=1\nSTATUS=Reload failed due to invalid configuration");
            return;
        }

        nlohmann::json previous_server_config;

        try {
            log_server::info("{}: Creating backup of server configuration.", __func__);
            previous_server_config = irods::server_properties::copy_configuration();
        }
        catch (const irods::exception& e) {
            log_server::error("{}: Reload error: {}. Server will continue with existing configuration.",
                              __func__,
                              e.client_display_what());
            irods::notify_service_manager("READY=1\nSTATUS=Reload failed");
            return;
        }
        catch (const std::exception& e) {
            log_server::error(
                "{}: Reload error: {}. Server will continue with existing configuration.", __func__, e.what());
            irods::notify_service_manager("READY=1\nSTATUS=Reload failed");
            return;
        }

        try {
            log_server::info("{}: Reloading server configuration.", __func__);
            irods::server_properties::instance().reload();
        }
        catch (const irods::exception& e) {
            log_server::error("{}: Reload error: {}. Server will continue with existing configuration.",
                              __func__,
                              e.client_display_what());
            irods::server_properties::instance().set_configuration(std::move(previous_server_config));
            irods::notify_service_manager("READY=1\nSTATUS=Reload failed");
            return;
        }
        catch (const std::exception& e) {
            log_server::error(
                "{}: Reload error: {}. Server will continue with existing configuration.", __func__, e.what());
            irods::server_properties::instance().set_configuration(std::move(previous_server_config));
            irods::notify_service_manager("READY=1\nSTATUS=Reload failed");
            return;
        }

        //
        // At this point, the new configuration has been read into memory. Now the server must update
        // its state and replace its child processes.
        //

        try {
            // Update the logger for the main server process.
            log_server::set_level(log_ns::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_SERVER));
            log_ns::legacy::set_level(log_ns::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_LEGACY));
            log_ns::set_server_zone(irods::get_server_property<std::string>(irods::KW_CFG_ZONE_NAME));
            log_ns::set_server_hostname(irods::get_server_property<std::string>(irods::KW_CFG_HOST));
        }
        catch (const irods::exception& e) {
            log_server::warn("{}: Could not update the logger's state: {}. Continuing with reload.",
                             __func__,
                             e.client_display_what());
        }
        catch (const std::exception& e) {
            log_server::warn(
                "{}: Could not update the logger's state: {}. Continuing with reload.", __func__, e.what());
        }

        // Get the host and port information from the server configuration before stopping the agent factory
        // and delay server. Doing this before stopping the child processes protects the main server from
        // exceptions being thrown when extracting the host and zone port from the configuration.
        std::string local_server_host;
        std::string local_server_port_string;
        try {
            // The host property in server_config.json defines the true identity of the local server.
            // We cannot use localhost or the loopback address because the computer may have multiple
            // network interfaces and/or hostnames which map to different IPs.
            local_server_host = irods::get_server_property<std::string>(irods::KW_CFG_HOST);

            // Use the zone port property from server_config.json. This property defines the port for
            // server-to-server connections within the zone.
            const auto local_server_port = irods::get_server_property<int>(irods::KW_CFG_ZONE_PORT);
            local_server_port_string = std::to_string(local_server_port);
        }
        catch (const irods::exception& e) {
            log_server::error("{}: Reload error: {}", __func__, e.client_display_what());
            irods::notify_service_manager("READY=1\nSTATUS=Reload error");
            return;
        }
        catch (const std::exception& e) {
            log_server::error("{}: Reload error: {}", __func__, e.what());
            irods::notify_service_manager("READY=1\nSTATUS=Reload error");
            return;
        }

        if (g_pid_ds > 0) {
            log_server::info("{}: Sending SIGTERM to delay server.", __func__);
            kill(g_pid_ds, SIGTERM);
        }

        log_server::info("{}: Sending SIGQUIT to agent factory.", __func__);
        kill(g_pid_af, SIGQUIT);

        // Reset the variable holding the delay server's PID so the delay server migration logic can handle
        // the relaunching of the delay server for us.
        g_pid_ds = 0;

        // Wait for the previous agent factory to close its listening socket before launching
        // the new agent factory.
        while (true) {
            // If this loop cannot make process for some reason, allow the admin to stop the server
            // without needing SIGKILL.
            if (g_terminate || g_terminate_graceful) {
                log_server::info("{}: Received shutdown instruction. Ending reload operation.", __func__);
                return;
            }

            try {
                if (is_server_listening_for_connections(local_server_host, local_server_port_string) == 0) {
                    log_server::info("{}: Waiting for previous agent factory to close its listening socket.", __func__);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }

                break;
            }
            catch (const irods::exception& e) {
                log_server::debug(
                    "{}: Unexpected error while waiting for previous agent factory to close its listening socket: {}",
                    __func__,
                    e.client_display_what());
            }
            catch (const std::exception& e) {
                log_server::debug(
                    "{}: Unexpected error while waiting for previous agent factory to close its listening socket: {}",
                    __func__,
                    e.what());
            }
        }

        // Launch a new agent factory to serve client requests.
        // The previous agent factory is allowed to linger around until its children terminate.
        launch_agent_factory(__func__, _write_to_stdout, _enable_test_mode);

        // We do not need to manually launch the delay server because the delay server migration
        // logic will handle that for us.

        irods::notify_service_manager("READY=1\nSTATUS=Configuration reloaded");
    } // handle_configuration_reload

    auto launch_delay_server(bool _write_to_stdout, bool _enable_test_mode) -> void
    {
        auto launch = (0 == g_pid_ds);

        if (g_pid_ds > 0) {
            if (const auto ec = kill(g_pid_ds, 0); ec == -1) {
                if (EPERM == errno || ESRCH == errno) {
                    launch = true;
                    g_pid_ds = 0;
                }
            }
        }

        if (launch) {
            log_server::info("{}: Launching Delay Server.", __func__);

            // If we're planning on calling one of the functions from the exec-family,
            // then we're only allowed to use async-signal-safe functions following the call
            // to fork(). To avoid potential issues, we build up the argument list before
            // doing the fork-exec.

            auto binary = (irods::get_irods_sbin_directory() / "irodsDelayServer").string();
            std::string hn_shm_name{irods::experimental::net::hostname_cache::shared_memory_name()};
            std::string dns_shm_name{irods::experimental::net::dns_cache::shared_memory_name()};

            std::vector<char*> args{binary.data(), hn_shm_name.data(), dns_shm_name.data()};

            std::string stdout_opt = "--stdout";
            if (_write_to_stdout) {
                args.push_back(stdout_opt.data());
            }

            std::string test_mode_opt = "--test-mode";
            if (_enable_test_mode) {
                args.push_back(test_mode_opt.data());
            }

            args.push_back(nullptr);

            g_pid_ds = fork();

            if (0 == g_pid_ds) {
                execv(args[0], args.data());

                // If execv() fails, the POSIX standard recommends using _exit() instead of exit() to avoid
                // flushing stdio buffers and handlers registered by the parent.
                //
                // In the case of C++, this is necessary to avoid triggering destructors. Triggering a destructor
                // could result in assertions made by the struct/class being violatied. For some data types,
                // violating an assertion results in program termination (i.e. SIGABRT).
                _exit(1);
            }
            else if (g_pid_ds > 0) {
                log_server::info("{}: Delay Server PID = [{}].", __func__, g_pid_ds);
            }
            else {
                log_server::error("{}: Could not launch delay server [errno={}].", __func__, errno);
                g_pid_ds = 0;
            }
        }
    } // launch_delay_server

    auto get_preferred_host(const std::string_view _host) -> std::string
    {
        if (const auto resolved = resolve_hostname(_host, hostname_resolution_scheme::match_preferred); resolved) {
            return *resolved;
        }

        return std::string{_host};
    } // get_preferred_host

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    auto migrate_and_launch_delay_server(bool _write_to_stdout,
                                         bool _enable_test_mode,
                                         std::chrono::steady_clock::time_point& _time_start) -> void
    {
        // The host property in server_config.json defines the true identity of the local server.
        // We cannot use localhost or the loopback address because the computer may have multiple
        // network interfaces and/or hostnames which map to different IPs.
        const auto local_server_host = irods::get_server_property<std::string>(irods::KW_CFG_HOST);

        if (0 == g_pid_ds) {
            log_server::debug(
                "{}: Checking if Agent Factory is ready to accept client requests before launching the Delay Server.",
                __func__);

            // Defer the launch of the delay server if the agent factory isn't listening.
            try {
                // Use the zone port property from server_config.json. This property defines the port for
                // server-to-server connections within the zone.
                const auto local_server_port = irods::get_server_property<int>(irods::KW_CFG_ZONE_PORT);
                const auto local_server_port_string = std::to_string(local_server_port);

                if (is_server_listening_for_connections(local_server_host, local_server_port_string) != 0) {
                    return;
                }
            }
            catch (const irods::exception& e) {
                log_server::debug(
                    "{}: Connect Error: {}. Deferring launch of Delay Server.", __func__, e.client_display_what());
                return;
            }
            catch (const std::exception& e) {
                log_server::debug("{}: Connect Error: {}. Deferring launch of Delay Server.", __func__, e.what());
                return;
            }
        }

        const auto migration_sleep_time_in_seconds =
            irods::get_advanced_setting<int>(irods::KW_CFG_MIGRATE_DELAY_SERVER_SLEEP_TIME_IN_SECONDS);
        const auto now = std::chrono::steady_clock::now();

        if (now - _time_start < std::chrono::seconds{migration_sleep_time_in_seconds}) {
            return;
        }

        _time_start = now;

        std::optional<std::string> leader;
        std::optional<std::string> successor;

        try {
            irods::experimental::client_connection conn;
            leader = get_delay_server_leader(conn);
            successor = get_delay_server_successor(conn);

            if (leader && successor) {
                log_server::debug("{}: Delay server leader [{}] and successor [{}].", __func__, *leader, *successor);

                // Update the host information based on the "host_resolution" configuration, if defined.
                leader = get_preferred_host(*leader);
                successor = get_preferred_host(*successor);

                // 4 cases
                // ----------------------------------------------------------------------------
                // C  L  S  R
                // ----------------------------------------------------------------------------
                // 0  0  0  Invalid state / No-op
                // 1  1  0  Delay server migration complete
                // 2  0  1  Leader has shut down delay server, Successor launching delay server
                // 3  1  1  Leader running delay server, Admin requested migration
                // ----------------------------------------------------------------------------
                // C = Case, L = Leader, S = Successor, R = Result

                // This server is the leader and may be running a delay server.
                if (local_server_host == *leader) {
                    // Case 3
                    if (local_server_host == *successor) {
                        launch_delay_server(_write_to_stdout, _enable_test_mode);

                        // Clear successor entry in catalog. This isn't necessary, but helps
                        // keep the admin from becoming confused.
                        if (g_pid_ds > 0) {
                            set_delay_server_migration_info(conn, KW_DELAY_SERVER_MIGRATION_IGNORE, "");
                        }
                    }
                    // Case 3
                    else if (!successor->empty()) {
                        // Migration requested. Stop local delay server if running and clear the
                        // leader entry in the catalog.
                        if (g_pid_ds > 0) {
                            kill(g_pid_ds, SIGTERM);

                            // Wait for delay server to complete current tasks. Depending on the workload, this
                            // could take minutes, hours, days to complete.
                            int status = 0;
                            waitpid(g_pid_ds, &status, 0);
                            log_server::info(
                                "Delay server has completed shutdown [exit_code={}].", WEXITSTATUS(status));
                            g_pid_ds = 0;
                        }
                    }
                    // Case 1
                    else {
                        launch_delay_server(_write_to_stdout, _enable_test_mode);
                    }
                }
                // Case 2
                else if (local_server_host == *successor) {
                    // leader == successor (i.e. case 3) is covered by the first if-branch.

                    // Delay servers lock tasks before execution. This allows the successor server
                    // to launch a delay server without duplicating work.
                    launch_delay_server(_write_to_stdout, _enable_test_mode);

                    if (g_pid_ds > 0) {
                        set_delay_server_migration_info(conn, local_server_host, "");
                    }
                }
            }
        }
        catch (const irods::exception& e) {
            log_server::error("{}: {}", __func__, e.client_display_what());
        }
        catch (const std::exception& e) {
            log_server::error("{}: {}", __func__, e.what());
        }
    } // migrate_and_launch_delay_server

    auto log_stacktrace_files(std::chrono::steady_clock::time_point& _time_start) -> void
    {
        const auto timeout =
            irods::get_advanced_setting<int>(irods::KW_CFG_STACKTRACE_FILE_PROCESSOR_SLEEP_TIME_IN_SECONDS);
        const auto now = std::chrono::steady_clock::now();

        if (now - _time_start < std::chrono::seconds{timeout}) {
            return;
        }

        _time_start = now;

        for (auto&& entry : fs::directory_iterator{irods::get_irods_stacktrace_directory().c_str()}) {
            // Expected filename format:
            //
            //     <epoch_seconds>.<epoch_milliseconds>.<agent_pid>
            //
            // 1. Extract the timestamp from the filename and convert it to ISO8601 format.
            // 2. Extract the agent pid from the filename.
            const auto p = entry.path().generic_string();

            if (p.ends_with(irods::STACKTRACE_NOT_READY_FOR_LOGGING_SUFFIX)) {
                log_server::trace("Skipping [{}].", p);
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
            log_server::trace("epoch seconds = [{}], remaining millis = [{}], agent pid = [{}]",
                              epoch_seconds,
                              remaining_millis,
                              pid);

            try {
                // Convert the epoch value to ISO8601 format.
                log_server::trace("Converting epoch seconds to UTC timestamp.");
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
                // clang-format off
                irods::experimental::log::server::critical({
                    {"log_message", boost::stacktrace::to_string(stacktrace)},
                    {"stacktrace_agent_pid", pid},
                    {"stacktrace_timestamp_utc", fmt::format("{}.{}Z", utc_ss.str(), remaining_millis)},
                    {"stacktrace_timestamp_epoch_seconds", epoch_seconds},
                    {"stacktrace_timestamp_epoch_milliseconds", remaining_millis}
                });
                // clang-format on

                // 4. Delete the stacktrace file.
                //
                // We don't want the stacktrace files to go away without making it into the log.
                // We can't rely on the log invocation above because of syslog.
                // We don't want these files to accumulate for long running servers.
                log_server::trace("Removing stacktrace file from disk.");
                fs::remove(entry);
            }
            catch (...) {
                // Something happened while logging the stacktrace file.
                // Leaving the stacktrace file in-place for processing later.
                log_server::trace("Caught exception while processing stacktrace file.");
            }
        }
    } // log_stacktrace_files

    auto remove_leftover_agent_info_files_for_ips() -> void
    {
        for (const auto& entry : fs::directory_iterator{irods::get_irods_proc_directory().c_str()}) {
            try {
                const auto agent_pid = std::stoi(entry.path().stem().string());

                // If the agent process does not exist or the main server process doesn't
                // have permission to send signals to the agent process, then remove the
                // agent file so that ips doesn't report it as an active agent.
                if (kill(agent_pid, 0) == -1 && (ESRCH == errno || EPERM == errno)) {
                    fs::remove(entry);
                }
            }
            catch (const std::exception& e) {
                log_server::error("{}: {}: {}", __func__, entry.path().c_str(), e.what());
            }
        };
    } // remove_leftover_agent_info_files_for_ips

    auto init_access_time_queue() -> bool
    {
        try {
            //
            // Retrieve access time configuration from the catalog.
            //

            std::optional<std::string> queue_name_prefix;
            std::optional<std::string> queue_size;
            std::optional<std::string> batch_size;
            std::optional<std::string> resolution_in_seconds;

            const auto role = irods::get_server_property<std::string>(irods::KW_CFG_CATALOG_SERVICE_ROLE);

            // Initialization of the access time queue happens before the agent factory is available.
            // This means we cannot rely on a local iRODS connection to retrieve the access time configuration
            // from the database.

            if (role == irods::KW_CFG_SERVICE_ROLE_PROVIDER) {
                // Catalog Service Providers are required to have database credentials, so all we have to do
                // to get around the lack of an agent factory is use nanodbc to interact with the database
                // directly.

                auto [db_instance, db_conn] = irods::experimental::catalog::new_database_connection();
                auto row = execute(
                    db_conn,
                    "select option_name, option_value from R_GRID_CONFIGURATION where namespace = 'access_time'");

                while (row.next()) {
                    const auto opt_name = row.get<std::string>(0);

                    if (opt_name == irods::KW_CFG_ACCESS_TIME_QUEUE_NAME_PREFIX) {
                        queue_name_prefix = row.get<std::string>(1);
                    }
                    else if (opt_name == irods::KW_CFG_ACCESS_TIME_QUEUE_SIZE) {
                        queue_size = row.get<std::string>(1);
                    }
                    else if (opt_name == irods::KW_CFG_ACCESS_TIME_BATCH_SIZE) {
                        batch_size = row.get<std::string>(1);
                    }
                    else if (opt_name == irods::KW_CFG_ACCESS_TIME_RESOLUTION_IN_SECONDS) {
                        resolution_in_seconds = row.get<std::string>(1);
                    }
                }
            }
            else if (role == irods::KW_CFG_SERVICE_ROLE_CONSUMER) {
                // Catalog Service Consumers aren't guaranteed to have database credentials. We work around
                // the lack of an agent factory and database credentials by connecting to a Catalog Service
                // Provider directly and invoking the grid configuration API. This works because consumer
                // servers are designed to wait for the provider to become available before continuing with
                // server initialization.

                rodsEnv env;
                _getRodsEnv(env);

                const auto config_handle = irods::server_properties::instance().map();
                const auto& config = config_handle.get_json();

                const nlohmann::json::json_pointer json_path{"/catalog_provider_hosts/0"};
                const auto provider_host = get_preferred_host(config.at(json_path).get_ref<const std::string&>());
                const auto zone_port = config.at(irods::KW_CFG_ZONE_PORT).get<int>();

                irods::experimental::client_connection conn{provider_host, zone_port, {env.rodsUserName, env.rodsZone}};

                GridConfigurationInput input{};
                std::strcpy(input.name_space, irods::KW_CFG_ACCESS_TIME);

                GridConfigurationOutput* output{};

                const auto opt_names = {
                    std::pair{irods::KW_CFG_ACCESS_TIME_QUEUE_NAME_PREFIX, &queue_name_prefix},
                    std::pair{irods::KW_CFG_ACCESS_TIME_QUEUE_SIZE, &queue_size},
                    std::pair{irods::KW_CFG_ACCESS_TIME_BATCH_SIZE, &batch_size},
                    std::pair{irods::KW_CFG_ACCESS_TIME_RESOLUTION_IN_SECONDS, &resolution_in_seconds}};

                for (auto&& opt : opt_names) {
                    std::strcpy(input.option_name, opt.first);

                    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
                    irods::at_scope_exit free_output{[&output] { std::free(output); }};

                    const auto ec = rc_get_grid_configuration_value(static_cast<RcComm*>(conn), &input, &output);
                    if (ec < 0) {
                        log_server::error("{}: Could not retrieve grid configuration value from catalog "
                                          "[error_code={}, namespace=access_time, option_name={}].",
                                          __func__,
                                          ec,
                                          opt.first);

                        // We break out of the for-loop instead of returning to allow more error
                        // information to be logged and so that the consumer branch is closer in behavior
                        // to the provider branch.
                        break;
                    }

                    *opt.second = output->option_value;
                }
            }

            //
            // At this point, we expect to have all access time configuration values available.
            //

            if (!queue_name_prefix) {
                log_server::error("{}: Failed to initialize access time queue: Is [access_time.queue_name_prefix] "
                                  "defined in R_GRID_CONFIGURATION?",
                                  __func__);
                return false;
            }

            if (!queue_size) {
                log_server::error("{}: Failed to initialize access time queue: Is [access_time.queue_size] defined in "
                                  "R_GRID_CONFIGURATION?",
                                  __func__);
                return false;
            }

            if (!batch_size) {
                log_server::error("{}: Failed to initialize access time queue: Is [access_time.batch_size] defined in "
                                  "R_GRID_CONFIGURATION?",
                                  __func__);
                return false;
            }

            if (!resolution_in_seconds) {
                log_server::error("{}: Failed to initialize access time queue: Is [access_time.resolution_in_seconds] "
                                  "defined in R_GRID_CONFIGURATION?",
                                  __func__);
                return false;
            }

            irods::access_time_queue::init(*queue_name_prefix, std::stoi(*queue_size));

            // Verify the batch size value is acceptable.
            try {
                if (batch_size->empty()) {
                    throw std::exception{};
                }

                const auto value = std::stoll(*batch_size);
                // Limited to 32-bit values.
                if (value <= 0 || value > std::numeric_limits<std::int32_t>::max()) {
                    throw std::exception{};
                }

                g_atime_batch_size = value;
            }
            catch (const std::exception&) {
                g_atime_invalid_batch_size_detected = true;
                g_atime_batch_size = 20000; // Use default.
            }

            // Store configuration values in global variables for later use.
            g_atime_resolution_in_seconds = *std::move(resolution_in_seconds);

            return true;
        }
        catch (const irods::exception& e) {
            log_server::error("{}: Failed to initialize access time queue: {}", __func__, e.client_display_what());
        }
        catch (const std::exception& e) {
            log_server::error("{}: Failed to initialize access time queue: {}", __func__, e.what());
        }

        return false;
    } // init_access_time_queue

    auto apply_access_time_updates() -> void
    {
        try {
            log_server::debug("{}: Checking if Agent Factory is ready to accept client requests before processing "
                              "access time updates.",
                              __func__);

            // Defer processing of the access time queue if the agent factory isn't listening.
            try {
                // The host property in server_config.json defines the true identity of the local server.
                // We cannot use localhost or the loopback address because the computer may have multiple
                // network interfaces and/or hostnames which map to different IPs.
                const auto local_server_host = irods::get_server_property<std::string>(irods::KW_CFG_HOST);

                // Use the zone port property from server_config.json. This property defines the port for
                // server-to-server connections within the zone.
                const auto local_server_port = irods::get_server_property<int>(irods::KW_CFG_ZONE_PORT);
                const auto local_server_port_string = std::to_string(local_server_port);

                if (is_server_listening_for_connections(local_server_host, local_server_port_string) != 0) {
                    return;
                }
            }
            catch (const irods::exception& e) {
                log_server::debug("{}: Connect Error: {}. Deferring processing of access time updates.",
                                  __func__,
                                  e.client_display_what());
                return;
            }
            catch (const std::exception& e) {
                log_server::debug(
                    "{}: Connect Error: {}. Deferring processing of access time updates.", __func__, e.what());
                return;
            }

            const auto start_time = std::chrono::steady_clock::now();

            irods::experimental::client_connection conn;
            irods::access_time_queue::access_time_data data;
            nlohmann::json::array_t updates;

            if (g_atime_invalid_batch_size_detected) {
                log_server::warn("{}: Invalid batch size for access time updates detected in grid configuration. Using "
                                 "default batch size of 20000.",
                                 __func__);
            }

            // Allow the admin to limit the number of atime updates in case the queue size is very large.
            const auto update_count =
                std::min(irods::access_time_queue::number_of_queued_updates(), g_atime_batch_size);
            log_server::debug(
                "{}: Number of access time updates before deduplication is [{}].", __func__, update_count);

            for (std::size_t i = 0; i < update_count; ++i) {
                if (!irods::access_time_queue::try_dequeue(data)) {
                    break;
                }

                // clang-format off
                updates.push_back(nlohmann::json{
                    {"data_id", data.data_id},
                    {"replica_number", data.replica_number},
                    {"atime", data.last_accessed}
                });
                // clang-format on
            }

            if (updates.empty()) {
                return;
            }

            // Group elements by data id and replica number in descending order.
            std::sort(std::begin(updates), std::end(updates), [](const auto& _lhs, const auto& _rhs) {
                const auto& lhs_data_id = _lhs.at("data_id");
                const auto& rhs_data_id = _rhs.at("data_id");
                if (lhs_data_id > rhs_data_id) {
                    return true;
                }

                if (lhs_data_id == rhs_data_id) {
                    const auto& lhs_replica_number = _lhs.at("replica_number");
                    const auto& rhs_replica_number = _rhs.at("replica_number");
                    if (lhs_replica_number > rhs_replica_number) {
                        return true;
                    }

                    if (lhs_replica_number == rhs_replica_number) {
                        return _lhs.at("atime") > _rhs.at("atime");
                    }
                }

                return false;
            });

            // Reduce updates such that each replica is updated once. On completion, the JSON
            // is guaranteed to contain only the most recent update for each replica.
            auto last = std::unique(std::begin(updates), std::end(updates), [](const auto& _lhs, const auto& _rhs) {
                return _lhs.at("data_id") == _rhs.at("data_id") &&
                       _lhs.at("replica_number") == _rhs.at("replica_number");
            });
            updates.erase(last, std::end(updates));
            log_server::debug(
                "{}: Deduplication complete. Access time updates reduced to [{}].", __func__, updates.size());

            // Apply updates.
            const auto json_input = nlohmann::json{{"access_time_updates", updates}}.dump();
            char* ignored{};
            const auto ec = rc_update_replica_access_time(static_cast<RcComm*>(conn), json_input.c_str(), &ignored);
            log_server::debug("{}: rc_update_replica_access_time returned [{}].", __func__, ec);

            const auto elapsed = std::chrono::steady_clock::now() - start_time;
            log_server::debug("{}: Access time updates took [{}] seconds to apply.",
                              __func__,
                              std::chrono::duration<double>(elapsed).count());
        }
        catch (const irods::exception& e) {
            log_server::error(
                "{}: Caught exception while processing access time updates: {}", __func__, e.client_display_what());
        }
        catch (const std::exception& e) {
            log_server::error("{}: Caught exception while processing access time updates: {}", __func__, e.what());
        }
    } // apply_access_time_updates

    // TODO(#8015): This can be used to check if the server running the old delay server leader is running.
    // The idea is that this gives us a strong guarantee around delay server migration and leader promotion.
    // The successor would use this to contact the server which was running the old delay server. The successor
    // could use this to determine if the leader was still responsive.
    auto is_server_listening_for_connections(const std::string& _host, const std::string& _port) -> int
    {
        boost::asio::ip::tcp::iostream stream;
        try {
            log_server::debug("{}: Connecting to (host, port) = ([{}], [{}])", __func__, _host, _port);
            stream.connect(_host, _port);
            if (!stream) {
                return -1;
            }
        }
        catch (const std::exception& e) {
            return -1;
        }
        log_server::debug("{}: Connected to server. Sending HEARTBEAT message.", __func__);

        // Send heartbeat to the agent to avoid a noisy log. This is important for startup
        // purposes. Sending the heartbeat message instructs the agent to end the server-side
        // connection cleanly.
        // NOLINTNEXTLINE(bugprone-string-literal-with-embedded-nul)
        stream.write("\x00\x00\x00\x33<MsgHeader_PI><type>HEARTBEAT</type></MsgHeader_PI>", 55);
        stream.flush();

        log_server::debug("{}: Reading response from server.", __func__);
        std::array<char, 256> buffer{};
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::string_view msg(buffer.data(), stream.gcount());
        log_server::debug("{}: Received [{}] from server.", __func__, msg);
        if (msg != "HEARTBEAT") {
            log_server::debug("{}: Heartbeat Error: Did not get expected response.", __func__);
            return -1;
        }

        return 0;
    } // is_server_listening_for_connections
} // anonymous namespace
