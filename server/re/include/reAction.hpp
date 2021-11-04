#ifndef _RE_ACTION_HPP_
#define _RE_ACTION_HPP_

// reAction.hpp - header file for Actions that are 'called' when executing the rules by the rule engine modules

#include "rodsUser.h"
#include "rods.h"
#include "rcGlobalExtern.h"
#include "objInfo.h"
#include "reSysDataObjOpr.hpp"
#include "reDataObjOpr.hpp"
#include "reNaraMetaData.hpp"
#include "reIn2p3SysRule.hpp"
#include "irods_ms_plugin.hpp"

int msiRollback( ruleExecInfo_t *rei );
int msiSetACL( msParam_t *recursiveFlag, msParam_t *accessLevel, msParam_t *userName,
               msParam_t *pathName, ruleExecInfo_t *rei );
int print_hello( ruleExecInfo_t *c );
int print_hello_arg( msParam_t* xs, ruleExecInfo_t *rei );
int delayExec( msParam_t* condition, msParam_t* workflow,
               msParam_t* recoverWorkFlow, ruleExecInfo_t *rei );
int remoteExec( msParam_t* hostName, msParam_t* condition, msParam_t* workflow,
                msParam_t* recoverWorkFlow, ruleExecInfo_t *rei );
int msiSleep( msParam_t* sec, msParam_t* microsec, ruleExecInfo_t *rei );
int writeBytesBuf( msParam_t* where, msParam_t* inBuf, ruleExecInfo_t *rei );
int writePosInt( msParam_t* where, msParam_t* inInt, ruleExecInfo_t *rei );
int writeKeyValPairs( msParam_t *where, msParam_t *inKVPair, msParam_t *separator, ruleExecInfo_t *rei );
int msiGetDiffTime( msParam_t* inpParam1,  msParam_t* inpParam2, msParam_t* inpParam3, msParam_t* outParam, ruleExecInfo_t *rei );
int msiGetSystemTime( msParam_t* outParam,  msParam_t* inpParam, ruleExecInfo_t *rei );
int msiGetFormattedSystemTime( msParam_t* outParam, msParam_t* inpParam, msParam_t* inpFormatParam, ruleExecInfo_t *rei );
int msiHumanToSystemTime( msParam_t* inpParam, msParam_t* outParam, ruleExecInfo_t *rei );
int msiStrToBytesBuf( msParam_t* str_msp, msParam_t* buf_msp, ruleExecInfo_t *rei );
int msiBytesBufToStr( msParam_t* buf_msp, msParam_t* str_msp, ruleExecInfo_t *rei );

int msiApplyDCMetadataTemplate( msParam_t* inpParam, msParam_t* outParam, ruleExecInfo_t *rei );
int msiListEnabledMS( msParam_t *outKVPairs, ruleExecInfo_t *rei );

int msiSetResource( msParam_t* xrescName, ruleExecInfo_t *rei );
int msiPrintKeyValPair( msParam_t* where, msParam_t* inKVPair,  ruleExecInfo_t *rei );
int msiGetValByKey( msParam_t* inKVPair,  msParam_t* inKey, msParam_t* outVal,  ruleExecInfo_t *rei );
int msiAddKeyVal( msParam_t *inKeyValPair, msParam_t *key, msParam_t *value, ruleExecInfo_t *rei );
int msiExecStrCondQuery( msParam_t* queryParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei );
int msiExecGenQuery( msParam_t* genQueryInParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei );
int msiMakeQuery( msParam_t* selectListParam, msParam_t* conditionsParam,
                  msParam_t* queryOutParam, ruleExecInfo_t *rei );
int msiMakeGenQuery( msParam_t* selectListStr, msParam_t* condStr, msParam_t* genQueryInpParam, ruleExecInfo_t *rei );
int msiGetMoreRows( msParam_t *genQueryInp_msp, msParam_t *genQueryOut_msp, msParam_t *continueInx, ruleExecInfo_t *rei );
int msiCloseGenQuery( msParam_t *genQueryInp_msp, msParam_t *genQueryOut_msp, ruleExecInfo_t *rei );
int msiAddSelectFieldToGenQuery( msParam_t *select, msParam_t *function, msParam_t *queryInput, ruleExecInfo_t *rei );
int msiAddConditionToGenQuery( msParam_t *attribute, msParam_t *coperator, msParam_t *value, msParam_t *queryInput, ruleExecInfo_t *rei );
int msiPrintGenQueryOutToBuffer( msParam_t *queryOut, msParam_t *format, msParam_t *buffer, ruleExecInfo_t *rei );

int msiQuota( ruleExecInfo_t *rei );
int msiDeleteUnusedAVUs( ruleExecInfo_t *rei );
int msiGoodFailure( ruleExecInfo_t *rei );
int msiRegisterData( ruleExecInfo_t *rei );
int msiCheckPermission( msParam_t *perm, ruleExecInfo_t *rei );
int msiCheckAccess( msParam_t *inObjName, msParam_t * inOperation,
                    msParam_t * outResult, ruleExecInfo_t *rei );
int msiCheckOwner( ruleExecInfo_t *rei );
int msiCreateUser( ruleExecInfo_t *rei );
int msiCreateCollByAdmin( msParam_t *parColl, msParam_t *childName, ruleExecInfo_t *rei );
int msiCommit( ruleExecInfo_t *rei );
int msiDeleteCollByAdmin( msParam_t *parColl, msParam_t *childName, ruleExecInfo_t *rei );
int msiDeleteUser( ruleExecInfo_t *rei );
int msiAddUserToGroup( msParam_t *msParam, ruleExecInfo_t *rei );
int msiSendMail( msParam_t *toAddr, msParam_t *subjectLine, msParam_t *body, ruleExecInfo_t *rei );
int msiGetObjType( msParam_t *objNameP, msParam_t *objTypeP,
                   ruleExecInfo_t *rei );
int msiAssociateKeyValuePairsToObj( msParam_t *mDP, msParam_t* objP,  msParam_t* typP,
                                    ruleExecInfo_t *rei );
int msiSetKeyValuePairsToObj( msParam_t *mDP, msParam_t* objP,  msParam_t* typP,
                              ruleExecInfo_t *rei );
int msiExtractTemplateMDFromBuf( msParam_t* sOP, msParam_t* tSP,
                                 msParam_t *mDP, ruleExecInfo_t *rei );
int msiReadMDTemplateIntoTagStruct( msParam_t* sOP, msParam_t* tSP, ruleExecInfo_t *rei );
int msiFreeBuffer( msParam_t* memP, ruleExecInfo_t *rei );
int msiGetIcatTime( msParam_t* timeOutParam,  msParam_t* typeInParam, ruleExecInfo_t *rei );

int msiGetTaggedValueFromString( msParam_t *inTagParag, msParam_t *inStrParam,
                                 msParam_t *outValueParam, ruleExecInfo_t *rei );
int recover_print_hello( ruleExecInfo_t *c );

int recover_msiCreateUser( ruleExecInfo_t *rei );
int recover_msiCreateCollByAdmin( msParam_t *parColl, msParam_t *childName, ruleExecInfo_t *rei );

int msiString2KeyValPair( msParam_t *inBufferP, msParam_t* outKeyValPairP, ruleExecInfo_t *rei );
int msiStrArray2String( msParam_t* inSAParam, msParam_t* outStr, ruleExecInfo_t *rei );

int msiAW1( msParam_t* mPIn, msParam_t* mPOut2, ruleExecInfo_t *rei );
int msiRenameLocalZone( msParam_t *oldName, msParam_t *newName,
                        ruleExecInfo_t *rei );
int msiRenameCollection( msParam_t *oldName, msParam_t *newName,
                         ruleExecInfo_t *rei );
int msiRenameLocalZoneCollection(msParam_t* _new_zone_name, ruleExecInfo_t* _rei);
int msiAclPolicy( msParam_t *msParam, ruleExecInfo_t *rei );
int msiSetQuota( msParam_t *type, msParam_t *name, msParam_t *resource,
                 msParam_t *value, ruleExecInfo_t *rei );
int msiRemoveKeyValuePairsFromObj( msParam_t *metadataParam,
                                   msParam_t* objParam,
                                   msParam_t* typeParam,
                                   ruleExecInfo_t *rei );
int msiModAVUMetadata( msParam_t* _item_type,
                       msParam_t* _item_name,
                       msParam_t* _avu_op,
                       msParam_t* _attr_name,
                       msParam_t* _attr_val,
                       msParam_t* _attr_unit,
                       ruleExecInfo_t* _rei   );
int
msiPrintGenQueryInp( msParam_t *where, msParam_t* genQueryInpParam, ruleExecInfo_t *rei );

int
msiGetContInxFromGenQueryOut( msParam_t* genQueryOutParam, msParam_t* continueInx, ruleExecInfo_t *rei );

int msiSetReplComment( msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,
                       msParam_t *inpParam4, ruleExecInfo_t *rei );
int
msiSetBulkPutPostProcPolicy( msParam_t *xflag, ruleExecInfo_t *rei );
int msiCutBufferInHalf( msParam_t* mPIn, ruleExecInfo_t *rei );
int msiDoSomething( msParam_t *inParam, msParam_t *outParam, ruleExecInfo_t *rei );
int msiString2StrArray( msParam_t *inBufferP, msParam_t* outStrArrayP, ruleExecInfo_t *rei );

int msiTakeThreeArgumentsAndDoNothing(msParam_t *arg1, msParam_t *arg2, msParam_t *arg3, ruleExecInfo_t *rei);

namespace irods
{
    // =-=-=-=-=-=-=-
    // implementation of the microservice table class, which initializes the table during the ctor
    // inserting the statically compiled microservices during construction providing RAII behavior
    // NOTE:: this template class is the same as the ms_table typedef from irods_ms_plugin.hpp
    template<>
    lookup_table< irods::ms_table_entry* >::lookup_table() {
        table_[ "print_hello" ] = new irods::ms_table_entry( "print_hello", 0, std::function<int(ruleExecInfo_t*)>( print_hello ) );
        table_[ "print_hello_arg" ] = new irods::ms_table_entry( "print_hello_arg", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( print_hello_arg ) );
        table_[ "msiQuota" ] = new irods::ms_table_entry( "msiQuota", 0, std::function<int(ruleExecInfo_t*)>( msiQuota ) );
        table_[ "msiDeleteUnusedAVUs" ] = new irods::ms_table_entry( "msiDeleteUnusedAVUs", 0, std::function<int(ruleExecInfo_t*)>( msiDeleteUnusedAVUs ) );
        table_[ "msiGoodFailure" ] = new irods::ms_table_entry( "msiGoodFailure", 0, std::function<int(ruleExecInfo_t*)>( msiGoodFailure ) );
        table_[ "msiSetResource" ] = new irods::ms_table_entry( "msiSetResource", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>(  msiSetResource ) );
        table_[ "msiCheckPermission" ] = new irods::ms_table_entry( "msiCheckPermission", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>(  msiCheckPermission ) );
        table_[ "msiCheckAccess" ] = new irods::ms_table_entry( "msiCheckAccess", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>(  msiCheckAccess ) );
        table_[ "msiCheckOwner" ] = new irods::ms_table_entry( "msiCheckOwner", 0, std::function<int(ruleExecInfo_t*)>( msiCheckOwner ) );
        table_[ "msiCreateUser" ] = new irods::ms_table_entry( "msiCreateUser", 0, std::function<int(ruleExecInfo_t*)>( msiCreateUser ) );
        table_[ "msiCreateCollByAdmin" ] = new irods::ms_table_entry( "msiCreateCollByAdmin", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiCreateCollByAdmin ) );
        table_[ "msiSendMail" ] = new irods::ms_table_entry( "msiSendMail", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSendMail ) );
        table_[ "recover_print_hello" ] = new irods::ms_table_entry( "recover_print_hello", 0, std::function<int(ruleExecInfo_t*)>( recover_print_hello ) );
        table_[ "msiCommit" ] = new irods::ms_table_entry( "msiCommit", 0, std::function<int(ruleExecInfo_t*)>( msiCommit ) );
        table_[ "msiRollback" ] = new irods::ms_table_entry( "msiRollback", 0, std::function<int(ruleExecInfo_t*)>( msiRollback ) );
        table_[ "msiDeleteCollByAdmin" ] = new irods::ms_table_entry( "msiDeleteCollByAdmin", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDeleteCollByAdmin ) );
        table_[ "msiDeleteUser" ] = new irods::ms_table_entry( "msiDeleteUser", 0, std::function<int(ruleExecInfo_t*)>( msiDeleteUser ) );
        table_[ "msiAddUserToGroup" ] = new irods::ms_table_entry( "msiAddUserToGroup", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiAddUserToGroup ) );
        table_[ "msiSetDefaultResc" ] = new irods::ms_table_entry( "msiSetDefaultResc", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSetDefaultResc ) );
        table_[ "msiSysReplDataObj" ] = new irods::ms_table_entry( "msiSysReplDataObj", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSysReplDataObj ) );
        table_[ "msiStageDataObj" ] = new irods::ms_table_entry( "msiStageDataObj", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiStageDataObj ) );
        table_[ "msiSetDataObjPreferredResc" ] = new irods::ms_table_entry( "msiSetDataObjPreferredResc", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetDataObjPreferredResc ) );
        table_[ "msiSetDataObjAvoidResc" ] = new irods::ms_table_entry( "msiSetDataObjAvoidResc", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetDataObjAvoidResc ) );
        table_[ "msiSortDataObj" ] = new irods::ms_table_entry( "msiSortDataObj", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSortDataObj ) );
        table_[ "msiSysChksumDataObj" ] = new irods::ms_table_entry( "msiSysChksumDataObj", 0, std::function<int(ruleExecInfo_t*)>( msiSysChksumDataObj ) );
        table_[ "msiSetDataTypeFromExt" ] = new irods::ms_table_entry( "msiSetDataTypeFromExt", 0, std::function<int(ruleExecInfo_t*)>( msiSetDataTypeFromExt ) );
        table_[ "msiSetNoDirectRescInp" ] = new irods::ms_table_entry( "msiSetNoDirectRescInp", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetNoDirectRescInp ) );
        table_[ "msiSetNumThreads" ] = new irods::ms_table_entry( "msiSetNumThreads", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSetNumThreads ) );
        table_[ "msiDeleteDisallowed" ] = new irods::ms_table_entry( "msiDeleteDisallowed", 0, std::function<int(ruleExecInfo_t*)>( msiDeleteDisallowed ) );
        table_[ "msiOprDisallowed" ] = new irods::ms_table_entry( "msiOprDisallowed", 0, std::function<int(ruleExecInfo_t*)>( msiOprDisallowed ) );
        table_[ "msiDataObjCreate" ] = new irods::ms_table_entry( "msiDataObjCreate", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjCreate ) );
        table_[ "msiDataObjOpen" ] = new irods::ms_table_entry( "msiDataObjOpen", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjOpen ) );
        table_[ "msiDataObjClose" ] = new irods::ms_table_entry( "msiDataObjClose", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjClose ) );
        table_[ "msiDataObjLseek" ] = new irods::ms_table_entry( "msiDataObjLseek", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjLseek ) );
        table_[ "msiDataObjRead" ] = new irods::ms_table_entry( "msiDataObjRead", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjRead ) );
        table_[ "msiDataObjWrite" ] = new irods::ms_table_entry( "msiDataObjWrite", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjWrite ) );
        table_[ "msiDataObjUnlink" ] = new irods::ms_table_entry( "msiDataObjUnlink", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjUnlink ) );
        table_[ "msiDataObjRepl" ] = new irods::ms_table_entry( "msiDataObjRepl", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjRepl ) );
        table_[ "msiDataObjCopy" ] = new irods::ms_table_entry( "msiDataObjCopy", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjCopy ) );
        table_[ "msiExtractNaraMetadata" ] = new irods::ms_table_entry( "msiExtractNaraMetadata", 0, std::function<int(ruleExecInfo_t*)>( msiExtractNaraMetadata ) );
        table_[ "msiGetObjType" ] = new irods::ms_table_entry( "msiGetObjType", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetObjType ) );
        table_[ "msiAssociateKeyValuePairsToObj" ] = new irods::ms_table_entry( "msiAssociateKeyValuePairsToObj", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAssociateKeyValuePairsToObj ) );
        table_[ "msiSetKeyValuePairsToObj" ] = new irods::ms_table_entry( "msiSetKeyValuePairsToObj", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSetKeyValuePairsToObj ) );
        table_[ "msiExtractTemplateMDFromBuf" ] = new irods::ms_table_entry( "msiExtractTemplateMDFromBuf", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiExtractTemplateMDFromBuf ) );
        table_[ "msiReadMDTemplateIntoTagStruct" ] = new irods::ms_table_entry( "msiReadMDTemplateIntoTagStruct", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiReadMDTemplateIntoTagStruct ) );
        table_[ "msiDataObjPut" ] = new irods::ms_table_entry( "msiDataObjPut", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjPut ) );
        table_[ "msiDataObjGet" ] = new irods::ms_table_entry( "msiDataObjGet", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjGet ) );
        table_[ "msiDataObjChksum" ] = new irods::ms_table_entry( "msiDataObjChksum", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjChksum ) );
        table_[ "msiDataObjPhymv" ] = new irods::ms_table_entry( "msiDataObjPhymv", 6, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjPhymv ) );
        table_[ "msiDataObjRename" ] = new irods::ms_table_entry( "msiDataObjRename", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjRename ) );
        table_[ "msiDataObjTrim" ] = new irods::ms_table_entry( "msiDataObjTrim", 6, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjTrim ) );
        table_[ "msiCollCreate" ] = new irods::ms_table_entry( "msiCollCreate", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiCollCreate ) );
        table_[ "msiRmColl" ] = new irods::ms_table_entry( "msiRmColl", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiRmColl ) );
        table_[ "msiCollRepl" ] = new irods::ms_table_entry( "msiCollRepl", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiCollRepl ) );
        table_[ "msiPhyPathReg" ] = new irods::ms_table_entry( "msiPhyPathReg", 5, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiPhyPathReg ) );
        table_[ "msiObjStat" ] = new irods::ms_table_entry( "msiObjStat", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiObjStat ) );
        table_[ "msiDataObjRsync" ] = new irods::ms_table_entry( "msiDataObjRsync", 5, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDataObjRsync ) );
        table_[ "msiCollRsync" ] = new irods::ms_table_entry( "msiCollRsync", 5, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiCollRsync ) );
        table_[ "msiFreeBuffer" ] = new irods::ms_table_entry( "msiFreeBuffer", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiFreeBuffer ) );
        table_[ "msiNoChkFilePathPerm" ] = new irods::ms_table_entry( "msiNoChkFilePathPerm", 0, std::function<int(ruleExecInfo_t*)>( msiNoChkFilePathPerm ) );
        table_[ "msiSetChkFilePathPerm" ] = new irods::ms_table_entry( "msiSetChkFilePathPerm", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetChkFilePathPerm ) );
        table_[ "msiNoTrashCan" ] = new irods::ms_table_entry( "msiNoTrashCan", 0, std::function<int(ruleExecInfo_t*)>( msiNoTrashCan ) );
        table_[ "msiSetPublicUserOpr" ] = new irods::ms_table_entry( "msiSetPublicUserOpr", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetPublicUserOpr ) );
        table_[ "delayExec" ] = new irods::ms_table_entry( "delayExec", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( delayExec ) );
        table_[ "remoteExec" ] = new irods::ms_table_entry( "remoteExec", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( remoteExec ) );
        table_[ "msiSleep" ] = new irods::ms_table_entry( "msiSleep", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSleep ) );
        table_[ "writeBytesBuf" ] = new irods::ms_table_entry( "writeBytesBuf", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( writeBytesBuf ) );
        table_[ "writePosInt" ] = new irods::ms_table_entry( "writePosInt", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( writePosInt ) );
        table_[ "writeKeyValPairs" ] = new irods::ms_table_entry( "writeKeyValPairs", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( writeKeyValPairs ) );
        table_[ "msiGetDiffTime" ] = new irods::ms_table_entry( "msiGetDiffTime", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetDiffTime ) );
        table_[ "msiGetSystemTime" ] = new irods::ms_table_entry( "msiGetSystemTime", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetSystemTime ) );
        table_[ "msiHumanToSystemTime" ] = new irods::ms_table_entry( "msiHumanToSystemTime", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiHumanToSystemTime ) );
        table_[ "msiGetFormattedSystemTime" ] = new irods::ms_table_entry( "msiGetFormattedSystemTime", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetFormattedSystemTime ) );
        table_[ "msiStrToBytesBuf" ] = new irods::ms_table_entry( "msiStrToBytesBuf", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrToBytesBuf ) );
        table_[ "msiBytesBufToStr" ] = new irods::ms_table_entry( "msiBytesBufToStr", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiBytesBufToStr ) );
        table_[ "msiApplyDCMetadataTemplate" ] = new irods::ms_table_entry( "msiApplyDCMetadataTemplate", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiApplyDCMetadataTemplate ) );
        table_[ "msiListEnabledMS" ] = new irods::ms_table_entry( "msiListEnabledMS", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiListEnabledMS ) );
        table_[ "msiPrintKeyValPair" ] = new irods::ms_table_entry( "msiPrintKeyValPair", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiPrintKeyValPair ) );
        table_[ "msiGetValByKey" ] = new irods::ms_table_entry( "msiGetValByKey", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetValByKey ) );
        table_[ "msiAddKeyVal" ] = new irods::ms_table_entry( "msiAddKeyVal", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAddKeyVal ) );
        table_[ "msiExecStrCondQuery" ] = new irods::ms_table_entry( "msiExecStrCondQuery", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiExecStrCondQuery ) );
        table_[ "msiExecGenQuery" ] = new irods::ms_table_entry( "msiExecGenQuery", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiExecGenQuery ) );
        table_[ "msiMakeQuery" ] = new irods::ms_table_entry( "msiMakeQuery", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiMakeQuery ) );
        table_[ "msiMakeGenQuery" ] = new irods::ms_table_entry( "msiMakeGenQuery", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiMakeGenQuery ) );
        table_[ "msiGetMoreRows" ] = new irods::ms_table_entry( "msiGetMoreRows", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetMoreRows ) );
        table_[ "msiCloseGenQuery" ] = new irods::ms_table_entry( "msiCloseGenQuery", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiCloseGenQuery ) );
        table_[ "msiAddSelectFieldToGenQuery" ] = new irods::ms_table_entry( "msiAddSelectFieldToGenQuery", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAddSelectFieldToGenQuery ) );
        table_[ "msiAddConditionToGenQuery" ] = new irods::ms_table_entry( "msiAddConditionToGenQuery", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAddConditionToGenQuery ) );
        table_[ "msiPrintGenQueryOutToBuffer" ] = new irods::ms_table_entry( "msiPrintGenQueryOutToBuffer", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiPrintGenQueryOutToBuffer ) );
        table_[ "msiExecCmd" ] = new irods::ms_table_entry( "msiExecCmd", 6, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiExecCmd ) );
        table_[ "msiSetGraftPathScheme" ] = new irods::ms_table_entry( "msiSetGraftPathScheme", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSetGraftPathScheme ) );
        table_[ "msiSetRandomScheme" ] = new irods::ms_table_entry( "msiSetRandomScheme", 0, std::function<int(ruleExecInfo_t*)>( msiSetRandomScheme ) );
        table_[ "msiCheckHostAccessControl" ] = new irods::ms_table_entry( "msiCheckHostAccessControl", 0, std::function<int(ruleExecInfo_t*)>( msiCheckHostAccessControl ) );
        table_[ "msiGetIcatTime" ] = new irods::ms_table_entry( "msiGetIcatTime", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetIcatTime ) );
        table_[ "msiGetTaggedValueFromString" ] = new irods::ms_table_entry( "msiGetTaggedValueFromString", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetTaggedValueFromString ) );
        table_[ "msiString2KeyValPair" ] = new irods::ms_table_entry( "msiString2KeyValPair", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiString2KeyValPair ) );
        table_[ "msiStrArray2String" ] = new irods::ms_table_entry( "msiStrArray2String", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrArray2String ) );
        table_[ "msiString2StrArray" ] = new irods::ms_table_entry( "msiString2StrArray", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiString2StrArray ) );
        table_[ "msiAW1" ] = new irods::ms_table_entry( "msiAW1", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAW1 ) );
        table_[ "msiRenameLocalZone" ] = new irods::ms_table_entry( "msiRenameLocalZone", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiRenameLocalZone ) );
        table_[ "msiRenameCollection" ] = new irods::ms_table_entry( "msiRenameCollection", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiRenameCollection ) );
        table_[ "msiRenameLocalZoneCollection" ] = new irods::ms_table_entry( "msiRenameLocalZoneCollection", 1, std::function<int(msParam_t*, ruleExecInfo_t*)>( msiRenameLocalZoneCollection ) );
        table_[ "msiAclPolicy" ] = new irods::ms_table_entry( "msiAclPolicy", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>(msiAclPolicy ) );
        table_[ "msiSetQuota" ] = new irods::ms_table_entry( "msiSetQuota", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>(msiSetQuota ) );
        table_[ "msiRemoveKeyValuePairsFromObj" ] = new irods::ms_table_entry( "msiRemoveKeyValuePairsFromObj", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiRemoveKeyValuePairsFromObj ) );
        table_[ "msiModAVUMetadata" ] = new irods::ms_table_entry( "msiModAVUMetadata", 6, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiModAVUMetadata ) );
        table_[ "msiSetReServerNumProc" ] = new irods::ms_table_entry( "msiSetReServerNumProc", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetReServerNumProc ) );
        table_[ "msiPrintGenQueryInp" ] = new irods::ms_table_entry( "msiPrintGenQueryInp", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiPrintGenQueryInp ) );
        table_[ "msiTarFileExtract" ] = new irods::ms_table_entry( "msiTarFileExtract", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiTarFileExtract ) );
        table_[ "msiTarFileCreate" ] = new irods::ms_table_entry( "msiTarFileCreate", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiTarFileCreate ) );
        table_[ "msiPhyBundleColl" ] = new irods::ms_table_entry( "msiPhyBundleColl", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiPhyBundleColl ) );
        table_[ "msiServerMonPerf" ] = new irods::ms_table_entry( "msiServerMonPerf", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiServerMonPerf ) );
        table_[ "msiFlushMonStat" ] = new irods::ms_table_entry( "msiFlushMonStat", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiFlushMonStat ) );
        table_[ "msiDigestMonStat" ] = new irods::ms_table_entry( "msiDigestMonStat", 7, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDigestMonStat ) );
        table_[ "msiGetContInxFromGenQueryOut" ] = new irods::ms_table_entry( "msiGetContInxFromGenQueryOut", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetContInxFromGenQueryOut ) );
        table_[ "msiSetACL" ] = new irods::ms_table_entry( "msiSetACL", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSetACL ) );
        table_[ "msiSetRescQuotaPolicy" ] = new irods::ms_table_entry( "msiSetRescQuotaPolicy", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetRescQuotaPolicy ) );
        table_[ "msiSetReplComment" ] = new irods::ms_table_entry( "msiSetReplComment", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSetReplComment ) );
        table_[ "msiSetBulkPutPostProcPolicy" ] = new irods::ms_table_entry( "msiSetBulkPutPostProcPolicy", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetBulkPutPostProcPolicy ) );
        table_[ "msiCutBufferInHalf" ] = new irods::ms_table_entry( "msiCutBufferInHalf", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiCutBufferInHalf ) );
        table_[ "msiDoSomething" ] = new irods::ms_table_entry( "msiDoSomething", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDoSomething ) );
        table_[ "msiSysMetaModify" ] = new irods::ms_table_entry( "msiSysMetaModify", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSysMetaModify ) );
        table_[ "msiTakeThreeArgumentsAndDoNothing" ] = new irods::ms_table_entry( "msiTakeThreeArgumentsAndDoNothing", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiTakeThreeArgumentsAndDoNothing ) );
    }; // ms_table::ms_table

    // =-=-=-=-=-=-=-
    // implement the micros table dtor
    template<>
    lookup_table< irods::ms_table_entry* >::~lookup_table() {
        ms_table::iterator itr = begin();
        for ( ; itr != end(); ++itr ) {
            delete itr->second;
        }
    }
} // namespace irods

//irods::ms_table MicrosTable;

#endif // _RE_ACTION_HPP_
