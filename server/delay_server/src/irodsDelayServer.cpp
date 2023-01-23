#include "irods/irodsDelayServer.hpp"

#include "irods/client_connection.hpp"
#include "irods/connection_pool.hpp"
#include "irods/get_delay_rule_info.h"
#include "irods/initServer.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_client_api_table.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_delay_queue.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_pack_table.hpp"
#include "irods/irods_query.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_server_state.hpp"
#include "irods/json_deserialization.hpp"
#include "irods/json_serialization.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/msParam.h"
#include "irods/objInfo.h"
#include "irods/query_processor.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rcMisc.h"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsPackTable.h"
#include "irods/rodsUser.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/ruleExecDel.h"
#include "irods/ruleExecSubmit.h"
#include "irods/server_utilities.hpp"
#include "irods/thread_pool.hpp"

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <cstring>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <ios>
#include <thread>
#include <string>
#include <string_view>
#include <fstream>

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

// clang-format off
namespace ix = irods::experimental;

namespace logger = irods::experimental::log;

using json   = nlohmann::json;
// clang-format on

namespace
{
    std::atomic_bool delay_server_terminated{};

    void init_logger(
        const bool write_to_stdout,
        const bool enable_test_mode)
    {
        logger::init(write_to_stdout, enable_test_mode);

        logger::server::set_level(logger::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_SERVER));
        logger::legacy::set_level(logger::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_LEGACY));
        logger::delay_server::set_level(logger::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_DELAY_SERVER));

        logger::set_server_type("delay_server");

        if (char hostname[HOST_NAME_MAX + 1]{}; gethostname(hostname, sizeof(hostname)) == 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            logger::set_server_hostname(hostname);
        }

        // Attach the zone name to the logger.
        // We can't use the server properties interface because it depends on the logger.
        try {
            // Find the server configuration file.
            std::string config_path;

            if (const auto err = irods::get_full_path_for_config_file("server_config.json", config_path); !err.ok()) {
                return;
            }

            // Load the server configuration file in as JSON.
            nlohmann::json config;

            if (std::ifstream in{config_path}; in) {
                in >> config;
            }
            else {
                return;
            }

            logger::set_server_zone(config.at(irods::KW_CFG_ZONE_NAME).get<std::string>());
        }
        catch (...) {
        }
    } // init_logger

    std::optional<std::string_view> next_executor()
    {
        const auto config_handle{irods::server_properties::instance().map()};
        const auto& config{config_handle.get_json()};

        const auto advanced_settings = config.find(irods::KW_CFG_ADVANCED_SETTINGS);
        if (advanced_settings == std::end(config)) {
            return std::nullopt;
        }

        const auto executors = advanced_settings->find(irods::KW_CFG_DELAY_RULE_EXECUTORS);
        if (executors == std::end(*advanced_settings)) {
            return std::nullopt;
        }

        if (executors->empty()) {
            return std::nullopt;
        }

        static std::mutex executor_index_mutex;
        std::scoped_lock index_lock{executor_index_mutex};

        static int index = 0;

        // The calculation of the index could be configurable and would give the admin
        // options for how an executor is selected (e.g. round robin, random, etc.).
        index = index % executors->size();

        return executors->at(index++).get_ref<const std::string&>();
    }

    ix::client_connection get_new_connection(const std::optional<std::string>& _client_user)
    {
        try {
            if (_client_user) {
                if (const auto executor = next_executor(); executor) {
                    rodsEnv env{};
                    _getRodsEnv(env);

                    logger::delay_server::debug("Connecting to host [{}] as proxy user [{}] on behalf of user [{}] ...",
                                                *executor, env.rodsUserName, *_client_user);

                    return {*executor, env.rodsPort, env.rodsUserName, env.rodsZone, *_client_user, env.rodsZone};
                }
            }
        }
        catch (...) {
            logger::delay_server::error("Could not get the next delay rule executor.");
        }

        logger::delay_server::debug("Connecting to local server using server credentials.");

        return {};
    } // get_new_connection

    ruleExecSubmitInp_t fill_rule_exec_submit_inp(irods::experimental::client_connection& conn,
                                                  const std::string_view rule_id)
    {
        const std::string id{rule_id};
        BytesBuf* bbuf{};

        if (const auto ec = rc_get_delay_rule_info(static_cast<RcComm*>(conn), id.c_str(), &bbuf); ec != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Error retrieving delay rule matching rule ID [{}]", rule_id));
        }

        irods::at_scope_exit free_bbuf{[bbuf] { freeBBuf(bbuf); }};

        const auto* p = static_cast<const char*>(bbuf->buf);
        const auto info = json::parse(p, p + bbuf->len); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        // clang-format off
        const auto get_string_ref = [&info](const char* _prop) constexpr -> const std::string&
        {
            return info.at(_prop).get_ref<const std::string&>();
        };
        // clang-format on

        const auto& rei_file_path = get_string_ref("rei_file_path");
        const auto& exe_context = get_string_ref("exe_context");

        namespace fs = boost::filesystem;

        ruleExecSubmitInp_t rule_exec_submit_inp{};

        // Rules that have not been migrated from REI files will have the following catalog state:
        // - r_rule_exec.exe_context will be empty.
        // - r_rule_exec.rei_file_path will not be set to "EMPTY_REI_PATH".
        // - r_rule_exec.rei_file_path will be set to a valid file path on the file system.
        //
        // These rules will be migrated if and only if the rule text does not contain session variables.
        if (exe_context.empty() && rei_file_path != "EMPTY_REI_PATH" && fs::exists(rei_file_path)) {
            std::ifstream rei_file{rei_file_path, std::ios::in | std::ios::binary};

            if (!rei_file) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(SYS_OPEN_REI_FILE_ERR, fmt::format("Could not open REI file for rule [path={}].", rei_file_path));
            }

            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            rule_exec_submit_inp.packedReiAndArgBBuf = static_cast<BytesBuf*>(std::malloc(sizeof(BytesBuf)));
            rule_exec_submit_inp.packedReiAndArgBBuf->len = static_cast<int>(fs::file_size(rei_file_path));
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            rule_exec_submit_inp.packedReiAndArgBBuf->buf = std::malloc(rule_exec_submit_inp.packedReiAndArgBBuf->len + 1);

            std::memset(rule_exec_submit_inp.packedReiAndArgBBuf->buf, 0, rule_exec_submit_inp.packedReiAndArgBBuf->len + 1);

            rei_file.read(static_cast<char*>(rule_exec_submit_inp.packedReiAndArgBBuf->buf),
                          rule_exec_submit_inp.packedReiAndArgBBuf->len);

            if (rei_file.gcount() != rule_exec_submit_inp.packedReiAndArgBBuf->len) {
                const auto msg = fmt::format("Incorrect number of bytes read [expected={}, read={}].",
                                             rule_exec_submit_inp.packedReiAndArgBBuf->len,
                                             rei_file.gcount());
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(UNIX_FILE_READ_ERR, msg);
            }
        }

        // clang-format off
        const auto to_cstr = []<typename T>(T&& _s) constexpr noexcept -> char*
        {
            return static_cast<char*>(std::forward<T>(_s));
        };
        // clang-format on

        rstrcpy(to_cstr(rule_exec_submit_inp.ruleExecId), id.c_str(), NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.ruleName), get_string_ref("rule_name").c_str(), META_STR_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.reiFilePath), rei_file_path.c_str(), MAX_NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.userName), get_string_ref("user_name").c_str(), NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.exeAddress), get_string_ref("exe_address").c_str(), NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.exeTime), get_string_ref("exe_time").c_str(), TIME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.exeFrequency), get_string_ref("exe_frequency").c_str(), NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.priority), get_string_ref("priority").c_str(), NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.lastExecTime), get_string_ref("last_exe_time").c_str(), NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.exeStatus), get_string_ref("exe_status").c_str(), NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.estimateExeTime), get_string_ref("estimated_exe_time").c_str(), NAME_LEN);
        rstrcpy(to_cstr(rule_exec_submit_inp.notificationAddr), get_string_ref("notification_addr").c_str(), NAME_LEN);

        ix::key_value_proxy kvp{rule_exec_submit_inp.condInput};
        kvp[RULE_EXECUTION_CONTEXT_KW] = exe_context;

        return rule_exec_submit_inp;
    } // fill_rule_exec_submit_inp

    int update_entry_for_repeat(
        rcComm_t& _comm,
        ruleExecSubmitInp_t& _inp,
        int _exec_status)
    {
        logger::delay_server::trace("Updating rule's execution frequency [rule_id={}].", _inp.ruleExecId);

        // Prepare input for rule exec mod
        _exec_status = _exec_status > 0 ? 0 : _exec_status;

        // Prepare input for getting next repeat time
        char current_time[NAME_LEN]{};
        char* ef_string = _inp.exeFrequency;
        char next_time[NAME_LEN]{};
        snprintf(current_time, NAME_LEN, "%ld", std::time(nullptr));

        const auto delete_rule_exec_info{[&_comm, &_inp]() -> int {
            ruleExecDelInp_t rule_exec_del_inp{};
            rstrcpy(rule_exec_del_inp.ruleExecId, _inp.ruleExecId, NAME_LEN);
            const int status = rcRuleExecDel(&_comm, &rule_exec_del_inp);
            if (status < 0) {
                logger::delay_server::error("{}:{} - rcRuleExecDel failed {} for ID {}",
                    __FUNCTION__, __LINE__, status, rule_exec_del_inp.ruleExecId);
            }
            return status;
        }};

        const auto update_rule_exec_info = [&](const bool repeat_rule) -> int {
            ruleExecModInp_t rule_exec_mod_inp{};
            rstrcpy(rule_exec_mod_inp.ruleId, _inp.ruleExecId, NAME_LEN);

            if (std::strlen(_inp.priority) > 0) {
                // The priority string is not empty, but it could be invalid.
                // If the priority cannot be parsed into a valid signed integer, set it
                // to the default priority level of 5.
                try {
                    if (const auto p = std::stoi(_inp.priority); p < 1 || p > 9) {
                        addKeyVal(&rule_exec_mod_inp.condInput, RULE_PRIORITY_KW, "5");
                    }
                }
                catch (...) {
                    addKeyVal(&rule_exec_mod_inp.condInput, RULE_PRIORITY_KW, "5");
                }
            }
            else {
                // An empty priority is an indicator that the rule existed prior to iRODS v4.2.9
                // and that it needs to be set to the default priority level of 5.
                addKeyVal(&rule_exec_mod_inp.condInput, RULE_PRIORITY_KW, "5");
            }

            addKeyVal(&rule_exec_mod_inp.condInput, RULE_LAST_EXE_TIME_KW, current_time);
            addKeyVal(&rule_exec_mod_inp.condInput, RULE_EXE_TIME_KW, next_time);
            if(repeat_rule) {
                addKeyVal(&rule_exec_mod_inp.condInput, RULE_EXE_FREQUENCY_KW, ef_string);
            }

            const int status = rcRuleExecMod(&_comm, &rule_exec_mod_inp);
            if (status < 0) {
                logger::delay_server::error("{}:{} - rcRuleExecMod failed {} for rule ID {}",
                                             __FUNCTION__, __LINE__, status, rule_exec_mod_inp.ruleId);
            }

            if (rule_exec_mod_inp.condInput.len > 0) {
                clearKeyVal(&rule_exec_mod_inp.condInput);
            }

            return status;
        };

        logger::delay_server::debug("[{}:{}] - time:[{}],ef:[{}],next:[{}]",
                                    __FUNCTION__, __LINE__, current_time, ef_string, next_time);
        const int repeat_status = getNextRepeatTime(current_time, ef_string, next_time);
        switch(repeat_status) {
            case 0:
                // Continue with given delay regardless of success
                return update_rule_exec_info(false);
            case 1:
                // Remove if successful, otherwise update next exec time
                return !_exec_status ? delete_rule_exec_info() : update_rule_exec_info(false);
            case 2:
                // Remove regardless of success
                return delete_rule_exec_info();
            case 3:
                // Update with new exec time and frequency regardless of success
                return update_rule_exec_info(true);
            case 4:
                // Delete if successful, otherwise update with new exec time and frequency
                return !_exec_status ? delete_rule_exec_info() : update_rule_exec_info(true);
            default:
                logger::delay_server::error("{}:{} - getNextRepeatTime returned unknown value {} for rule ID {}",
                                            __FUNCTION__, __LINE__, repeat_status, _inp.ruleExecId);
                return repeat_status; 
        }
    } // update_entry_for_repeat

    void migrate_rule_execution_context_into_catalog(rcComm_t& _comm,
                                                     const std::string_view _rule_id,
                                                     const std::string_view _rei_file_path,
                                                     const std::string_view _rule_exec_ctx) noexcept
    {
        logger::delay_server::trace("Migrating REI file into catalog [rule_id={}] ...", _rule_id);

        try {
            ruleExecModInp_t input{};
            rstrcpy(input.ruleId, _rule_id.data(), NAME_LEN);

            ix::key_value_proxy kvp{input.condInput};
            kvp[RULE_REI_FILE_PATH_KW] = "EMPTY_REI_PATH";
            kvp[RULE_EXECUTION_CONTEXT_KW] = _rule_exec_ctx.data();

            if (const auto ec = rcRuleExecMod(&_comm, &input); ec < 0) {
                logger::delay_server::error("Failed to migrate REI file into catalog [rule_id={}, error_code={}]", _rule_id, ec);
                return;
            }

            std::string new_file_name{ _rei_file_path};
            new_file_name += ".migrated_into_catalog";

            boost::system::error_code ec;
            boost::filesystem::rename(_rei_file_path.data(), new_file_name, ec);
        }
        catch (...) {
            logger::delay_server::error("An unexpected error was encountered during REI file migration [rule_id={}].", _rule_id);
        }
    } // migrate_rule_execution_context_into_catalog

    exec_rule_expression_t pack_exec_rule_expression(ruleExecSubmitInp_t& _inp)
    {
        exec_rule_expression_t exec_rule{};

        const int packed_rei_len = _inp.packedReiAndArgBBuf->len;
        exec_rule.packed_rei_.len = packed_rei_len;
        exec_rule.packed_rei_.buf = _inp.packedReiAndArgBBuf->buf;

        const size_t rule_len = strlen(_inp.ruleName);
        const auto buffer_len = rule_len + 1;
        exec_rule.rule_text_.buf = static_cast<char*>(std::malloc(sizeof(char) * buffer_len));
        exec_rule.rule_text_.len = buffer_len;
        rstrcpy(static_cast<char*>(exec_rule.rule_text_.buf), _inp.ruleName, buffer_len);

        return exec_rule;
    } // pack_exec_rule_expression

    int run_rule_exec(rcComm_t& _comm, ruleExecSubmitInp_t& _inp)
    {
        logger::delay_server::trace("Generating rule execution context [rule_id={}].", _inp.ruleExecId);

        ruleExecInfoAndArg_t* rei_and_arg{};
        ruleExecInfo_t rei{};
        ruleExecInfo_t* rei_ptr{};

        // Indicates whether the rule needs to be migrated from an REI file to the catalog.
        bool migrate_rule = false;

        ix::key_value_proxy kvp{_inp.condInput};

        // The rule execution context will be stored in the catalog as JSON in
        // r_rule_exec.exe_context for migrated rules. If the rule has not been migrated,
        // then the context information must be stored in an REI file.
        if (kvp.contains(RULE_EXECUTION_CONTEXT_KW) && !kvp[RULE_EXECUTION_CONTEXT_KW].value().empty()) {
            rodsLog(LOG_DEBUG, "Inflating rule execution context from JSON string [rule_id=%s] ...", _inp.ruleExecId);

            rei = irods::to_rule_execution_info(kvp[RULE_EXECUTION_CONTEXT_KW].value());
            rei_ptr = &rei;

            // The nullptr and zero (0) represent the argument vector and its size (i.e. argv and argc).
            // Pack the REI so that it is available on the server-side.
            if (const auto ec = packReiAndArg(&rei, nullptr, 0, &_inp.packedReiAndArgBBuf); ec < 0) {
                logger::delay_server::error("Failed to pack rule execution information.");
                return SYS_INTERNAL_ERR;
            }
        }
        else {
            rodsLog(LOG_DEBUG, "Inflating rule execution context from REI file [rule_id=%s] ...", _inp.ruleExecId);

            // Rules are only migrated into the catalog if they do not use session variables.
            // This is because session variables are considered obsolete and will not be available
            // in iRODS v4.3.0. Rules should use the variables provided by dynamic PEPs.
            migrate_rule = !irods::contains_session_variables(_inp.ruleName);

            const auto ec = unpack_struct(_inp.packedReiAndArgBBuf->buf,
                                         reinterpret_cast<void**>(&rei_and_arg),
                                         "ReiAndArg_PI",
                                         RodsPackTable,
                                         NATIVE_PROT,
                                         nullptr);

            if (ec < 0) {
                logger::delay_server::error("Could not unpack struct [error_code={}].", ec);
                return ec;
            }

            rei_ptr = rei_and_arg->rei;
        }

        // Set the proxy user from the rei before delegating to the agent following behavior
        // from touchupPackedRei.
        //
        // In a standard connection, client and proxy info are identical.
        // This prevents potential escalation of privileges.
        _comm.proxyUser = *rei_ptr->uoic;

        exec_rule_expression_t exec_rule = pack_exec_rule_expression(_inp);
        exec_rule.params_ = rei_ptr->msParamArray;

        irods::at_scope_exit at_scope_exit{[&exec_rule, &rei, rei_and_arg] {
            clearBBuf(&exec_rule.rule_text_);

            if (rei_and_arg) {
                if (rei_and_arg->rei->rsComm) {
                    std::free(rei_and_arg->rei->rsComm);
                }

                freeRuleExecInfoStruct(rei_and_arg->rei, (FREE_MS_PARAM | FREE_DOINP));

                return;
            }

            if (rei.rsComm) {
                std::free(rei.rsComm);
            }

            freeRuleExecInfoInternals(&rei, (FREE_MS_PARAM | FREE_DOINP));
        }};

        // Execute rule.
        logger::delay_server::trace("Executing rule [rule_id={}].", _inp.ruleExecId);
        auto status = rcExecRuleExpression(&_comm, &exec_rule);
        if (delay_server_terminated) {
            logger::delay_server::info("Rule [{}] completed with status [{}] but delay server was terminated.",
                                       _inp.ruleExecId, status);
        }

        if (migrate_rule) {
            migrate_rule_execution_context_into_catalog(_comm,
                                                        _inp.ruleExecId,
                                                        _inp.reiFilePath,
                                                        irods::to_json(rei_and_arg->rei).dump());
        }

        if (strlen(_inp.exeFrequency) > 0) {
            logger::delay_server::trace("Updating rule execution information for next run [rule_id={}].", _inp.ruleExecId);
            return update_entry_for_repeat(_comm, _inp, status);
        }

        if (status < 0) {
            logger::delay_server::error("ruleExec of {}: {} failed.", _inp.ruleExecId, _inp.ruleName);
            ruleExecDelInp_t rule_exec_del_inp{};
            rstrcpy(rule_exec_del_inp.ruleExecId, _inp.ruleExecId, NAME_LEN);
            status = rcRuleExecDel(&_comm, &rule_exec_del_inp);
            if (status < 0) {
                logger::delay_server::error("rcRuleExecDel failed for {}, stat={}", _inp.ruleExecId, status);
                // Establish a new connection as the original may be invalid
                ix::client_connection conn;
                status = rcRuleExecDel(static_cast<rcComm_t*>(conn), &rule_exec_del_inp);
                if (status < 0) {
                    logger::delay_server::error("rcRuleExecDel failed again for {}, stat={} - exiting", _inp.ruleExecId, status);
                }
            }
            return status;
        }

        // Success - remove rule from catalog
        logger::delay_server::trace("Removing rule from catalog [rule_id={}].", _inp.ruleExecId);
        ruleExecDelInp_t rule_exec_del_inp{};
        rstrcpy(rule_exec_del_inp.ruleExecId, _inp.ruleExecId, NAME_LEN);
        status = rcRuleExecDel(&_comm, &rule_exec_del_inp);
        if (status < 0) {
            logger::delay_server::error("Failed deleting rule exec {} from catalog", rule_exec_del_inp.ruleExecId);
        }

        logger::delay_server::trace("Rule processed [rule_id={}].", _inp.ruleExecId);

        return status;
    } // run_rule_exec

    void execute_rule(irods::delay_queue& queue, const std::string_view rule_id)
    {
        if (delay_server_terminated) {
            return;
        }

        ruleExecSubmitInp_t rule_exec_submit_inp{};

        irods::at_scope_exit at_scope_exit{[&rule_exec_submit_inp] {
            freeBBuf(rule_exec_submit_inp.packedReiAndArgBBuf);
        }};

        ix::client_connection conn = get_new_connection(std::nullopt);

        try {
            rule_exec_submit_inp = fill_rule_exec_submit_inp(conn, rule_id);
        }
        catch (const irods::exception& e) {
            if (delay_server_terminated) {
                // Get out!
                return;
            }

            if (CAT_NO_ROWS_FOUND == e.code()) {
                // CAT_NO_ROWS_FOUND is returned in the event that the rule ID does not exist
                // in the catalog. If this is the case, we assume that an executor has completed
                // the rule since retrieving it in the main thread. Therefore, there is nothing
                // to do and the rule can be safely removed from the queue.
                logger::delay_server::info("dequeueing rule because rule ID [{}] no longer exists in the catalog", rule_id);
                queue.dequeue_rule(std::string(rule_exec_submit_inp.ruleExecId));
            }
            else {
                // Something serious happened - log it here.
                logger::delay_server::error("[{}:{}] - [{}]", __func__, __LINE__, e.client_display_what());
            }

            return;
        }
        catch (const std::exception& e) {
            logger::delay_server::error("[{}:{}] - [{}]", __func__, __LINE__, e.what());

            return;
        }

        conn = get_new_connection(rule_exec_submit_inp.userName);
        try {
            if (const int status = run_rule_exec(conn, rule_exec_submit_inp); status < 0) {
                logger::delay_server::error("Rule exec for [{}] failed. status = [{}]", rule_exec_submit_inp.ruleExecId, status);
            }
        }
        catch (const std::exception& e) {
            logger::delay_server::error("Exception caught during execution of rule [{}]: [{}]",
                                        rule_exec_submit_inp.ruleExecId, e.what());
        }

        logger::delay_server::debug("rule [{}] complete", rule_exec_submit_inp.ruleExecId);
        if (!delay_server_terminated) {
            logger::delay_server::debug("dequeueing rule [{}]", rule_exec_submit_inp.ruleExecId);
            queue.dequeue_rule(std::string(rule_exec_submit_inp.ruleExecId));
        }
        logger::delay_server::debug("rule [{}] exists in queue: [{}]", rule_exec_submit_inp.ruleExecId, queue.contains_rule_id(rule_exec_submit_inp.ruleExecId));
    } // execute_rule

    auto make_delay_queue_query_processor(
        irods::thread_pool& thread_pool,
        irods::delay_queue& queue) -> irods::query_processor<rcComm_t>
    {
        using result_row = irods::query_processor<rsComm_t>::result_row;

        const auto job = [&](const result_row& result) -> void
        {
            const auto& rule_id = result[0];
            if (queue.contains_rule_id(rule_id)) {
                return;
            }

            logger::delay_server::debug("Enqueueing rule ID [{}]", rule_id);

            try {
                queue.enqueue_rule(rule_id);
            }
            catch (const std::bad_alloc& e) {
                logger::delay_server::trace("Delay queue memory limit reached. Ignoring rule ID [{}] for now.", rule_id);
                return;
            }

            irods::thread_pool::post(thread_pool, [&queue, result] {
                // Remove the rule from the delay queue no matter what.
                //
                // This is necessary due to exceptions. If an exception is thrown from execute_rule(),
                // the rule won't be removed. This is bad because it would result in the queued rule never
                // being handled until the delay server process is restarted.
                //
                // This at_scope_exit object protects the delay server from this situation. It also allows
                // the rule to be rescheduled for execution.
                irods::at_scope_exit remove_rule_from_queue{[&] {
                    try {
                        logger::delay_server::trace("Dequeuing rule ID [{}] ...", result[0]);
                        queue.dequeue_rule(result[0]);
                        logger::delay_server::trace("Rule ID [{}] dequeued successfully.", result[0]);
                    }
                    catch (...) {}
                }};

                try {
                    execute_rule(queue, result[0]);
                }
                catch (const irods::exception& e) {
                    logger::delay_server::error(e.what());
                }
                catch (const std::exception& e) {
                    logger::delay_server::error(e.what());
                }
                catch (...) {
                    logger::delay_server::error("Caught an unknown error.");
                }
            });
        };

        const auto qstr = fmt::format("SELECT RULE_EXEC_ID, ORDER_DESC(RULE_EXEC_PRIORITY) "
                                      "WHERE RULE_EXEC_TIME <= '{}'", std::time(nullptr));

        return {qstr, job};
    } // make_delay_queue_query_processor
} // anonymous namespace

int main(int argc, char** argv)
{
    bool enable_test_mode = false;
    bool write_to_stdout = false;
    int c{};
    while (EOF != (c = getopt(argc, argv, "tu"))) {
        switch (c) {
            case 't':
                enable_test_mode = true;
                break;
            case 'u':
                write_to_stdout = true;
                break;
            default:
                std::cerr << "Only -t and -u are supported" << std::endl;
                exit(1);
        }
    }

    irods::server_properties::instance().capture();

    init_logger(write_to_stdout, enable_test_mode);

    logger::delay_server::info("Initializing delay server ...");

    const auto pid_file_fd = irods::create_pid_file(irods::PID_FILENAME_DELAY_SERVER);
    if (pid_file_fd == -1) {
        return 1;
    }

    set_ips_display_name(boost::filesystem::path{argv[0]}.filename().c_str());

    load_client_api_plugins();

    const auto signal_exit_handler = [](int) { delay_server_terminated.store(true); };
    signal(SIGINT, signal_exit_handler);
    signal(SIGHUP, signal_exit_handler);
    signal(SIGTERM, signal_exit_handler);
    signal(SIGUSR1, signal_exit_handler);

    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl = irods::get_pack_table();
    init_api_table(api_tbl, pk_tbl);

    const auto sleep_time = [] {
        try {
            return irods::get_advanced_setting<const int>(irods::KW_CFG_DELAY_SERVER_SLEEP_TIME_IN_SECONDS);
        }
        catch (...) {
            logger::delay_server::warn("Could not retrieve [{}] from advanced settings configuration. "
                                       "Using default value of {}.",
                                       irods::KW_CFG_DELAY_SERVER_SLEEP_TIME_IN_SECONDS,
                                       irods::default_delay_server_sleep_time_in_seconds);
        }

        return irods::default_delay_server_sleep_time_in_seconds;
    };

    const auto go_to_sleep = [&sleep_time] {
        const auto start_time = std::chrono::system_clock::now();
        const auto allowed_sleep_time = std::chrono::seconds{sleep_time()};

        // Loop until the server is signaled to shutdown or the max amount of time
        // to sleep has been reached.
        while (true) {
            if (delay_server_terminated.load()) {
                logger::delay_server::info("Delay server received shutdown signal.");
                return;
            }
            
            if (std::chrono::system_clock::now() - start_time >= allowed_sleep_time) {
                logger::delay_server::debug("Delay server is awake.");
                return;
            }

            std::this_thread::sleep_for(std::chrono::seconds{1});
        }
    };

    const auto number_of_concurrent_executors = [] {
        try {
            return irods::get_advanced_setting<const int>(irods::KW_CFG_NUMBER_OF_CONCURRENT_DELAY_RULE_EXECUTORS);
        }
        catch (...) {
            logger::delay_server::warn("Could not retrieve [{}] from advanced settings configuration. "
                                       "Using default value of {}.",
                                       irods::KW_CFG_NUMBER_OF_CONCURRENT_DELAY_RULE_EXECUTORS,
                                       irods::default_number_of_concurrent_delay_executors);
        }

        return irods::default_number_of_concurrent_delay_executors;
    }();

    irods::thread_pool thread_pool{number_of_concurrent_executors};

    const auto queue_size_in_bytes = []() -> int {
        try {
            const auto bytes = irods::get_advanced_setting<int>(irods::KW_CFG_MAX_SIZE_OF_DELAY_QUEUE_IN_BYTES);

            if (bytes > 0) {
                return bytes;
            }
        }
        catch (...) {
            logger::delay_server::warn("Could not retrieve [{}] from advanced settings configuration. "
                                       "Delay server will use as much memory as necessary.",
                                       irods::KW_CFG_MAX_SIZE_OF_DELAY_QUEUE_IN_BYTES);
        }

        return 0;
    }();

    irods::delay_queue queue{queue_size_in_bytes};

    try {
        while (!delay_server_terminated) {
            try {
                irods::server_properties::instance().capture();

                auto delay_queue_processor = make_delay_queue_query_processor(thread_pool, queue);

                logger::delay_server::trace("Gathering rules for execution ...");
                ix::client_connection query_conn;
                auto future = delay_queue_processor.execute(thread_pool, static_cast<rcComm_t&>(query_conn));

                logger::delay_server::trace("Waiting for rules to finish processing ...");
                auto errors = future.get();

                logger::delay_server::trace("Rules have been processed. Checking for errors ...");
                if (errors.size() > 0) {
                    for (const auto& [code, msg] : errors) {
                        logger::delay_server::error("Executing delayed rule failed - [{}]::[{}]", code, msg);
                    }
                }
            }
            catch (const irods::exception& e) {
                logger::delay_server::error(e.what());
            }
            catch (const std::exception& e) {
                logger::delay_server::error(e.what());
            }

            logger::delay_server::trace("Delay server is going to sleep.");
            go_to_sleep();
        }
    }
    catch (const irods::exception& e) {
        logger::delay_server::error(e.what());
    }

    logger::delay_server::info("Delay server exited normally.");

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    __lsan_do_leak_check();
#endif

    return 0;
}
