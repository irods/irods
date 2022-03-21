#include "irods/irods_re_structs.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/irods_re_ruleexistshelper.hpp"
#include "irods/rcMisc.h"
#include "irods/packStruct.h"
#include "irods/dataObjOpen.h"
#include "irods/dataObjWrite.h"
#include "irods/dataObjClose.h"
#include "irods/dataObjLseek.h"
#include "irods/fileLseek.h"
#include "irods/rodsErrorTable.h"
#include "irods/ruleExecSubmit.h"
#include "irods/rsDataObjOpen.hpp"
#include "irods/rsDataObjLseek.hpp"
#include "irods/rsDataObjWrite.hpp"
#include "irods/rsDataObjClose.hpp"

#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_logger.hpp"
#include "irods/dstream.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#define IRODS_IO_TRANSPORT_ENABLE_SERVER_SIDE_API
#include "irods/transport/default_transport.hpp"

#include <list>

extern int ProcessType;

extern const packInstruct_t RodsPackTable[];
irods::ms_table& get_microservice_table();

// =-=-=-=-=-=-=-
// function to look up and / or load a microservice for execution
int actionTableLookUp( irods::ms_table_entry& _entry, char* _action ) {
    irods::ms_table& MicrosTable = get_microservice_table();

    std::string str_act( _action );

    if ( str_act[0] == 'a' && str_act[1] == 'c' ) {
        return -1;
    }

    // =-=-=-=-=-=-=
    // look up Action in microservice table.  If it returns
    // the end() iterator, is is not found so try to load it.
    if ( !MicrosTable.has_entry( str_act ) ) {
        rodsLog( LOG_DEBUG, "actionTableLookUp - [%s] not found, load it.", _action );
        irods::error ret = irods::load_microservice_plugin( MicrosTable, str_act );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return UNMATCHED_ACTION_ERR;
        }
        else {   // if loaded
            rodsLog( LOG_DEBUG, "actionTableLookUp - loaded [%s]", _action );
        } // else
    }  // if not found

    _entry = *MicrosTable[ str_act ];

    return 0;

} // actionTableLookUp

int
applyRule(const char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag ) {
    // special case for libmso to work
    //if(strchr(inAction, '(') != NULL) {
    //    return applyRule331(inAction, inMsParamArray, rei, reiSaveFlag);
    //}
    if (!RuleExistsHelper::Instance()->checkOperation(inAction)) {
        return SUCCESS().code();
    }

    (void) reiSaveFlag;
    (void) inMsParamArray;
    irods::rule_engine_context_manager<
        irods::unit,
        ruleExecInfo_t*,
        irods::AUDIT_RULE> re_ctx_mgr(
                               irods::re_plugin_globals->global_re_mgr,
                               rei);
    irods::error err = re_ctx_mgr.exec_rule(inAction);
    if(!err.ok()) {
        rodsLog(
            LOG_ERROR,
            "applyRule: %d, %s",
            err.code(),
            err.result().c_str()
        );
    }
    return err.code();
}

int
applyRuleArg( const char *action, const char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
              ruleExecInfo_t *rei, int reiSaveFlag ) {
    if (!RuleExistsHelper::Instance()->checkOperation(action)) {
        return SUCCESS().code();
    }

    (void) reiSaveFlag;
    irods::rule_engine_context_manager<
        irods::unit,
        ruleExecInfo_t*,
        irods::AUDIT_RULE> re_ctx_mgr(
                               irods::re_plugin_globals->global_re_mgr,
                               rei);
    std::list<boost::any> args2;
    for(int i = 0; i<argc;i++) {
        args2.push_back(boost::any(std::string(args[i])));
    }
    irods::error err = re_ctx_mgr.exec_rule(action, irods::unpack(args2));
    if(!err.ok()) {
        rodsLog(
            LOG_ERROR,
            "applyRuleArg: %d, %s",
            err.code(),
            err.result().c_str()
        );
    }
    return err.code();
}

int applyRuleWithInOutVars(
    const char*            _action,
    std::list<boost::any>& _params,
    ruleExecInfo_t*        _rei ) {
    if (!RuleExistsHelper::Instance()->checkOperation(_action)) {
        return SUCCESS().code();
    }

    irods::rule_engine_context_manager<
        irods::unit,
        ruleExecInfo_t*,
        irods::AUDIT_RULE> re_ctx_mgr(
                               irods::re_plugin_globals->global_re_mgr,
                               _rei);
    irods::error err = re_ctx_mgr.exec_rule(_action, irods::unpack(_params));
    if(!err.ok()) {
        rodsLog(
            LOG_ERROR,
            "applyRuleWithInOutVars: %d, %s",
            err.code(),
            err.result().c_str()
        );
    }
    return err.code();
}

void freeCmdExecOut( execCmdOut_t *ruleExecOut ) {
    if ( ruleExecOut != NULL ) {
        if ( ruleExecOut->stdoutBuf.buf != 0 ) {
            free( ruleExecOut->stdoutBuf.buf );
        }
        if ( ruleExecOut->stderrBuf.buf != 0 ) {
            free( ruleExecOut->stderrBuf.buf );
        }
        free( ruleExecOut );
    }

}

int
initReiWithDataObjInp( ruleExecInfo_t *rei, rsComm_t *rsComm,
                       dataObjInp_t *dataObjInp ) {
    memset( rei, 0, sizeof( ruleExecInfo_t ) );
    rei->doinp = dataObjInp;
    rei->rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei->uoic = &rsComm->clientUser;
        rei->uoip = &rsComm->proxyUser;
    }

    // also initialize condInputData for resource session variables
    rei->condInputData = (keyValPair_t *)malloc(sizeof(keyValPair_t));
    memset(rei->condInputData, 0, sizeof(keyValPair_t));

    return 0;
}

int
initReiWithCollInp( ruleExecInfo_t *rei, rsComm_t *rsComm,
                    collInp_t *collCreateInp, collInfo_t *collInfo ) {
    int status;

    memset( rei, 0, sizeof( ruleExecInfo_t ) );
    memset( collInfo, 0, sizeof( collInfo_t ) );
    rei->coi = collInfo;
    /* try to fill out as much info in collInfo as possible */
    if ( ( status = splitPathByKey( collCreateInp->collName,
                                    collInfo->collParentName, MAX_NAME_LEN, collInfo->collName, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "initReiWithCollInp: splitPathByKey for %s error, status = %d",
                 collCreateInp->collName, status );
        return status;
    }
    else {
        rstrcpy( collInfo->collName, collCreateInp->collName, MAX_NAME_LEN );
    }
    rei->rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei->uoic = &rsComm->clientUser;
        rei->uoip = &rsComm->proxyUser;
    }

    return 0;
}

namespace
{
    int append_message_to_data_object(ruleExecInfo_t* _rei, const char* _path, char* _message)
    {
        using log = irods::experimental::log;

        if (!_rei || !_rei->rsComm) {
            log::rule_engine::error("Rule execution info pointer or connection pointer is null");
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        try {
            using odstream = irods::experimental::io::odstream;
            using default_transport = irods::experimental::io::server::default_transport;

            default_transport tp{*_rei->rsComm};
            odstream out{tp, _path, std::ios_base::in | std::ios_base::ate};

            if (!out) {
                // clang-format off
                log::rule_engine::error({{"log_message", "Cannot open data object for writing"},
                                         {"path", _path}});
                // clang-format on
                return USER_FILE_DOES_NOT_EXIST;
            }

            out << _message;
        }
        catch (const std::exception& e) {
            log::rule_engine::error(e.what());
        }

        return 0;
    }

    void append_message_to_buffer(ruleExecInfo_t* _rei, const char* _target, char* _message)
    {
        execCmdOut_t* myExecCmdOut{};
        msParamArray_t* inMsParamArray = _rei->msParamArray;
        msParam_t* mP = getMsParamByLabel(inMsParamArray, "ruleExecOut");

        if (mP && mP->inOutStruct) {
            if (strcmp(mP->type, STR_MS_T) == 0) {
                myExecCmdOut = (execCmdOut_t*) malloc(sizeof(execCmdOut_t));
                memset(myExecCmdOut, 0, sizeof(execCmdOut_t));
                mP->inOutStruct = myExecCmdOut;
                mP->type = strdup(ExecCmdOut_MS_T);
            }
            else {
                myExecCmdOut = (execCmdOut_t*) mP->inOutStruct;
            }
        }
        else {
            myExecCmdOut = (execCmdOut_t*) malloc(sizeof(execCmdOut_t));
            memset(myExecCmdOut, 0, sizeof(execCmdOut_t));

            if (!mP) {
                addMsParam(inMsParamArray, "ruleExecOut", ExecCmdOut_MS_T, myExecCmdOut, nullptr);
            }
            else {
                mP->inOutStruct = myExecCmdOut;
                mP->type = strdup(ExecCmdOut_MS_T);
            }
        }

        if (strcmp(_target, "stdout") == 0) {
            appendToByteBuf(&myExecCmdOut->stdoutBuf, _message);
        }
        else if (strcmp(_target, "stderr") == 0) {
            appendToByteBuf(&myExecCmdOut->stderrBuf, _message);
        }
    }
} // anonymous namespace

int _writeString( char *writeId, char *writeStr, ruleExecInfo_t *rei )
{
    using log = irods::experimental::log;

    if (!writeId) {
        log::rule_engine::error("Output target is null");
        return 0;
    }

    if (std::strcmp(writeId, "serverLog") == 0) {
        log::rule_engine::info("writeString: " + std::string{writeStr});
        return 0;
    }

    if (!rei->rsComm) {
        log::rule_engine::error("Connection object is null");
        return 0;
    }

    namespace fs = irods::experimental::filesystem::server;

    if (fs::is_data_object(*rei->rsComm, writeId)) {
        return append_message_to_data_object(rei, writeId, writeStr);
    }

    if (strcmp(writeId, "stdout") == 0 || strcmp(writeId, "stderr") == 0) {
        append_message_to_buffer(rei, writeId, writeStr);
    }
    else {
        // clang-format off
        log::rule_engine::error({{"log_message", "Invalid output target"},
                                 {"output_target", writeId}});
        // clang-format on
    }

    return 0;
}

int writeString( msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei ) {
    int i;
    char *writeId;
    char *writeStr;

    if ( where->inOutStruct == NULL ) {
        writeId = where->label;
    }
    else {
        writeId = ( char* )where->inOutStruct;
    }

    if ( inString->inOutStruct == NULL ) {
        writeStr = strdup( ( char * ) inString->label );
    }
    else {
        writeStr = strdup( ( char * ) inString->inOutStruct );
    }
    i = _writeString( writeId, writeStr, rei );

    free( writeStr );
    return i;
}

int
touchupPackedRei( rsComm_t *rsComm, ruleExecInfo_t *myRei ) {
    int status = 0;

    if ( myRei == NULL || rsComm == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( myRei->rsComm != NULL ) {
        free( myRei->rsComm );
    }

    myRei->rsComm = rsComm;
    /* copy the clientUser. proxyUser is assumed to be in rsComm already */
    rsComm->clientUser = *( myRei->uoic );

    if ( myRei->doi != NULL ) {
        if ( myRei->doi->next != NULL ) {
            free( myRei->doi->next );
            myRei->doi->next = NULL;
        }
    }

    return status;
}

int
unpackRei( rsComm_t *rsComm, ruleExecInfo_t **rei,
           bytesBuf_t *packedReiBBuf ) {
    int status;

    if ( packedReiBBuf == NULL || rei == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* unpack the rei */

    /* always use NATIVE_PROT for rei */
    status = unpack_struct( packedReiBBuf->buf, ( void ** ) rei,
                           "Rei_PI", RodsPackTable, NATIVE_PROT, nullptr );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "unpackRei: unpackStruct error. status = %d", status );
        return status;
    }

    /* try to touch up the unpacked rei */

    status = touchupPackedRei( rsComm, *rei );

    return status;
}

int
packReiAndArg( ruleExecInfo_t *rei, char *myArgv[],
               int myArgc, bytesBuf_t **packedReiAndArgBBuf ) {

    if (!rei) {
        rodsLog( LOG_ERROR,
                 "packReiAndArg: NULL rei input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( !packedReiAndArgBBuf ) {
        rodsLog( LOG_ERROR,
                 "packReiAndArg: NULL packedReiAndArgBBuf input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( myArgc > 0 && ( myArgv == NULL || *myArgv == NULL ) ) {
        rodsLog( LOG_ERROR,
                 "packReiAndArg: NULL myArgv input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    ruleExecInfoAndArg_t reiAndArg{
        .rei=rei,
        .reArg={
            .myArgc=myArgc,
            .myArgv=myArgv
        }
    };

    /* pack the reiAndArg */

    int status = pack_struct( ( void * ) &reiAndArg, packedReiAndArgBBuf,
                         "ReiAndArg_PI", RodsPackTable, 0, NATIVE_PROT, nullptr );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "packReiAndArg: packStruct error. status = %d", status );
        return status;
    }

    return status;
}

int
unpackReiAndArg( rsComm_t *rsComm, ruleExecInfoAndArg_t **reiAndArg,
                 bytesBuf_t *packedReiAndArgBBuf ) {
    int status;
    /*ruleExecInfo_t *myRei;*/

    if ( packedReiAndArgBBuf == NULL || reiAndArg == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* unpack the reiAndArg */

    status = unpack_struct( packedReiAndArgBBuf->buf, ( void ** ) reiAndArg,
                           "ReiAndArg_PI", RodsPackTable, NATIVE_PROT, nullptr );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "unpackReiAndArg: unpackStruct error. status = %d", status );
        return status;
    }

    /* try to touch up the unpacked rei */

    status = touchupPackedRei( rsComm, ( *reiAndArg )->rei );

    return status;
}

std::map<std::string, std::vector<std::string>>
getTaggedValues(const char *str) {
    std::map<std::string, std::vector<std::string>> ret_map{};
    if (!str) {
        return ret_map;
    }
    std::vector<std::tuple<std::string, const char*>> tag_stack;
    while ((str = strchr(str, '<'))) {
        const char * tag_end = strchr(str, '>');
        if (str[1] == '/') {
            std::string& last_tag = std::get<0>(tag_stack.back());
            if ( last_tag.size() != static_cast<std::size_t>(tag_end - str - 2) || strncmp(last_tag.c_str(), str + 2, last_tag.size())  != 0 ) {
                THROW( INPUT_ARG_NOT_WELL_FORMED_ERR, boost::format("unterminated tag: [%s]") % last_tag);
            }
            ret_map[last_tag].emplace_back(std::get<1>(tag_stack.back()), str);
            tag_stack.pop_back();
        } else {
            tag_stack.emplace_back(std::string{str + 1, tag_end}, tag_end + 1);
        }
        str = tag_end;
    }
    if ( !tag_stack.empty() ) {
        std::string& last_tag = std::get<0>(tag_stack.back());
        THROW( INPUT_ARG_NOT_WELL_FORMED_ERR, boost::format("unterminated tag: [%s]") % last_tag);
    }
    return ret_map;
}

int
fillSubmitConditions( const char *action, const char *inDelayCondition,
                      bytesBuf_t *packedReiAndArgBBuf, ruleExecSubmitInp_t *ruleSubmitInfo,
                      ruleExecInfo_t *rei ) {

    snprintf( ruleSubmitInfo->ruleName, sizeof( ruleSubmitInfo->ruleName ), "%s", action );

    boost::optional<std::map<std::string, std::vector<std::string>>> taggedValues;
    try {
        taggedValues = getTaggedValues(inDelayCondition);
    } catch ( const irods::exception& e) {
        irods::log( irods::error(e) );
        return e.code();
    }

    auto it = taggedValues->find("INST_NAME");
    if ( it != taggedValues->end() ) {
        addKeyVal( &ruleSubmitInfo->condInput, INSTANCE_NAME_KW, it->second.front().c_str() );
        taggedValues->erase(it);
    }

    it = taggedValues->find("ET");
    if ( it != taggedValues->end() ) {
        strncpy(ruleSubmitInfo->exeTime, it->second.front().c_str(), TIME_LEN);
        if ( int i = checkDateFormat( ruleSubmitInfo->exeTime ) ) {
            return i;
        }
        taggedValues->erase(it);
    }

    it = taggedValues->find("EF");
    if ( it != taggedValues->end() ) {
        strncpy(ruleSubmitInfo->exeFrequency, it->second.front().c_str(), NAME_LEN);
        taggedValues->erase(it);
    }

    it = taggedValues->find("PRIORITY");
    if ( it != taggedValues->end() ) {
        strncpy(ruleSubmitInfo->priority, it->second.front().c_str(), NAME_LEN);
        taggedValues->erase(it);
    }

    it = taggedValues->find("PLUSET");
    if ( it != taggedValues->end() ) {
        int status = getOffsetTimeStr( ruleSubmitInfo->exeTime, it->second.front().c_str() );
        if (status < 0) { return status; }
        if ( int i = checkDateFormat( ruleSubmitInfo->exeTime ) ) {
            return i;
        }
        taggedValues->erase(it);
    }

    ruleSubmitInfo->condInput.len = 0;
    ruleSubmitInfo->packedReiAndArgBBuf = packedReiAndArgBBuf;
    if ( strlen( ruleSubmitInfo->userName ) == 0 ) {
        if ( strlen( rei->uoic->userName ) != 0 ) {
            snprintf( ruleSubmitInfo->userName, sizeof( ruleSubmitInfo->userName ),
                    "%s", rei->uoic->userName );
        }
        else if ( strlen( rei->rsComm->clientUser.userName ) != 0 ) {
            snprintf( rei->rsComm->clientUser.userName, sizeof( rei->rsComm->clientUser.userName ),
                    "%s", rei->uoic->userName );
        }
    }
    return 0;
}
