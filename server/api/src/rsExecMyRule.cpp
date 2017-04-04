#include "execMyRule.h"
#include "miscServerFunct.hpp"
#include "rcMisc.h"
#include "irods_re_plugin.hpp"
#include "rsExecMyRule.hpp"

extern std::unique_ptr<struct irods::global_re_plugin_mgr> irods::re_plugin_globals;

int rsExecMyRule(
    rsComm_t*        _comm,
    execMyRuleInp_t* _exec_inp,
    msParamArray_t** _out_arr ) {

    if ( _exec_inp == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsExecMyRule error. NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    char* available_str = getValByKey(
                            &_exec_inp->condInput,
                            "available" );

    if ( available_str ) {
        std::vector< std::string > instance_names;
        irods::error ret = list_rule_plugin_instances( instance_names );
        
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }

        std::string ret_string = "Available rule engine plugin instances:\n";
        for ( auto& name : instance_names ) {
            ret_string += "\t";
            ret_string += name;
            ret_string += "\n";
        }

        int i = addRErrorMsg( &_comm->rError, 0, ret_string.c_str() );
        if (i) {
        irods::log( ERROR( i, "addRErrorMsg failed" ) );
        }
        return i;
    }

    rodsServerHost_t* rods_svr_host = nullptr;
    int remoteFlag = resolveHost( &_exec_inp->addr, &rods_svr_host );

    if ( remoteFlag == REMOTE_HOST ) {
        return remoteExecMyRule( _comm, _exec_inp,
                                   _out_arr, rods_svr_host );
    }

    char* inst_name_str = getValByKey(
                              &_exec_inp->condInput,
                              irods::CFG_INSTANCE_NAME_KW.c_str() );
    std::string inst_name;
    if( inst_name_str ) {
        inst_name = inst_name_str;
    }
    else {
        irods::error ret = get_default_rule_plugin_instance( inst_name );
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }
    }

    ruleExecInfo_t rei;
    initReiWithDataObjInp( &rei, _comm, NULL );
    rei.condInputData = &_exec_inp->condInput;

    /* need to have a non zero inpParamArray for execMyRule to work */
    if ( _exec_inp->inpParamArray == NULL ) {
        _exec_inp->inpParamArray =
            ( msParamArray_t * ) malloc( sizeof( msParamArray_t ) );
        memset( _exec_inp->inpParamArray, 0, sizeof( msParamArray_t ) );
    }
    rei.msParamArray = _exec_inp->inpParamArray;

    rstrcpy( rei.ruleName, EXEC_MY_RULE_KW, NAME_LEN );

    const std::string& my_rule_text   = _exec_inp->myRule;
    const std::string& out_param_desc = _exec_inp->outParamDesc;
    irods::rule_engine_context_manager<
        irods::unit,
        ruleExecInfo_t*,
        irods::AUDIT_RULE> re_ctx_mgr(
                               irods::re_plugin_globals->global_re_mgr,
                               &rei);
    irods::error err = re_ctx_mgr.exec_rule_text(
                           inst_name,
                           my_rule_text,
                           _exec_inp->inpParamArray,
                           out_param_desc);
    if(!err.ok()) {
        rodsLog(
            LOG_ERROR,
            "%s : %d, %s",
            __FUNCTION__,
            err.code(),
            err.result().c_str()
        );
        return err.code();
    }

    trimMsParamArray( rei.msParamArray, _exec_inp->outParamDesc );

    *_out_arr = rei.msParamArray;
    rei.msParamArray = NULL;

    if ( err.code() < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsExecMyRule : execMyRule error for %s, status = %d",
                 _exec_inp->myRule, err.code() );
        return err.code();
    }

    return err.code();
}

int
remoteExecMyRule( rsComm_t *_comm, execMyRuleInp_t *_exec_inp,
                  msParamArray_t **_out_arr, rodsServerHost_t *rods_svr_host ) {
    int status;

    if ( rods_svr_host == NULL ) {
        rodsLog( LOG_ERROR,
                 "remoteExecMyRule: Invalid rods_svr_host" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( _comm, rods_svr_host ) ) < 0 ) {
        return status;
    }

    status = rcExecMyRule( rods_svr_host->conn, _exec_inp, _out_arr );

    return status;
}
