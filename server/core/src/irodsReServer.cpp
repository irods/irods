#include "connection_pool.hpp"
#include "client_connection.hpp"
#include "initServer.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_delay_queue.hpp"
#include "irods_log.hpp"
#include "irods_query.hpp"
#include "irods_re_structs.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_state.hpp"
#include "rcGlobalExtern.h"
#include "rcMisc.h"
#include "rodsDef.h"
#include "thread_pool.hpp"
#include "irodsReServer.hpp"
#include "miscServerFunct.hpp"
#include "msParam.h"
#include "objInfo.h"
#include "query_processor.hpp"
#include "rodsClient.h"
#include "rodsErrorTable.h"
#include "rodsPackTable.h"
#include "rodsUser.h"
#include "rsGlobalExtern.hpp"
#include "rsLog.hpp"
#include "ruleExecDel.h"
#include "ruleExecSubmit.h"
#include "key_value_proxy.hpp"
#include "server_utilities.hpp"
#include "json_serialization.hpp"
#include "json_deserialization.hpp"
#include "server_utilities.hpp"
#include "get_delay_rule_info.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <fmt/format.h>
#include <json.hpp>

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

// clang-format off
namespace ix = irods::experimental;

using json   = nlohmann::json;
// clang-format on

namespace {
    std::atomic_bool re_server_terminated{};

    void set_ips_display_name(const std::string_view _display_name)
    {
        // Setting this environment variable is required so that "ips" can display
        // the command name alongside the connection information.
        if (setenv(SP_OPTION, _display_name.data(), /* overwrite */ 1)) {
            rodsLog(LOG_WARNING, "Could not set environment variable [spOption] for ips.");
        }
    }

    int init_log() {
        /* Handle option to log sql commands */
        auto sql_log_level = getenv(SP_LOG_SQL);
        if(sql_log_level) {
            int j{1};
            #ifdef SYSLOG
                j = atoi(sql_log_level);
            #endif
            rodsLogSqlReq(j);
        }

        /* Set the logging level */
        rodsLogLevel(LOG_NOTICE); /* default */
        auto log_level = getenv(SP_LOG_LEVEL);
        if (log_level) {
            rodsLogLevel(std::atoi(log_level));
        }

        #ifdef SYSLOG
            /* Open a connection to syslog */
            openlog("irodsDelayServer", LOG_ODELAY | LOG_PID, LOG_DAEMON);
        #endif
        int log_fd = logFileOpen(SERVER, nullptr, RULE_EXEC_LOGFILE);
        if (log_fd >= 0) {
            daemonize(SERVER, log_fd);
        }
        return log_fd;
    }

    ruleExecSubmitInp_t fill_rule_exec_submit_inp(
        irods::experimental::client_connection& conn,
        const std::string_view rule_id)
    {
        const std::string id{rule_id};
        BytesBuf* bbuf{};

        if (const auto ec = rc_get_delay_rule_info(static_cast<RcComm*>(conn), id.c_str(), &bbuf); ec != 0) {
            THROW(ec, fmt::format("Error retrieving delay rule matching rule id [rule_id={}]", rule_id));
        }

        irods::at_scope_exit free_bbuf{[bbuf] { freeBBuf(bbuf); }};

        const auto* p = static_cast<const char*>(bbuf->buf);
        const auto info = json::parse(p, p + bbuf->len);

        namespace fs = boost::filesystem;

        ruleExecSubmitInp_t rule_exec_submit_inp{};

        const auto get_string_ref = [&info](const char* _prop) -> const std::string&
        {
            return info.at(_prop).get_ref<const std::string&>();
        };

        const auto& rei_file_path = get_string_ref("rei_file_path");
        const auto& exe_context = get_string_ref("exe_context");

        // Rules that have not been migrated from REI files will have the following catalog state:
        // - r_rule_exec.exe_context will be empty.
        // - r_rule_exec.rei_file_path will not be set to "EMPTY_REI_PATH".
        // - r_rule_exec.rei_file_path will be set to a valid file path on the file system.
        //
        // These rules will be migrated if and only if the rule text does not contain session variables.
        if (exe_context.empty() &&
            rei_file_path != "EMPTY_REI_PATH" &&
            fs::exists(rei_file_path))
        {
            std::ifstream rei_file{rei_file_path, std::ios::in | std::ios::binary};

            if (!rei_file) {
                THROW(SYS_OPEN_REI_FILE_ERR, fmt::format("Could not open REI file for rule [path={}].", rei_file_path));
            }

            rule_exec_submit_inp.packedReiAndArgBBuf = static_cast<BytesBuf*>(std::malloc(sizeof(BytesBuf)));
            rule_exec_submit_inp.packedReiAndArgBBuf->len = static_cast<int>(fs::file_size(rei_file_path));
            rule_exec_submit_inp.packedReiAndArgBBuf->buf = std::malloc(rule_exec_submit_inp.packedReiAndArgBBuf->len + 1);

            std::memset(rule_exec_submit_inp.packedReiAndArgBBuf->buf, 0, rule_exec_submit_inp.packedReiAndArgBBuf->len + 1);

            rei_file.read(static_cast<char*>(rule_exec_submit_inp.packedReiAndArgBBuf->buf),
                          rule_exec_submit_inp.packedReiAndArgBBuf->len);

            if (rei_file.gcount() != rule_exec_submit_inp.packedReiAndArgBBuf->len) {
                const auto msg = fmt::format("Incorrect number of bytes read [expected={}, read={}].",
                                             rule_exec_submit_inp.packedReiAndArgBBuf->len,
                                             rei_file.gcount());
                THROW(UNIX_FILE_READ_ERR, msg);
            }
        }

        rstrcpy(rule_exec_submit_inp.ruleExecId, id.c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.ruleName, get_string_ref("rule_name").c_str(), META_STR_LEN);
        rstrcpy(rule_exec_submit_inp.reiFilePath, rei_file_path.c_str(), MAX_NAME_LEN);
        rstrcpy(rule_exec_submit_inp.userName, get_string_ref("user_name").c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeAddress, get_string_ref("exe_address").c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeTime, get_string_ref("exe_time").c_str(), TIME_LEN);
        rstrcpy(rule_exec_submit_inp.exeFrequency, get_string_ref("exe_frequency").c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.priority, get_string_ref("priority").c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.lastExecTime, get_string_ref("last_exe_time").c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeStatus, get_string_ref("exe_status").c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.estimateExeTime, get_string_ref("estimated_exe_time").c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.notificationAddr, get_string_ref("notification_addr").c_str(), NAME_LEN);

        ix::key_value_proxy kvp{rule_exec_submit_inp.condInput};
        kvp[RULE_EXECUTION_CONTEXT_KW] = exe_context;

        return rule_exec_submit_inp;
    }

    int update_entry_for_repeat(
        rcComm_t& _comm,
        ruleExecSubmitInp_t& _inp,
        int _exec_status)
    {
        rodsLog(LOG_DEBUG, "Updating rule's execution frequency [rule_id=%s].", _inp.ruleExecId);

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
                const auto msg = fmt::format("{}:{} - rcRuleExecDel failed {} for id {}",
                                             __FUNCTION__, __LINE__, status, rule_exec_del_inp.ruleExecId);
                irods::log(LOG_ERROR, msg);
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
                const auto msg = fmt::format("{}:{} - rcRuleExecMod failed {} for id {}",
                                             __FUNCTION__, __LINE__, status, rule_exec_mod_inp.ruleId);
                irods::log(LOG_ERROR, msg);
            }
            if (rule_exec_mod_inp.condInput.len > 0) {
                clearKeyVal(&rule_exec_mod_inp.condInput);
            }
            return status;
        };

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
            default: {
                rodsLog(LOG_ERROR, "%s:%s - getNextRepeatTime returned unknown value %d for id %s",
                        __FUNCTION__, __LINE__, repeat_status, _inp.ruleExecId);
                return repeat_status; 
            }
        }
    }

    void migrate_rule_execution_context_into_catalog(rcComm_t& _comm,
                                                     const std::string_view _rule_id,
                                                     const std::string_view _rei_file_path,
                                                     const std::string_view _rule_exec_ctx) noexcept
    {
        rodsLog(LOG_DEBUG, "Migrating REI file into catalog [rule_id=%s] ...", _rule_id.data());

        try {
            ruleExecModInp_t input{};
            rstrcpy(input.ruleId, _rule_id.data(), NAME_LEN);

            ix::key_value_proxy kvp{input.condInput};
            kvp[RULE_REI_FILE_PATH_KW] = "EMPTY_REI_PATH";
            kvp[RULE_EXECUTION_CONTEXT_KW] = _rule_exec_ctx.data();

            if (const auto ec = rcRuleExecMod(&_comm, &input); ec < 0) {
                const auto msg = fmt::format("Failed to migrate REI file into catalog [rule_id={}, error_code={}]", _rule_id, ec);
                rodsLog(LOG_ERROR, msg.data());
                return;
            }

            std::string new_file_name{ _rei_file_path};
            new_file_name += ".migrated_into_catalog";

            boost::system::error_code ec;
            boost::filesystem::rename(_rei_file_path.data(), new_file_name, ec);
        }
        catch (...) {
            rodsLog(LOG_ERROR, "An unexpected error was encountered during REI file migration [rule_id=%s] ...", _rule_id.data());
        }
    }

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
    }

    int run_rule_exec(rcComm_t& _comm, ruleExecSubmitInp_t& _inp)
    {
        rodsLog(LOG_DEBUG, "Generating rule execution context [rule_id=%s].", _inp.ruleExecId);

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
                rodsLog(LOG_ERROR, "Failed to pack rule execution information.");
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
                rodsLog(LOG_ERROR, "Could not unpack struct [error_code=%d].", ec);
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
        rodsLog(LOG_DEBUG, "Executing rule [rule_id=%s].", _inp.ruleExecId);
        auto status = rcExecRuleExpression(&_comm, &exec_rule);
        if (re_server_terminated) {
            const auto msg = fmt::format("Rule [{}] completed with status [{}] but RE server was terminated.",
                                         _inp.ruleExecId, status);
            irods::log(LOG_DEBUG, msg);
        }

        if (migrate_rule) {
            migrate_rule_execution_context_into_catalog(_comm,
                                                        _inp.ruleExecId,
                                                        _inp.reiFilePath,
                                                        irods::to_json(rei_and_arg->rei).dump());
        }

        if (strlen(_inp.exeFrequency) > 0) {
            rodsLog(LOG_DEBUG, "Updating rule execution information for next run [rule_id=%s].", _inp.ruleExecId);
            return update_entry_for_repeat(_comm, _inp, status);
        }

        if (status < 0) {
            rodsLog(LOG_ERROR, "ruleExec of %s: %s failed.", _inp.ruleExecId, _inp.ruleName);
            ruleExecDelInp_t rule_exec_del_inp{};
            rstrcpy(rule_exec_del_inp.ruleExecId, _inp.ruleExecId, NAME_LEN);
            status = rcRuleExecDel(&_comm, &rule_exec_del_inp);
            if (status < 0) {
                rodsLog(LOG_ERROR, "rcRuleExecDel failed for %s, stat=%d", _inp.ruleExecId, status);
                // Establish a new connection as the original may be invalid
                ix::client_connection conn;
                status = rcRuleExecDel(static_cast<rcComm_t*>(conn), &rule_exec_del_inp);
                if (status < 0) {
                    irods::log(LOG_ERROR,
                            (boost::format("rcRuleExecDel failed again for %s, stat=%d - exiting") %
                            _inp.ruleExecId % status).str());
                }
            }
            return status;
        }

        // Success - remove rule from catalog
        rodsLog(LOG_DEBUG, "Removing rule from catalog [rule_id=%s].", _inp.ruleExecId);
        ruleExecDelInp_t rule_exec_del_inp{};
        rstrcpy(rule_exec_del_inp.ruleExecId, _inp.ruleExecId, NAME_LEN);
        status = rcRuleExecDel(&_comm, &rule_exec_del_inp);
        if (status < 0) {
            rodsLog(LOG_ERROR, "Failed deleting rule exec %s from catalog", rule_exec_del_inp.ruleExecId);
        }

        rodsLog(LOG_DEBUG, "Rule processed [rule_id=%s].", _inp.ruleExecId);

        return status;
    }

    void execute_rule(irods::delay_queue& queue, const std::string_view rule_id)
    {
        if (re_server_terminated) {
            return;
        }

        ruleExecSubmitInp_t rule_exec_submit_inp{};

        irods::at_scope_exit at_scope_exit{[&rule_exec_submit_inp] {
            freeBBuf(rule_exec_submit_inp.packedReiAndArgBBuf);
        }};

        ix::client_connection conn;

        try {
            rule_exec_submit_inp = fill_rule_exec_submit_inp(conn, rule_id);
        }
        catch (const irods::exception& e) {
            if (re_server_terminated) {
                // Get out!
                return;
            }

            if (CAT_NO_ROWS_FOUND == e.code()) {
                // CAT_NO_ROWS_FOUND is returned in the event that the rule ID does not exist
                // in the catalog. If this is the case, we assume that an executor has completed
                // the rule since retrieving it in the main thread. Therefore, there is nothing
                // to do and the rule can be safely removed from the queue.
                irods::log(LOG_NOTICE, fmt::format("dequeueing rule because rule ID [{}] no longer exists in the catalog", rule_id));
                queue.dequeue_rule(std::string(rule_exec_submit_inp.ruleExecId));
            }
            else {
                // Something serious happened - log it here.
                irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __func__, __LINE__, e.client_display_what()));
            }

            return;
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __func__, __LINE__, e.what()));

            return;
        }

        try {
            if (const int status = run_rule_exec(conn, rule_exec_submit_inp); status < 0) {
                rodsLog(LOG_ERROR, "Rule exec for [%s] failed. status = [%d]",
                        rule_exec_submit_inp.ruleExecId, status);
            }
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, "Exception caught during execution of rule [%s]: [%s]",
                    rule_exec_submit_inp.ruleExecId, e.what());
        }

        irods::log(LOG_DEBUG8, fmt::format("rule [{}] complete", rule_exec_submit_inp.ruleExecId));
        if (!re_server_terminated) {
            irods::log(LOG_DEBUG8, fmt::format("dequeueing rule [{}]", rule_exec_submit_inp.ruleExecId));
            queue.dequeue_rule(std::string(rule_exec_submit_inp.ruleExecId));
        }
        irods::log(LOG_DEBUG8, fmt::format("rule [{}] exists in queue: [{}]",
                   rule_exec_submit_inp.ruleExecId,
                   queue.contains_rule_id(rule_exec_submit_inp.ruleExecId)));
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

            rodsLog(LOG_DEBUG, "Enqueueing rule [%s]", rule_id.data());

            try {
                queue.enqueue_rule(rule_id);
            }
            catch (const std::bad_alloc& e) {
                rodsLog(LOG_DEBUG, "Delay queue memory limit reached. Ignoring rule [%s] for now.", rule_id.c_str());
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
                        irods::log(LOG_DEBUG10, fmt::format("Dequeuing rule ID [{}] ...", result[0]));
                        queue.dequeue_rule(result[0]);
                        irods::log(LOG_DEBUG10, fmt::format("Rule ID [{}] dequeued successfully.", result[0]));
                    }
                    catch (...) {}
                }};

                try {
                    execute_rule(queue, result[0]);
                }
                catch (const irods::exception& e) {
                    irods::log(LOG_ERROR, e.what());
                }
                catch (const std::exception& e) {
                    irods::log(LOG_ERROR, e.what());
                }
                catch (...) {
                    irods::log(LOG_ERROR, "Caught an unknown error.");
                }
            });
        };

        const auto qstr = fmt::format("SELECT RULE_EXEC_ID, ORDER_DESC(RULE_EXEC_PRIORITY) "
                                      "WHERE RULE_EXEC_TIME <= '{}'", std::time(nullptr));

        return {qstr, job};
    }
} // anonymous namespace

int main(int, char* _argv[])
{
    set_ips_display_name(boost::filesystem::path{_argv[0]}.filename().c_str());

    load_client_api_plugins();

    static std::condition_variable term_cv;
    static std::mutex term_m;
    const auto signal_exit_handler = [](int signal) {
        rodsLog(LOG_NOTICE, "RE server received signal [%d]", signal);
        re_server_terminated = true;
        term_cv.notify_all();
    };
    signal(SIGINT, signal_exit_handler);
    signal(SIGHUP, signal_exit_handler);
    signal(SIGTERM, signal_exit_handler);
    signal(SIGUSR1, signal_exit_handler);

    auto log_fd = init_log();
    if(log_fd < 0) {
        exit(log_fd);
    }

    const auto sleep_time = [] {
        try {
            return irods::get_advanced_setting<const int>(irods::CFG_RE_SERVER_SLEEP_TIME);
        }
        catch (...) {
            rodsLog(LOG_WARNING,
                    "Could not retrieve [%s] from advanced settings configuration. Using default value of %i.",
                    irods::CFG_RE_SERVER_SLEEP_TIME.c_str(),
                    irods::default_re_server_sleep_time);
        }

        return irods::default_re_server_sleep_time;
    }();

    const auto go_to_sleep = [&sleep_time]() {
        std::unique_lock<std::mutex> sleep_lock{term_m};
        const auto until = std::chrono::system_clock::now() + std::chrono::seconds(sleep_time);
        if (std::cv_status::no_timeout == term_cv.wait_until(sleep_lock, until)) {
            irods::log(LOG_DEBUG, "RE server awoken by a notification");
        }
    };

    const auto thread_count = [] {
        try {
            return irods::get_advanced_setting<const int>(irods::CFG_MAX_NUMBER_OF_CONCURRENT_RE_PROCS);
        }
        catch (...) {
            rodsLog(LOG_WARNING,
                    "Could not retrieve [%s] from advanced settings configuration. Using default value of %i.",
                    irods::CFG_MAX_NUMBER_OF_CONCURRENT_RE_PROCS.c_str(),
                    irods::default_max_number_of_concurrent_re_threads);
        }

        return irods::default_max_number_of_concurrent_re_threads;
    }();
    irods::thread_pool thread_pool{thread_count};

    const auto queue_size_in_bytes = []() -> int {
        try {
            const auto bytes = irods::get_advanced_setting<int>(irods::CFG_MAX_SIZE_OF_DELAY_QUEUE_IN_BYTES_KW);

            if (bytes > 0) {
                return bytes;
            }
        }
        catch (...) {
            rodsLog(LOG_WARNING,
                    "Could not retrieve [%s] from advanced settings configuration. "
                    "Delay server will use as much memory as necessary.",
                    irods::CFG_MAX_SIZE_OF_DELAY_QUEUE_IN_BYTES_KW.c_str());
        }

        return 0;
    }();

    irods::delay_queue queue{queue_size_in_bytes};

    try {
        while(!re_server_terminated) {
            rodsLog(LOG_DEBUG, "RE server is awake.");

            try {
                irods::parse_and_store_hosts_configuration_file_as_json();

                auto delay_queue_processor = make_delay_queue_query_processor(thread_pool, queue);

                rodsLog(LOG_DEBUG, "Gathering rules for execution ...");
                ix::client_connection query_conn;
                auto future = delay_queue_processor.execute(thread_pool, static_cast<rcComm_t&>(query_conn));

                rodsLog(LOG_DEBUG, "Waiting for rules to finish processing ...");
                auto errors = future.get();

                rodsLog(LOG_DEBUG, "Rules have been processed. Checking for errors ...");
                if(errors.size() > 0) {
                    for(const auto& [code, msg] : errors) {
                        rodsLog(LOG_ERROR, "Executing delayed rule failed - [%d]::[%s]", code, msg.data());
                    }
                }
            } catch(const irods::exception& e) {
                irods::log(e);
            } catch(const std::exception& e) {
                irods::log(LOG_ERROR, e.what());
            }

            rodsLog(LOG_DEBUG, "RE server is going to sleep.");
            go_to_sleep();
        }
    } catch(const irods::exception& e) {
        irods::log(e);
    }

    irods::log(LOG_NOTICE, "RE server exiting...");

    return 0;
}
