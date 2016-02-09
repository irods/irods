/* reAction.hpp - header file for Actions that are 'called' when executing the rules by
 * the rule engine modules
 */
#ifndef _RE_ACTION_HPP_
#define _RE_ACTION_HPP_

#include "rodsUser.h"
#include "rods.h"
#include "rcGlobalExtern.h"
#include "reDefines.h"
#include "objInfo.h"
#include "regExpMatch.hpp"
#include "reSysDataObjOpr.hpp"
#include "msiHelper.hpp"
#include "reDataObjOpr.hpp"
#include "reNaraMetaData.hpp"
#include "reIn2p3SysRule.hpp"
#include "reFuncDefs.hpp"

typedef struct {
    char action[MAX_ACTION_SIZE];
    int numberOfStringArgs;
    funcPtr callAction;
} microsdef_t;

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
int msiApplyAllRules( msParam_t *actionParam, msParam_t* reiSaveFlagParam,
                      msParam_t* allRuleExecFlagParam, ruleExecInfo_t *rei );
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
//int msiAdmShowIRB(msParam_t *bufP, ruleExecInfo_t *rei);
int msiAdmShowDVM( msParam_t *bufP, ruleExecInfo_t *rei );
int msiAdmShowFNM( msParam_t *bufP, ruleExecInfo_t *rei );
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

int msiXmsgServerConnect( msParam_t* outConnParam, ruleExecInfo_t *rei );
int msiXmsgCreateStream( msParam_t* inConnParam,
                         msParam_t* inGgetXmsgTicketInpParam,
                         msParam_t* outXmsgTicketInfoParam,
                         ruleExecInfo_t *rei );
int msiCreateXmsgInp( msParam_t* inMsgNumber,
                      msParam_t* inMsgType,
                      msParam_t* inNumberOfReceivers,
                      msParam_t* inMsg,
                      msParam_t* inNumberOfDeliverySites,
                      msParam_t* inDeliveryAddressList,
                      msParam_t* inDeliveryPortList,
                      msParam_t* inMiscInfo,
                      msParam_t* inXmsgTicketInfoParam,
                      msParam_t* outSendXmsgInfoParam,
                      ruleExecInfo_t *rei );
int msiSendXmsg( msParam_t* inConnParam,
                 msParam_t* inSendXmsgInpParam,
                 ruleExecInfo_t *rei );
int msiRcvXmsg( msParam_t* inConnParam,
                msParam_t* inTicketNumber,
                msParam_t* inMsgNumber,
                msParam_t* outMsgType,
                msParam_t* outMsg,
                msParam_t* outSendUser,
                ruleExecInfo_t *rei );
int msiXmsgServerDisConnect( msParam_t* inConnParam, ruleExecInfo_t *rei );
int msiString2KeyValPair( msParam_t *inBufferP, msParam_t* outKeyValPairP, ruleExecInfo_t *rei );
int msiStrArray2String( msParam_t* inSAParam, msParam_t* outStr, ruleExecInfo_t *rei );

int msiAW1( msParam_t* mPIn, msParam_t* mPOut2, ruleExecInfo_t *rei );
int msiRenameLocalZone( msParam_t *oldName, msParam_t *newName,
                        ruleExecInfo_t *rei );
int msiRenameCollection( msParam_t *oldName, msParam_t *newName,
                         ruleExecInfo_t *rei );
int msiAclPolicy( msParam_t *msParam, ruleExecInfo_t *rei );
int msiSetQuota( msParam_t *type, msParam_t *name, msParam_t *resource,
                 msParam_t *value, ruleExecInfo_t *rei );
int msiRemoveKeyValuePairsFromObj( msParam_t *metadataParam,
                                   msParam_t* objParam,
                                   msParam_t* typeParam,
                                   ruleExecInfo_t *rei );

int
msiPrintGenQueryInp( msParam_t *where, msParam_t* genQueryInpParam, ruleExecInfo_t *rei );

int
msiGetContInxFromGenQueryOut( msParam_t* genQueryOutParam, msParam_t* continueInx, ruleExecInfo_t *rei );

int msiAdmReadDVMapsFromFileIntoStruct( msParam_t *inDvmFileNameParam, msParam_t *outCoreDVMapStruct, ruleExecInfo_t *rei );
int msiAdmInsertDVMapsFromStructIntoDB( msParam_t *inDvmBaseNameParam, msParam_t *inCoreDVMapStruct, ruleExecInfo_t *rei );
int msiGetDVMapsFromDBIntoStruct( msParam_t *inDvmBaseNameParam, msParam_t *inVersionParam, msParam_t *outCoreDVMapStruct, ruleExecInfo_t *rei );
int msiAdmWriteDVMapsFromStructIntoFile( msParam_t *inDvmFileNameParam, msParam_t *inCoreDVMapStruct, ruleExecInfo_t *rei );

int msiAdmReadFNMapsFromFileIntoStruct( msParam_t *inDvmFileNameParam, msParam_t *outCoreFNMapStruct, ruleExecInfo_t *rei );
int msiAdmInsertFNMapsFromStructIntoDB( msParam_t *inDvmBaseNameParam, msParam_t *inCoreFNMapStruct, ruleExecInfo_t *rei );
int msiGetFNMapsFromDBIntoStruct( msParam_t *inDvmBaseNameParam, msParam_t *inVersionParam, msParam_t *outCoreFNMapStruct, ruleExecInfo_t *rei );
int msiAdmWriteFNMapsFromStructIntoFile( msParam_t *inDvmFileNameParam, msParam_t *inCoreFNMapStruct, ruleExecInfo_t *rei );

int msiAdmReadMSrvcsFromFileIntoStruct( msParam_t *inMsrvcFileNameParam, msParam_t *outCoreMsrvcStruct, ruleExecInfo_t *rei );
int msiAdmInsertMSrvcsFromStructIntoDB( msParam_t *inMsrvcBaseNameParam, msParam_t *inCoreMsrvcStruct, ruleExecInfo_t *rei );
int msiGetMSrvcsFromDBIntoStruct( msParam_t *inStatus, msParam_t *outCoreMsrvcStruct, ruleExecInfo_t *rei );
int msiAdmWriteMSrvcsFromStructIntoFile( msParam_t *inMsrvcFileNameParam, msParam_t *inCoreMsrvcStruct, ruleExecInfo_t *rei );
int writeXMsg( msParam_t* inStreamId, msParam_t *inHdr, msParam_t *inMsg, ruleExecInfo_t *rei );
int readXMsg( msParam_t* inStreamId, msParam_t* inCondRead,
              msParam_t *outMsgNum, msParam_t *outSeqNum,
              msParam_t *outHdr, msParam_t *outMsg,
              msParam_t *outUser, msParam_t *outAddr, ruleExecInfo_t *rei );
int msiSetReplComment( msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,
                       msParam_t *inpParam4, ruleExecInfo_t *rei );
int
msiSetBulkPutPostProcPolicy( msParam_t *xflag, ruleExecInfo_t *rei );
int msiCutBufferInHalf( msParam_t* mPIn, ruleExecInfo_t *rei );
int msiDoSomething( msParam_t *inParam, msParam_t *outParam, ruleExecInfo_t *rei );
int msiString2StrArray( msParam_t *inBufferP, msParam_t* outStrArrayP, ruleExecInfo_t *rei );


#include "irods_ms_plugin.hpp"

namespace irods {

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
        table_[ "msiSetRescSortScheme" ] = new irods::ms_table_entry( "msiSetRescSortScheme", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetRescSortScheme ) );
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
        table_[ "msiSetMultiReplPerResc" ] = new irods::ms_table_entry( "msiSetMultiReplPerResc", 0, std::function<int(ruleExecInfo_t*)>( msiSetMultiReplPerResc ) );
        table_[ "msiAdmShowDVM" ] = new irods::ms_table_entry( "msiAdmShowDVM", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiAdmShowDVM ) );
        table_[ "msiAdmShowFNM" ] = new irods::ms_table_entry( "msiAdmShowFNM", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiAdmShowFNM ) );
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
        table_[ "applyAllRules" ] = new irods::ms_table_entry( "applyAllRules", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiApplyAllRules ) );
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
        table_[ "msiXmsgServerConnect" ] = new irods::ms_table_entry( "msiXmsgServerConnect", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiXmsgServerConnect ) );
        table_[ "msiXmsgCreateStream" ] = new irods::ms_table_entry( "msiXmsgCreateStream", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiXmsgCreateStream ) );
        table_[ "msiCreateXmsgInp" ] = new irods::ms_table_entry( "msiCreateXmsgInp", 10, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiCreateXmsgInp ) );
        table_[ "msiSendXmsg" ] = new irods::ms_table_entry( "msiSendXmsg", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSendXmsg ) );
        table_[ "msiRcvXmsg" ] = new irods::ms_table_entry( "msiRcvXmsg", 6, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiRcvXmsg ) );
        table_[ "msiXmsgServerDisConnect" ] = new irods::ms_table_entry( "msiXmsgServerDisConnect", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiXmsgServerDisConnect ) );
        table_[ "msiString2KeyValPair" ] = new irods::ms_table_entry( "msiString2KeyValPair", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiString2KeyValPair ) );
        table_[ "msiStrArray2String" ] = new irods::ms_table_entry( "msiStrArray2String", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrArray2String ) );
        table_[ "msiString2StrArray" ] = new irods::ms_table_entry( "msiString2StrArray", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiString2StrArray ) );
        table_[ "msiAW1" ] = new irods::ms_table_entry( "msiAW1", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAW1 ) );
        table_[ "msiRenameLocalZone" ] = new irods::ms_table_entry( "msiRenameLocalZone", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiRenameLocalZone ) );
        table_[ "msiRenameCollection" ] = new irods::ms_table_entry( "msiRenameCollection", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiRenameCollection ) );
        table_[ "msiAclPolicy" ] = new irods::ms_table_entry( "msiAclPolicy", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>(msiAclPolicy ) );
        table_[ "msiSetQuota" ] = new irods::ms_table_entry( "msiSetQuota", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>(msiSetQuota ) );
        table_[ "msiRemoveKeyValuePairsFromObj" ] = new irods::ms_table_entry( "msiRemoveKeyValuePairsFromObj", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiRemoveKeyValuePairsFromObj ) );
        table_[ "msiSetReServerNumProc" ] = new irods::ms_table_entry( "msiSetReServerNumProc", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetReServerNumProc ) );
        table_[ "msiGetStdoutInExecCmdOut" ] = new irods::ms_table_entry( "msiGetStdoutInExecCmdOut", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetStdoutInExecCmdOut ) );
        table_[ "msiGetStderrInExecCmdOut" ] = new irods::ms_table_entry( "msiGetStderrInExecCmdOut", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetStderrInExecCmdOut ) );
        table_[ "msiAddKeyValToMspStr" ] = new irods::ms_table_entry( "msiAddKeyValToMspStr", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAddKeyValToMspStr ) );
        table_[ "msiPrintGenQueryInp" ] = new irods::ms_table_entry( "msiPrintGenQueryInp", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiPrintGenQueryInp ) );
        table_[ "msiTarFileExtract" ] = new irods::ms_table_entry( "msiTarFileExtract", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiTarFileExtract ) );
        table_[ "msiTarFileCreate" ] = new irods::ms_table_entry( "msiTarFileCreate", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiTarFileCreate ) );
        table_[ "msiPhyBundleColl" ] = new irods::ms_table_entry( "msiPhyBundleColl", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiPhyBundleColl ) );
        table_[ "msiWriteRodsLog" ] = new irods::ms_table_entry( "msiWriteRodsLog", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiWriteRodsLog ) );
        table_[ "msiServerMonPerf" ] = new irods::ms_table_entry( "msiServerMonPerf", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiServerMonPerf ) );
        table_[ "msiFlushMonStat" ] = new irods::ms_table_entry( "msiFlushMonStat", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiFlushMonStat ) );
        table_[ "msiDigestMonStat" ] = new irods::ms_table_entry( "msiDigestMonStat", 7, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDigestMonStat ) );
        table_[ "msiSplitPath" ] = new irods::ms_table_entry( "msiSplitPath", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSplitPath ) );
        table_[ "msiSplitPathByKey" ] = new irods::ms_table_entry( "msiSplitPathByKey", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSplitPathByKey ) );
        table_[ "msiGetSessionVarValue" ] = new irods::ms_table_entry( "msiGetSessionVarValue", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetSessionVarValue ) );
        table_[ "msiGetContInxFromGenQueryOut" ] = new irods::ms_table_entry( "msiGetContInxFromGenQueryOut", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetContInxFromGenQueryOut ) );
        table_[ "msiSetACL" ] = new irods::ms_table_entry( "msiSetACL", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSetACL ) );
        table_[ "msiSetRescQuotaPolicy" ] = new irods::ms_table_entry( "msiSetRescQuotaPolicy", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetRescQuotaPolicy ) );
        table_[ "msiAdmReadDVMapsFromFileIntoStruct" ] = new irods::ms_table_entry( "msiAdmReadDVMapsFromFileIntoStruct", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmReadDVMapsFromFileIntoStruct ) );
        table_[ "msiAdmInsertDVMapsFromStructIntoDB" ] = new irods::ms_table_entry( "msiAdmInsertDVMapsFromStructIntoDB", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmInsertDVMapsFromStructIntoDB ) );
        table_[ "msiGetDVMapsFromDBIntoStruct" ] = new irods::ms_table_entry( "msiGetDVMapsFromDBIntoStruct", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetDVMapsFromDBIntoStruct ) );
        table_[ "msiAdmWriteDVMapsFromStructIntoFile" ] = new irods::ms_table_entry( "msiAdmWriteDVMapsFromStructIntoFile", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmWriteDVMapsFromStructIntoFile ) );
        table_[ "msiAdmReadFNMapsFromFileIntoStruct" ] = new irods::ms_table_entry( "msiAdmReadFNMapsFromFileIntoStruct", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmReadFNMapsFromFileIntoStruct ) );
        table_[ "msiAdmInsertFNMapsFromStructIntoDB" ] = new irods::ms_table_entry( "msiAdmInsertFNMapsFromStructIntoDB", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmInsertFNMapsFromStructIntoDB ) );
        table_[ "msiGetFNMapsFromDBIntoStruct" ] = new irods::ms_table_entry( "msiGetFNMapsFromDBIntoStruct", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetFNMapsFromDBIntoStruct ) );
        table_[ "msiAdmWriteFNMapsFromStructIntoFile" ] = new irods::ms_table_entry( "msiAdmWriteFNMapsFromStructIntoFile", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmWriteFNMapsFromStructIntoFile ) );
        table_[ "msiAdmReadMSrvcsFromFileIntoStruct" ] = new irods::ms_table_entry( "msiAdmReadMSrvcsFromFileIntoStruct", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmReadMSrvcsFromFileIntoStruct ) );
        table_[ "msiAdmInsertMSrvcsFromStructIntoDB" ] = new irods::ms_table_entry( "msiAdmInsertMSrvcsFromStructIntoDB", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmInsertMSrvcsFromStructIntoDB ) );
        table_[ "msiGetMSrvcsFromDBIntoStruct" ] = new irods::ms_table_entry( "msiGetMSrvcsFromDBIntoStruct", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetMSrvcsFromDBIntoStruct ) );
        table_[ "msiAdmWriteMSrvcsFromStructIntoFile" ] = new irods::ms_table_entry( "msiAdmWriteMSrvcsFromStructIntoFile", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAdmWriteMSrvcsFromStructIntoFile ) );
        table_[ "writeXMsg" ] = new irods::ms_table_entry( "writeXMsg", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( writeXMsg ) );
        table_[ "readXMsg" ] = new irods::ms_table_entry( "readXMsg", 8, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( readXMsg ) );
        table_[ "msiSetReplComment" ] = new irods::ms_table_entry( "msiSetReplComment", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSetReplComment ) );
        table_[ "msiSetBulkPutPostProcPolicy" ] = new irods::ms_table_entry( "msiSetBulkPutPostProcPolicy", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiSetBulkPutPostProcPolicy ) );
        table_[ "msiStrlen" ] = new irods::ms_table_entry( "msiStrlen", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrlen ) );
        table_[ "msiStrCat" ] = new irods::ms_table_entry( "msiStrCat", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrCat ) );
        table_[ "msiStrchop" ] = new irods::ms_table_entry( "msiStrchop", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrchop ) );
        table_[ "msiSubstr" ] = new irods::ms_table_entry( "msiSubstr", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSubstr ) );
        table_[ "msiCutBufferInHalf" ] = new irods::ms_table_entry( "msiCutBufferInHalf", 1, std::function<int(msParam_t*,ruleExecInfo_t*)>( msiCutBufferInHalf ) );
        table_[ "msiDoSomething" ] = new irods::ms_table_entry( "msiDoSomething", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiDoSomething ) );
        table_[ "msiExit" ] = new irods::ms_table_entry( "msiExit", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiExit ) );
        table_[ "msiSysMetaModify" ] = new irods::ms_table_entry( "msiSysMetaModify", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSysMetaModify ) );

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

}; // namespace irods::

//irods::ms_table MicrosTable;

#endif	/* _RE_ACTION_HPP_ */
