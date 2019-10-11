#include "connection_pool.hpp"
#include "initServer.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_delay_queue.hpp"
#include "irods_logger.hpp"
#include "irods_query.hpp"
#include "irods_re_structs.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_state.hpp"
#include "irodsReServer.hpp"
#include "miscServerFunct.hpp"
#include "rodsClient.h"
#include "rodsPackTable.h"
#include "rsGlobalExtern.hpp"
#include "rsLog.hpp"
#include "ruleExecDel.h"
#include "ruleExecSubmit.h"
#include "thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/format.hpp>

using logger = irods::experimental::log;
namespace {
    void init_logger() {
        logger::init();
        irods::server_properties::instance().capture();
        logger::server::set_level(logger::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_DELAY_SERVER_KW));
        logger::set_server_type("delay_server");

        if (char hostname[HOST_NAME_MAX]{}; gethostname(hostname, sizeof(hostname)) == 0) {
            logger::set_server_host(hostname);
        }
    }

    ruleExecSubmitInp_t fill_rule_exec_submit_inp(
        const std::vector<std::string>& exec_info) {
        namespace bfs = boost::filesystem;

        ruleExecSubmitInp_t rule_exec_submit_inp{};
        rule_exec_submit_inp.packedReiAndArgBBuf = (bytesBuf_t*)malloc(sizeof(bytesBuf_t));

        const auto& rule_exec_id = exec_info[0].c_str();
        const auto& rei_file_path = exec_info[2].c_str();
        bfs::path p{rei_file_path};
        if (!bfs::exists(p)) {
            const int status{UNIX_FILE_STAT_ERR - errno};
            THROW(status, (boost::format("stat error for rei file [%s], id [%s]") %
                  rei_file_path % rule_exec_id).str());
        }

        rule_exec_submit_inp.packedReiAndArgBBuf->len = static_cast<int>(bfs::file_size(p));
        rule_exec_submit_inp.packedReiAndArgBBuf->buf = malloc(rule_exec_submit_inp.packedReiAndArgBBuf->len + 1);

        int fd = open(rei_file_path, O_RDONLY, 0);
        if (fd < 0) {
            const int status{UNIX_FILE_STAT_ERR - errno};
            THROW(status, (boost::format("open error for rei file [%s]") %
                  rei_file_path).str());
        }

        memset(rule_exec_submit_inp.packedReiAndArgBBuf->buf, 0,
               rule_exec_submit_inp.packedReiAndArgBBuf->len + 1);
        ssize_t status{read(fd, rule_exec_submit_inp.packedReiAndArgBBuf->buf,
                        rule_exec_submit_inp.packedReiAndArgBBuf->len)};
        close(fd);
        if (rule_exec_submit_inp.packedReiAndArgBBuf->len != static_cast<int>(status)) {
            if (status >= 0) {
                THROW(SYS_COPY_LEN_ERR, (boost::format("read error for [%s],toRead [%d], read [%d]") %
                      rei_file_path % rule_exec_submit_inp.packedReiAndArgBBuf->len % status).str());
            }
            status = UNIX_FILE_READ_ERR - errno;
            irods::log(LOG_ERROR, (boost::format("read error for file [%s], status = [%d]") %
                       rei_file_path % status).str());
        }

        rstrcpy(rule_exec_submit_inp.ruleExecId, rule_exec_id, NAME_LEN);
        rstrcpy(rule_exec_submit_inp.ruleName, exec_info[1].c_str(), META_STR_LEN);
        rstrcpy(rule_exec_submit_inp.reiFilePath, rei_file_path, MAX_NAME_LEN);
        rstrcpy(rule_exec_submit_inp.userName, exec_info[3].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeAddress, exec_info[4].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeTime, exec_info[5].c_str(), TIME_LEN);
        rstrcpy(rule_exec_submit_inp.exeFrequency, exec_info[6].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.priority, exec_info[7].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.lastExecTime, exec_info[8].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.exeStatus, exec_info[9].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.estimateExeTime, exec_info[10].c_str(), NAME_LEN);
        rstrcpy(rule_exec_submit_inp.notificationAddr, exec_info[11].c_str(), NAME_LEN);

        return rule_exec_submit_inp;
    }

    int update_entry_for_repeat(
        rcComm_t& _comm,
        ruleExecSubmitInp_t& _inp,
        int _exec_status) {
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
                irods::log(LOG_ERROR, (boost::format(
                           "%s:%d - rcRuleExecDel failed %d for id %s") %
                           __FUNCTION__ % __LINE__ % status % rule_exec_del_inp.ruleExecId).str());
            }
            return status;
        }};

        const auto update_rule_exec_info = [&](const bool repeat_rule) -> int {
            ruleExecModInp_t rule_exec_mod_inp{};
            rstrcpy(rule_exec_mod_inp.ruleId, _inp.ruleExecId, NAME_LEN);

            addKeyVal(&rule_exec_mod_inp.condInput, RULE_LAST_EXE_TIME_KW, current_time);
            addKeyVal(&rule_exec_mod_inp.condInput, RULE_EXE_TIME_KW, next_time);
            if(repeat_rule) {
                addKeyVal(&rule_exec_mod_inp.condInput, RULE_EXE_FREQUENCY_KW, ef_string);
            }
            const int status = rcRuleExecMod(&_comm, &rule_exec_mod_inp);
            if (status < 0) {
                irods::log(LOG_ERROR, (boost::format(
                           "%s:%d - rcRuleExecMod failed %d for id %s") %
                           __FUNCTION__ % __LINE__ % status % rule_exec_mod_inp.ruleId).str());
            }
            if (rule_exec_mod_inp.condInput.len > 0) {
                clearKeyVal(&rule_exec_mod_inp.condInput);
            }
            return status;
        };

        logger::delay_server::debug((boost::format("[%s:%d] - time:[%s],ef:[%s],next:[%s]") %
            __FUNCTION__ % __LINE__ % current_time % ef_string % next_time).str());
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
                irods::log(LOG_ERROR, (boost::format(
                           "%s:%d - getNextRepeatTime returned unknown value %d for id %s") %
                           __FUNCTION__ % __LINE__ % repeat_status % _inp.ruleExecId).str());
                return repeat_status; 
        }
    }

    exec_rule_expression_t pack_exec_rule_expression(
        ruleExecSubmitInp_t& _inp) {
        exec_rule_expression_t exec_rule{};

        int packed_rei_len = _inp.packedReiAndArgBBuf->len;
        exec_rule.packed_rei_.len = packed_rei_len;
        exec_rule.packed_rei_.buf = _inp.packedReiAndArgBBuf->buf;

        size_t rule_len = strlen(_inp.ruleName);
        exec_rule.rule_text_.buf = (char*)malloc(rule_len+1);
        exec_rule.rule_text_.len = rule_len+1;
        rstrcpy( (char*)exec_rule.rule_text_.buf, _inp.ruleName, rule_len+1);
        return exec_rule;
    }

    int run_rule_exec(
        rcComm_t& _comm,
        ruleExecSubmitInp_t& _inp) {
        // unpack the rei to get the user information
        ruleExecInfoAndArg_t* rei_and_arg{};
        int status = unpackStruct(
                         _inp.packedReiAndArgBBuf->buf,
                         (void**)&rei_and_arg,
                         "ReiAndArg_PI",
                         RodsPackTable,
                         NATIVE_PROT);
        if (status < 0) {
            logger::delay_server::error((boost::format("[%s] - unpackStruct error. status [%d]") %
                __FUNCTION__ % status).str());
            return status;
        }

        // set the proxy user from the rei before delegating to the agent
        // following behavior from touchupPackedRei
        _comm.proxyUser = *rei_and_arg->rei->uoic;

        exec_rule_expression_t exec_rule = pack_exec_rule_expression(_inp);
        exec_rule.params_ = rei_and_arg->rei->msParamArray;
        irods::at_scope_exit<std::function<void()>> at_scope_exit{[&exec_rule, &rei_and_arg] {
            clearBBuf(&exec_rule.rule_text_);
            if(rei_and_arg->rei) {
                if(rei_and_arg->rei->rsComm) {
                    free(rei_and_arg->rei->rsComm);
                }
                freeRuleExecInfoStruct(rei_and_arg->rei, (FREE_MS_PARAM | FREE_DOINP));
            }
            free(rei_and_arg);
        }};

        status = rcExecRuleExpression(&_comm, &exec_rule);
        if (strlen(_inp.exeFrequency) > 0) {
            return update_entry_for_repeat(_comm, _inp, status);
        }
        else if(status < 0) {
            logger::delay_server::error((boost::format("ruleExec of %s: %s failed.") %
                _inp.ruleExecId % _inp.ruleName).str());
            ruleExecDelInp_t rule_exec_del_inp{};
            rstrcpy(rule_exec_del_inp.ruleExecId, _inp.ruleExecId, NAME_LEN);
            status = rcRuleExecDel(&_comm, &rule_exec_del_inp);
            if (status < 0) {
                logger::delay_server::error((boost::format("rcRuleExecDel failed for %s, stat=%d") %
                    _inp.ruleExecId % status).str());
                // Establish a new connection as the original may be invalid
                rodsEnv env{};
                _getRodsEnv(env);
                auto tmp_pool = std::make_shared<irods::connection_pool>(
                    1,
                    env.rodsHost,
                    env.rodsPort,
                    env.rodsUserName,
                    env.rodsZone,
                    env.irodsConnectionPoolRefreshTime);
                status = rcRuleExecDel(&static_cast<rcComm_t&>(tmp_pool->get_connection()), &rule_exec_del_inp);
                if (status < 0) {
                    rodsLog(LOG_ERROR,
                            "rcRuleExecDel failed again for %s, stat=%d - exiting",
                            _inp.ruleExecId, status);
                }
            }
            return status;
        }
        else {
            // Success - remove rule from catalog
            ruleExecDelInp_t rule_exec_del_inp{};
            rstrcpy(rule_exec_del_inp.ruleExecId, _inp.ruleExecId, NAME_LEN);
            status = rcRuleExecDel(&_comm, &rule_exec_del_inp);
            if(status < 0) {
                logger::delay_server::error((boost::format("Failed deleting rule exec %s from catalog") %
                    rule_exec_del_inp.ruleExecId).str());
            }
            return status;
        }
    }

    // Task posted to io_service
    void execute_rule(
        std::shared_ptr<irods::connection_pool> _conn_pool,
        irods::delay_queue& _queue,
        const std::vector<std::string>& _rule_info,
        const std::atomic_bool& _re_server_terminated) {
        if (_re_server_terminated) {
            return;
        }
        // Prepare input for rule execution API call
        ruleExecSubmitInp_t rule_exec_submit_inp{};

        irods::at_scope_exit<std::function<void()>> at_scope_exit{[&rule_exec_submit_inp] {
            freeBBuf(rule_exec_submit_inp.packedReiAndArgBBuf);
        }};

        try{
            rule_exec_submit_inp = fill_rule_exec_submit_inp(_rule_info);
        } catch(const irods::exception& e) {
            irods::log(e);
            return;
        }

        try {
            logger::delay_server::trace((boost::format("Executing rule [%s]") % rule_exec_submit_inp.ruleExecId).str());
            int status = run_rule_exec(_conn_pool->get_connection(), rule_exec_submit_inp);
            if(status < 0) {
                logger::delay_server::error((boost::format("Rule exec for [%s] failed. status = [%d]") %
                    rule_exec_submit_inp.ruleExecId % status).str());
            }
        } catch(const std::exception& e) {
            logger::delay_server::error((boost::format("Exception caught during execution of rule [%s]: [%s]") %
                rule_exec_submit_inp.ruleExecId % e.what()).str());
        }

        _queue.dequeue_rule(std::string(rule_exec_submit_inp.ruleExecId));
    }
}

int main() {
    init_logger();
    logger::delay_server::debug("Initializing...");

    static std::atomic_bool re_server_terminated{};
    const auto signal_exit_handler = [](int signal) {
        logger::delay_server::error((boost::format("RE server received signal [%d]") % signal).str());
        re_server_terminated = true;
    };
    signal(SIGINT, signal_exit_handler);
    signal(SIGHUP, signal_exit_handler);
    signal(SIGTERM, signal_exit_handler);
    signal(SIGUSR1, signal_exit_handler);

    const auto sleep_time = [] {
        int sleep_time = irods::default_re_server_sleep_time;
        try {
            sleep_time = irods::get_advanced_setting<const int>(irods::CFG_RE_SERVER_SLEEP_TIME);
        } catch (const irods::exception& e) {
            irods::log(e);
        }
        return sleep_time;
    }();

    const auto thread_count = [] {
        int thread_count = irods::default_max_number_of_concurrent_re_threads;
        try {
            thread_count = irods::get_advanced_setting<const int>(irods::CFG_MAX_NUMBER_OF_CONCURRENT_RE_PROCS);
        } catch (const irods::exception& e) {
            irods::log(e);
        }
        return thread_count;
    }();

    irods::thread_pool thread_pool{thread_count};

    irods::delay_queue queue;

    while(!re_server_terminated) {
        try {
            rodsEnv env{};
            _getRodsEnv(env);
            auto query_conn_pool = std::make_shared<irods::connection_pool>(
                1,
                env.rodsHost,
                env.rodsPort,
                env.rodsUserName,
                env.rodsZone,
                env.irodsConnectionPoolRefreshTime);
            auto query_conn = query_conn_pool->get_connection();
            const auto now = std::to_string(std::time(nullptr));
            const auto qstr = (boost::format(
                "SELECT RULE_EXEC_ID, \
                        RULE_EXEC_NAME, \
                        RULE_EXEC_REI_FILE_PATH, \
                        RULE_EXEC_USER_NAME, \
                        RULE_EXEC_ADDRESS, \
                        RULE_EXEC_TIME, \
                        RULE_EXEC_FREQUENCY, \
                        RULE_EXEC_PRIORITY, \
                        RULE_EXEC_LAST_EXE_TIME, \
                        RULE_EXEC_STATUS, \
                        RULE_EXEC_ESTIMATED_EXE_TIME, \
                        RULE_EXEC_NOTIFICATION_ADDR \
                WHERE RULE_EXEC_TIME <= '%s'") % now).str();
            irods::query<rcComm_t> qobj{&static_cast<rcComm_t&>(query_conn), qstr};
            if(qobj.size() > 0) {
                rodsEnv env{};
                _getRodsEnv(env);
                auto conn_pool = std::make_shared<irods::connection_pool>(
                    std::min<int>(thread_count, qobj.size()),
                    env.rodsHost,
                    env.rodsPort,
                    env.rodsUserName,
                    env.rodsZone,
                    env.irodsConnectionPoolRefreshTime);
                for(const auto& result: qobj) {
                    const auto& rule_id = result[0];
                    if(queue.contains_rule_id(rule_id)) {
                        continue;
                    }
                    logger::delay_server::trace((boost::format("Enqueueing rule [%s]") % rule_id).str());
                    queue.enqueue_rule(rule_id);
                    irods::thread_pool::post(thread_pool, [conn_pool, &queue, result] {
                        execute_rule(conn_pool, queue, result, re_server_terminated);
                    });
                }
            }
        } catch(const std::exception& e) {
            irods::log(LOG_ERROR, e.what());
        } catch(const irods::exception& e) {
            irods::log(e);
        }
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
    }
}
