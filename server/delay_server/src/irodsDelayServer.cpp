#include "irods/irodsDelayServer.hpp"

#include "irods/catalog.hpp"
#include "irods/client_connection.hpp"
#include "irods/connection_pool.hpp"
#include "irods/initServer.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_client_api_table.hpp"
#include "irods/irods_delay_queue.hpp"
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
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsPackTable.h"
#include "irods/rodsUser.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rsLog.hpp"
#include "irods/ruleExecDel.h"
#include "irods/ruleExecSubmit.h"
#include "irods/server_utilities.hpp"
#include "irods/thread_pool.hpp"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <nanodbc/nanodbc.h>

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

using logger = irods::experimental::log;
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

        logger::server::set_level(logger::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_SERVER_KW));
        logger::legacy::set_level(logger::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_LEGACY_KW));
        logger::delay_server::set_level(logger::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_DELAY_SERVER_KW));

        logger::set_server_type("delay_server");

        if (char hostname[HOST_NAME_MAX]{}; gethostname(hostname, sizeof(hostname)) == 0) {
            logger::set_server_host(hostname);
        }
    } // init_logger

    std::optional<std::string_view> next_executor()
    {
        const auto& config = irods::server_properties::instance().map();

        const auto advanced_settings = config.find(irods::CFG_ADVANCED_SETTINGS_KW);
        if (advanced_settings == std::end(config)) {
            return std::nullopt;
        }

        const auto executors = advanced_settings->find(irods::DELAY_RULE_EXECUTORS_KW);
        if (executors == std::end(*advanced_settings)) {
            return std::nullopt;
        }

        if (executors->empty()) {
            return std::nullopt;
        }

        static std::atomic<int> index = 0;

        // The calculation of the index could be configurable and would give the admin
        // options for how an executor is selected (e.g. round robin, random, etc.).
        index = index++ % executors->size();

        return executors->at(index).get_ref<const std::string&>();
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

    void set_ips_display_name(const std::string_view _display_name)
    {
        // Setting this environment variable is required so that "ips" can display
        // the command name alongside the connection information.
        if (setenv(SP_OPTION, _display_name.data(), /* overwrite */ 1)) {
            logger::delay_server::warn("Could not set environment variable [spOption] for ips.");
        }
    } // set_ips_display_name

    ruleExecSubmitInp_t fill_rule_exec_submit_inp(const std::string_view rule_id)
    {
        std::vector<std::string> exec_info;

        try {
            // The database plugin implementation is not capable of fetching the r_rule_exec.exe_context
            // column when it contains a large string. To get around this limitation, the delay server
            // uses nanodbc directly. This solution assumes the delay server runs on an iRODS server that
            // has access to the database credentials.

            namespace ic = irods::experimental::catalog;

            auto [db_instance_name, db_conn] = ic::new_database_connection(/* read_server_config */ false);

            nanodbc::statement stmt{db_conn};
            nanodbc::prepare(stmt, "select rule_name, rei_file_path, user_name, exe_address, exe_time,"
                                   " exe_frequency, priority, last_exe_time, exe_status, estimated_exe_time,"
                                   " notification_addr, exe_context "
                                   "from R_RULE_EXEC where rule_exec_id = ?");

            stmt.bind(0, rule_id.data());

            auto row = nanodbc::execute(stmt);

            if (!row.next()) {
                THROW(CAT_NO_ROWS_FOUND, fmt::format("Could not find row matching rule id [rule_id={}]", rule_id));
            }

            constexpr auto number_of_columns = 12;
            exec_info.reserve(number_of_columns); 

            for (int i = 0; i < number_of_columns; ++i) {
                exec_info.push_back(row.get<std::string>(i, ""));
            }
        }
        catch (const std::exception& e) {
            THROW(SYS_INTERNAL_ERR, e.what());
        }

        namespace fs = boost::filesystem;

        ruleExecSubmitInp_t rule_exec_submit_inp{};

        // Rules that have not been migrated from REI files will have the following catalog state:
        // - r_rule_exec.exe_context will be empty.
        // - r_rule_exec.rei_file_path will not be set to "EMPTY_REI_PATH".
        // - r_rule_exec.rei_file_path will be set to a valid file path on the file system.
        //
        // These rules will be migrated if and only if the rule text does not contain session variables.
        if (const auto& rei_file_path = exec_info[1];
            exec_info[11].empty() &&
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

        rstrcpy(rule_exec_submit_inp.ruleExecId, rule_id.data(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.ruleName, exec_info[0].c_str(), META_STR_LEN);
        rstrcpy(rule_exec_submit_inp.reiFilePath, exec_info[1].c_str(), MAX_NAME_LEN);
        rstrcpy(rule_exec_submit_inp.userName, exec_info[2].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeAddress, exec_info[3].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeTime, exec_info[4].c_str(), TIME_LEN);
        rstrcpy(rule_exec_submit_inp.exeFrequency, exec_info[5].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.priority, exec_info[6].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.lastExecTime, exec_info[7].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeStatus, exec_info[8].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.estimateExeTime, exec_info[9].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.notificationAddr, exec_info[10].c_str(), NAME_LEN);

        ix::key_value_proxy kvp{rule_exec_submit_inp.condInput};
        kvp[RULE_EXECUTION_CONTEXT_KW] = exec_info[11];

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
                logger::delay_server::error("{}:{} - rcRuleExecDel failed {} for id {}",
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
                logger::delay_server::error("{}:{} - rcRuleExecMod failed {} for id {}",
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
                logger::delay_server::error("{}:{} - getNextRepeatTime returned unknown value {} for id {}",
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
            rule_exec_submit_inp = fill_rule_exec_submit_inp(rule_id);
        }
        catch (const irods::exception& e) {
            logger::delay_server::error(e.what());
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

        if (!delay_server_terminated) {
            queue.dequeue_rule(std::string(rule_exec_submit_inp.ruleExecId));
        }
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

            logger::delay_server::debug("Enqueueing rule [{}]", rule_id);

            try {
                queue.enqueue_rule(rule_id);
            }
            catch (const std::bad_alloc& e) {
                logger::delay_server::trace("Delay queue memory limit reached. Ignoring rule [{}] for now.", rule_id);
                return;
            }

            irods::thread_pool::post(thread_pool, [&queue, result] {
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

    set_ips_display_name(boost::filesystem::path{argv[0]}.filename().c_str());

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
            return irods::get_advanced_setting<const int>(irods::CFG_DELAY_SERVER_SLEEP_TIME_IN_SECONDS);
        }
        catch (const irods::exception& e) {
            logger::delay_server::error(e.what());
        }

        return irods::default_delay_server_sleep_time_in_seconds;
    }();

    const auto go_to_sleep = [&sleep_time] {
        const auto start_time = std::chrono::system_clock::now();
        const auto allowed_sleep_time = std::chrono::seconds{sleep_time};

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
            return irods::get_advanced_setting<const int>(irods::CFG_NUMBER_OF_CONCURRENT_DELAY_RULE_EXECUTORS);
        }
        catch (const irods::exception& e) {
            logger::delay_server::error(e.what());
        }

        return irods::default_number_of_concurrent_delay_executors;
    }();

    irods::thread_pool thread_pool{number_of_concurrent_executors};

    const auto queue_size_in_bytes = []() -> int {
        try {
            const auto bytes = irods::get_advanced_setting<int>(irods::CFG_MAX_SIZE_OF_DELAY_QUEUE_IN_BYTES_KW);

            if (bytes > 0) {
                return bytes;
            }
        }
        catch (const irods::exception& e) {
            logger::delay_server::warn("Could not retrieve delay queue byte limit from configuration. "
                                       "Delay server will use as much memory as necessary.");
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

    return 0;
}

