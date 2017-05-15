#include "irods_re_structs.hpp"
#include "irods_re_plugin.hpp"
#include "irods_re_ruleexistshelper.hpp"
#include "rcMisc.h"
#include "packStruct.h"
#include "dataObjOpen.h"
#include "dataObjWrite.h"
#include "dataObjClose.h"
#include "dataObjLseek.h"
#include "fileLseek.h"
#include "ruleExecSubmit.h"
#include "rsDataObjOpen.hpp"
#include "rsDataObjLseek.hpp"
#include "rsDataObjWrite.hpp"
#include "rsDataObjClose.hpp"

#include "irods_ms_plugin.hpp"
#include "irods_stacktrace.hpp"

#include <list>

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
applyRule( char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag ) {
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
            "applyRuleWithOutVars: %d, %s",
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

int _writeString( char *writeId, char *writeStr, ruleExecInfo_t *rei ) {

    // =-=-=-=-=-=-=-
    // JMC - backport 4619
    dataObjInp_t dataObjInp;
    openedDataObjInp_t openedDataObjInp;
    bytesBuf_t tmpBBuf;
    int fd, i;
    // =-=-=-=-=-=-=-

    if ( writeId != NULL && strcmp( writeId, "serverLog" ) == 0 ) {
        rodsLog( LOG_NOTICE, "writeString: inString = %s", writeStr );
        return 0;
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4619
    if ( writeId != NULL && writeId[0] == '/' ) {
        /* writing to an existing iRODS file */

        if ( rei == NULL || rei->rsComm == NULL ) {
            rodsLog( LOG_ERROR, "_writeString: input rei or rsComm is NULL" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        bzero( &dataObjInp, sizeof( dataObjInp ) );
        dataObjInp.openFlags = O_RDWR;
        snprintf( dataObjInp.objPath, MAX_NAME_LEN, "%s", writeId );
        fd = rsDataObjOpen( rei->rsComm, &dataObjInp );
        if ( fd < 0 ) {
            rodsLog( LOG_ERROR, "_writeString: rsDataObjOpen failed. status = %d", fd );
            return fd;
        }

        bzero( &openedDataObjInp, sizeof( openedDataObjInp ) );
        openedDataObjInp.l1descInx = fd;
        openedDataObjInp.offset = 0;
        openedDataObjInp.whence = SEEK_END;
        fileLseekOut_t *dataObjLseekOut = NULL;
        i = rsDataObjLseek( rei->rsComm, &openedDataObjInp, &dataObjLseekOut );
        free( dataObjLseekOut );
        if ( i < 0 ) {
            rodsLog( LOG_ERROR, "_writeString: rsDataObjLseek failed. status = %d", i );
            return i;
        }

        bzero( &openedDataObjInp, sizeof( openedDataObjInp ) );
        openedDataObjInp.l1descInx = fd;
        tmpBBuf.len = openedDataObjInp.len = strlen( writeStr ) + 1;
        tmpBBuf.buf =  writeStr;
        i = rsDataObjWrite( rei->rsComm, &openedDataObjInp, &tmpBBuf );
        if ( i < 0 ) {
            rodsLog( LOG_ERROR, "_writeString: rsDataObjWrite failed. status = %d", i );
            return i;
        }

        bzero( &openedDataObjInp, sizeof( openedDataObjInp ) );
        openedDataObjInp.l1descInx = fd;
        i = rsDataObjClose( rei->rsComm, &openedDataObjInp );
        return i;
    }

    // =-=-=-=-=-=-=-

    msParam_t * mP = NULL;
    msParamArray_t * inMsParamArray = rei->msParamArray;
    execCmdOut_t *myExecCmdOut;
    if ( ( ( mP = getMsParamByLabel( inMsParamArray, "ruleExecOut" ) ) != NULL ) &&
            ( mP->inOutStruct != NULL ) ) {
        if ( !strcmp( mP->type, STR_MS_T ) ) {
            myExecCmdOut = ( execCmdOut_t* )malloc( sizeof( execCmdOut_t ) );
            memset( myExecCmdOut, 0, sizeof( execCmdOut_t ) );
            mP->inOutStruct = myExecCmdOut;
            mP->type = strdup( ExecCmdOut_MS_T );
        }
        else {
            myExecCmdOut = ( execCmdOut_t* )mP->inOutStruct;
        }
    }
    else {
        myExecCmdOut = ( execCmdOut_t* )malloc( sizeof( execCmdOut_t ) );
        memset( myExecCmdOut, 0, sizeof( execCmdOut_t ) );
        if ( mP == NULL ) {
            addMsParam( inMsParamArray, "ruleExecOut", ExecCmdOut_MS_T, myExecCmdOut, NULL );
        }
        else {
            mP->inOutStruct = myExecCmdOut;
            mP->type = strdup( ExecCmdOut_MS_T );
        }
    }

    /***** Jun 27, 2007
           i  = replaceVariablesAndMsParams("",writeStr, rei->msParamArray, rei);
           if (i < 0)
           return i;
    ****/

    if ( writeId != NULL ) {
        if ( !strcmp( writeId, "stdout" ) ) {
            appendToByteBuf( &( myExecCmdOut->stdoutBuf ), ( char * ) writeStr );
        }
        else if ( !strcmp( writeId, "stderr" ) ) {
            appendToByteBuf( &( myExecCmdOut->stderrBuf ), ( char * ) writeStr );
        }
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

    /* alway use NATIVE_PROT for rei */
    status = unpackStruct( packedReiBBuf->buf, ( void ** ) rei,
                           "Rei_PI", RodsPackTable, NATIVE_PROT );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "unpackRei: unpackStruct error. status = %d", status );
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

    int status = packStruct( ( void * ) &reiAndArg, packedReiAndArgBBuf,
                         "ReiAndArg_PI", RodsPackTable, 0, NATIVE_PROT );

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

    status = unpackStruct( packedReiAndArgBBuf->buf, ( void ** ) reiAndArg,
                           "ReiAndArg_PI", RodsPackTable, NATIVE_PROT );

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

    it = taggedValues->find("EA");
    if ( it != taggedValues->end() ) {
        strncpy(ruleSubmitInfo->exeAddress, it->second.front().c_str(), NAME_LEN);
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

    it = taggedValues->find("PRI");
    if ( it != taggedValues->end() ) {
        strncpy(ruleSubmitInfo->priority, it->second.front().c_str(), NAME_LEN);
        taggedValues->erase(it);
    }

    it = taggedValues->find("EET");
    if ( it != taggedValues->end() ) {
        strncpy(ruleSubmitInfo->estimateExeTime, it->second.front().c_str(), NAME_LEN);
        taggedValues->erase(it);
    }

    it = taggedValues->find("NA");
    if ( it != taggedValues->end() ) {
        strncpy(ruleSubmitInfo->notificationAddr, it->second.front().c_str(), NAME_LEN);
        taggedValues->erase(it);
    }

    it = taggedValues->find("PLUSET");
    if ( it != taggedValues->end() ) {
        getOffsetTimeStr( ruleSubmitInfo->exeTime, it->second.front().c_str() );
        if ( int i = checkDateFormat( ruleSubmitInfo->exeTime ) ) {
            return i;
        }
        taggedValues->erase(it);
    }

    it = taggedValues->find("KVALPR");
    std::size_t i = 0;
    if ( it != taggedValues->end() ) {
        for ( const auto& kvp : it->second ) {
            std::size_t equals_index = kvp.find('=');
            if ( equals_index == std::string::npos ) {
                return INPUT_ARG_NOT_WELL_FORMED_ERR;
            }

            std::size_t end_key_index = equals_index;
            do {
                if ( end_key_index == 0 ) {
                    return INPUT_ARG_NOT_WELL_FORMED_ERR;
                }
            } while ( kvp[--end_key_index] == ' '  );
            ruleSubmitInfo->condInput.keyWord[i] = strndup( kvp.c_str(), end_key_index + 1 );

            std::size_t start_val_index = equals_index;
            do {
                if ( start_val_index == kvp.size() - 1 ) {
                    return INPUT_ARG_NOT_WELL_FORMED_ERR;
                }
            } while ( kvp[++start_val_index] == ' ' );

            ruleSubmitInfo->condInput.value[i] = strdup(kvp.c_str() + start_val_index);
            ++i;
        }
    }

    ruleSubmitInfo->condInput.len = i;
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
