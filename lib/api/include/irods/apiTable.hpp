#ifndef IRODS_API_TABLE_HPP
#define IRODS_API_TABLE_HPP

#include "irods/apiHandler.hpp"
#include "irods/apiHeaderAll.h"
#include "irods/apiNumber.h"
#include "irods/client_hints.h"
#include "irods/dataObjInpOut.h"
#include "irods/getRescQuota.h"
#include "irods/ies_client_hints.h"
#include "irods/irods_api_calling_functions.hpp"
#include "irods/msParam.h"
#include "irods/rcMisc.h"
#include "irods/rods.h"
#include "irods/rodsUser.h"
#include "irods/server_report.h"
#include "irods/zone_report.h"

#if defined(CREATE_API_TABLE_FOR_SERVER) && !defined(CREATE_API_TABLE_FOR_CLIENT)
#include "irods/rsAuthCheck.hpp"
#include "irods/rsAuthPluginRequest.hpp"
#include "irods/rsAuthRequest.hpp"
#include "irods/rsAuthResponse.hpp"
#include "irods/rsAuthenticate.hpp"
#include "irods/rsBulkDataObjPut.hpp"
#include "irods/rsBulkDataObjReg.hpp"
#include "irods/rsChkNVPathPerm.hpp"
#include "irods/rsChkObjPermAndStat.hpp"
#include "irods/rsClientHints.hpp"
#include "irods/rsCloseCollection.hpp"
#include "irods/rsCollCreate.hpp"
#include "irods/rsCollRepl.hpp"
#include "irods/rsDataCopy.hpp"
#include "irods/rsDataGet.hpp"
#include "irods/rsDataObjChksum.hpp"
#include "irods/rsDataObjClose.hpp"
#include "irods/rsDataObjCopy.hpp"
#include "irods/rsDataObjCreate.hpp"
#include "irods/rsDataObjCreateAndStat.hpp"
#include "irods/rsDataObjGet.hpp"
#include "irods/rsDataObjLock.hpp"
#include "irods/rsDataObjLseek.hpp"
#include "irods/rsDataObjOpen.hpp"
#include "irods/rsDataObjOpenAndStat.hpp"
#include "irods/rsDataObjPhymv.hpp"
#include "irods/rsDataObjPut.hpp"
#include "irods/rsDataObjRead.hpp"
#include "irods/rsDataObjRename.hpp"
#include "irods/rsDataObjRepl.hpp"
#include "irods/rsDataObjRsync.hpp"
#include "irods/rsDataObjTrim.hpp"
#include "irods/rsDataObjTruncate.hpp"
#include "irods/rsDataObjUnlink.hpp"
#include "irods/rsDataObjWrite.hpp"
#include "irods/rsDataPut.hpp"
#include "irods/rsEndTransaction.hpp"
#include "irods/rsExecCmd.hpp"
#include "irods/rsExecMyRule.hpp"
#include "irods/rsExecRuleExpression.hpp"
#include "irods/rsFileChksum.hpp"
#include "irods/rsFileChmod.hpp"
#include "irods/rsFileClose.hpp"
#include "irods/rsFileClosedir.hpp"
#include "irods/rsFileCreate.hpp"
#include "irods/rsFileGet.hpp"
#include "irods/rsFileGetFsFreeSpace.hpp"
#include "irods/rsFileLseek.hpp"
#include "irods/rsFileMkdir.hpp"
#include "irods/rsFileOpen.hpp"
#include "irods/rsFileOpendir.hpp"
#include "irods/rsFilePut.hpp"
#include "irods/rsFileRead.hpp"
#include "irods/rsFileReaddir.hpp"
#include "irods/rsFileRename.hpp"
#include "irods/rsFileRmdir.hpp"
#include "irods/rsFileStageToCache.hpp"
#include "irods/rsFileStat.hpp"
#include "irods/rsFileSyncToArch.hpp"
#include "irods/rsFileTruncate.hpp"
#include "irods/rsFileUnlink.hpp"
#include "irods/rsFileWrite.hpp"
#include "irods/rsGenQuery.hpp"
#include "irods/rsGeneralAdmin.hpp"
#include "irods/rsGeneralRowInsert.hpp"
#include "irods/rsGeneralRowPurge.hpp"
#include "irods/rsGeneralUpdate.hpp"
#include "irods/rsGetHierFromLeafId.hpp"
#include "irods/rsGetHierarchyForResc.hpp"
#include "irods/rsGetHostForGet.hpp"
#include "irods/rsGetHostForPut.hpp"
#include "irods/rsGetLimitedPassword.hpp"
#include "irods/rsGetMiscSvrInfo.hpp"
#include "irods/rsGetRemoteZoneResc.hpp"
#include "irods/rsGetRescQuota.hpp"
#include "irods/rsGetTempPassword.hpp"
#include "irods/rsGetTempPasswordForOther.hpp"
#include "irods/rsIESClientHints.hpp"
#include "irods/rsL3FileGetSingleBuf.hpp"
#include "irods/rsL3FilePutSingleBuf.hpp"
#include "irods/rsModAVUMetadata.hpp"
#include "irods/rsModAccessControl.hpp"
#include "irods/rsModColl.hpp"
#include "irods/rsModDataObjMeta.hpp"
#include "irods/rsObjStat.hpp"
#include "irods/rsOpenCollection.hpp"
#include "irods/rsOprComplete.hpp"
#include "irods/rsPamAuthRequest.hpp"
#include "irods/rsPhyPathReg.hpp"
#include "irods/rsProcStat.hpp"
#include "irods/rsQuerySpecColl.hpp"
#include "irods/rsReadCollection.hpp"
#include "irods/rsRegColl.hpp"
#include "irods/rsRegDataObj.hpp"
#include "irods/rsRegReplica.hpp"
#include "irods/rsRmColl.hpp"
#include "irods/rsRuleExecDel.hpp"
#include "irods/rsRuleExecMod.hpp"
#include "irods/rsRuleExecSubmit.hpp"
#include "irods/rsServerReport.hpp"
#include "irods/rsSimpleQuery.hpp"
#include "irods/rsSpecificQuery.hpp"
#include "irods/rsSslEnd.hpp"
#include "irods/rsSslStart.hpp"
#include "irods/rsStreamClose.hpp"
#include "irods/rsStreamRead.hpp"
#include "irods/rsStructFileBundle.hpp"
#include "irods/rsStructFileExtAndReg.hpp"
#include "irods/rsStructFileExtract.hpp"
#include "irods/rsStructFileSync.hpp"
#include "irods/rsSubStructFileClose.hpp"
#include "irods/rsSubStructFileClosedir.hpp"
#include "irods/rsSubStructFileCreate.hpp"
#include "irods/rsSubStructFileGet.hpp"
#include "irods/rsSubStructFileLseek.hpp"
#include "irods/rsSubStructFileMkdir.hpp"
#include "irods/rsSubStructFileOpen.hpp"
#include "irods/rsSubStructFileOpendir.hpp"
#include "irods/rsSubStructFilePut.hpp"
#include "irods/rsSubStructFileRead.hpp"
#include "irods/rsSubStructFileReaddir.hpp"
#include "irods/rsSubStructFileRename.hpp"
#include "irods/rsSubStructFileRmdir.hpp"
#include "irods/rsSubStructFileStat.hpp"
#include "irods/rsSubStructFileTruncate.hpp"
#include "irods/rsSubStructFileUnlink.hpp"
#include "irods/rsSubStructFileWrite.hpp"
#include "irods/rsSyncMountedColl.hpp"
#include "irods/rsTicketAdmin.hpp"
#include "irods/rsUnbunAndRegPhyBunfile.hpp"
#include "irods/rsUnregDataObj.hpp"
#include "irods/rsUserAdmin.hpp"
#include "irods/rsZoneReport.hpp"
#define NULLPTR_FOR_CLIENT_TABLE(x) x
#elif !defined(CREATE_API_TABLE_FOR_SERVER) && defined(CREATE_API_TABLE_FOR_CLIENT)
#define NULLPTR_FOR_CLIENT_TABLE(x) nullptr
#else
#error "exactly one of {CREATE_API_TABLE_FOR_SERVER, CREATE_API_TABLE_FOR_CLIENT} must be defined"
#endif

#include <boost/any.hpp>

#include <cstdlib>
#include <functional>

#define RS_AUTHENTICATE NULLPTR_FOR_CLIENT_TABLE(rsAuthenticate)
#define RS_AUTH_CHECK NULLPTR_FOR_CLIENT_TABLE(rsAuthCheck)
#define RS_AUTH_PLUG_REQ NULLPTR_FOR_CLIENT_TABLE(rsAuthPluginRequest)
#define RS_AUTH_REQUEST NULLPTR_FOR_CLIENT_TABLE(rsAuthRequest)
#define RS_AUTH_RESPONSE NULLPTR_FOR_CLIENT_TABLE(rsAuthResponse)
#define RS_BULK_DATA_OBJ_PUT NULLPTR_FOR_CLIENT_TABLE(rsBulkDataObjPut)
#define RS_BULK_DATA_OBJ_REG NULLPTR_FOR_CLIENT_TABLE(rsBulkDataObjReg)
#define RS_CHK_NV_PATH_PERM NULLPTR_FOR_CLIENT_TABLE(rsChkNVPathPerm)
#define RS_CHK_OBJ_PERM_AND_STAT NULLPTR_FOR_CLIENT_TABLE(rsChkObjPermAndStat)
#define RS_CLIENT_HINTS NULLPTR_FOR_CLIENT_TABLE(rsClientHints)
#define RS_CLOSE_COLLECTION NULLPTR_FOR_CLIENT_TABLE(rsCloseCollection)
#define RS_COLL_CREATE NULLPTR_FOR_CLIENT_TABLE(rsCollCreate)
#define RS_COLL_REPL NULLPTR_FOR_CLIENT_TABLE(rsCollRepl)
#define RS_DATA_COPY NULLPTR_FOR_CLIENT_TABLE(rsDataCopy)
#define RS_DATA_GET NULLPTR_FOR_CLIENT_TABLE(rsDataGet)
#define RS_DATA_OBJ_CHKSUM NULLPTR_FOR_CLIENT_TABLE(rsDataObjChksum)
#define RS_DATA_OBJ_CLOSE NULLPTR_FOR_CLIENT_TABLE(rsDataObjClose)
#define RS_DATA_OBJ_COPY NULLPTR_FOR_CLIENT_TABLE(rsDataObjCopy)
#define RS_DATA_OBJ_CREATE NULLPTR_FOR_CLIENT_TABLE(rsDataObjCreate)
#define RS_DATA_OBJ_CREATE_AND_STAT NULLPTR_FOR_CLIENT_TABLE(rsDataObjCreateAndStat)
#define RS_DATA_OBJ_GET NULLPTR_FOR_CLIENT_TABLE(rsDataObjGet)
#define RS_DATA_OBJ_LOCK NULLPTR_FOR_CLIENT_TABLE(rsDataObjLock)
#define RS_DATA_OBJ_LSEEK NULLPTR_FOR_CLIENT_TABLE(rsDataObjLseek)
#define RS_DATA_OBJ_OPEN NULLPTR_FOR_CLIENT_TABLE(rsDataObjOpen)
#define RS_DATA_OBJ_OPEN_AND_STAT NULLPTR_FOR_CLIENT_TABLE(rsDataObjOpenAndStat)
#define RS_DATA_OBJ_PHYMV NULLPTR_FOR_CLIENT_TABLE(rsDataObjPhymv)
#define RS_DATA_OBJ_PUT NULLPTR_FOR_CLIENT_TABLE(rsDataObjPut)
#define RS_DATA_OBJ_READ NULLPTR_FOR_CLIENT_TABLE(rsDataObjRead)
#define RS_DATA_OBJ_RENAME NULLPTR_FOR_CLIENT_TABLE(rsDataObjRename)
#define RS_DATA_OBJ_REPL NULLPTR_FOR_CLIENT_TABLE(rsDataObjRepl)
#define RS_DATA_OBJ_RSYNC NULLPTR_FOR_CLIENT_TABLE(rsDataObjRsync)
#define RS_DATA_OBJ_TRIM NULLPTR_FOR_CLIENT_TABLE(rsDataObjTrim)
#define RS_DATA_OBJ_TRUNCATE NULLPTR_FOR_CLIENT_TABLE(rsDataObjTruncate)
#define RS_DATA_OBJ_UNLINK NULLPTR_FOR_CLIENT_TABLE(rsDataObjUnlink)
#define RS_DATA_OBJ_UNLOCK NULLPTR_FOR_CLIENT_TABLE(rsDataObjUnlock)
#define RS_DATA_OBJ_WRITE NULLPTR_FOR_CLIENT_TABLE(rsDataObjWrite)
#define RS_DATA_PUT NULLPTR_FOR_CLIENT_TABLE(rsDataPut)
#define RS_END_TRANSACTION NULLPTR_FOR_CLIENT_TABLE(rsEndTransaction)
#define RS_EXEC_CMD NULLPTR_FOR_CLIENT_TABLE(rsExecCmd)
#define RS_EXEC_MY_RULE NULLPTR_FOR_CLIENT_TABLE(rsExecMyRule)
#define RS_EXEC_RULE_EXPRESSION NULLPTR_FOR_CLIENT_TABLE(rsExecRuleExpression)
#define RS_FILE_CHKSUM NULLPTR_FOR_CLIENT_TABLE(rsFileChksum)
#define RS_FILE_CHMOD NULLPTR_FOR_CLIENT_TABLE(rsFileChmod)
#define RS_FILE_CLOSE NULLPTR_FOR_CLIENT_TABLE(rsFileClose)
#define RS_FILE_CLOSEDIR NULLPTR_FOR_CLIENT_TABLE(rsFileClosedir)
#define RS_FILE_CREATE NULLPTR_FOR_CLIENT_TABLE(rsFileCreate)
#define RS_FILE_GET NULLPTR_FOR_CLIENT_TABLE(rsFileGet)
#define RS_FILE_GET_FS_FREE_SPACE NULLPTR_FOR_CLIENT_TABLE(rsFileGetFsFreeSpace)
#define RS_FILE_LSEEK NULLPTR_FOR_CLIENT_TABLE(rsFileLseek)
#define RS_FILE_MKDIR NULLPTR_FOR_CLIENT_TABLE(rsFileMkdir)
#define RS_FILE_OPEN NULLPTR_FOR_CLIENT_TABLE(rsFileOpen)
#define RS_FILE_OPENDIR NULLPTR_FOR_CLIENT_TABLE(rsFileOpendir)
#define RS_FILE_PUT NULLPTR_FOR_CLIENT_TABLE(rsFilePut)
#define RS_FILE_READ NULLPTR_FOR_CLIENT_TABLE(rsFileRead)
#define RS_FILE_READDIR NULLPTR_FOR_CLIENT_TABLE(rsFileReaddir)
#define RS_FILE_RENAME NULLPTR_FOR_CLIENT_TABLE(rsFileRename)
#define RS_FILE_RMDIR NULLPTR_FOR_CLIENT_TABLE(rsFileRmdir)
#define RS_FILE_STAGE_TO_CACHE NULLPTR_FOR_CLIENT_TABLE(rsFileStageToCache)
#define RS_FILE_STAT NULLPTR_FOR_CLIENT_TABLE(rsFileStat)
#define RS_FILE_SYNC_TO_ARCH NULLPTR_FOR_CLIENT_TABLE(rsFileSyncToArch)
#define RS_FILE_TRUNCATE NULLPTR_FOR_CLIENT_TABLE(rsFileTruncate)
#define RS_FILE_UNLINK NULLPTR_FOR_CLIENT_TABLE(rsFileUnlink)
#define RS_FILE_WRITE NULLPTR_FOR_CLIENT_TABLE(rsFileWrite)
#define RS_GENERAL_ADMIN NULLPTR_FOR_CLIENT_TABLE(rsGeneralAdmin)
#define RS_GENERAL_ROW_INSERT NULLPTR_FOR_CLIENT_TABLE(rsGeneralRowInsert)
#define RS_GENERAL_ROW_PURGE NULLPTR_FOR_CLIENT_TABLE(rsGeneralRowPurge)
#define RS_GENERAL_UPDATE NULLPTR_FOR_CLIENT_TABLE(rsGeneralUpdate)
#define RS_GEN_QUERY NULLPTR_FOR_CLIENT_TABLE(rsGenQuery)
#define RS_GET_HIER_FOR_RESC NULLPTR_FOR_CLIENT_TABLE(rsGetHierarchyForResc)
#define RS_GET_HIER_FROM_LEAF_ID NULLPTR_FOR_CLIENT_TABLE(rsGetHierFromLeafId)
#define RS_GET_HOST_FOR_GET NULLPTR_FOR_CLIENT_TABLE(rsGetHostForGet)
#define RS_GET_HOST_FOR_PUT NULLPTR_FOR_CLIENT_TABLE(rsGetHostForPut)
#define RS_GET_LIMITED_PASSWORD NULLPTR_FOR_CLIENT_TABLE(rsGetLimitedPassword)
#define RS_GET_MISC_SVR_INFO NULLPTR_FOR_CLIENT_TABLE(rsGetMiscSvrInfo)
#define RS_GET_REMOTE_ZONE_RESC NULLPTR_FOR_CLIENT_TABLE(rsGetRemoteZoneResc)
#define RS_GET_RESC_QUOTA NULLPTR_FOR_CLIENT_TABLE(rsGetRescQuota)
#define RS_GET_TEMP_PASSWORD NULLPTR_FOR_CLIENT_TABLE(rsGetTempPassword)
#define RS_GET_TEMP_PASSWORD_FOR_OTHER NULLPTR_FOR_CLIENT_TABLE(rsGetTempPasswordForOther)
#define RS_IES_CLIENT_HINTS NULLPTR_FOR_CLIENT_TABLE(rsClientHints)
#define RS_L3_FILE_GET_SINGLE_BUF NULLPTR_FOR_CLIENT_TABLE(rsL3FileGetSingleBuf)
#define RS_L3_FILE_PUT_SINGLE_BUF NULLPTR_FOR_CLIENT_TABLE(rsL3FilePutSingleBuf)
#define RS_MOD_ACCESS_CONTROL NULLPTR_FOR_CLIENT_TABLE(rsModAccessControl)
#define RS_MOD_AVU_METADATA NULLPTR_FOR_CLIENT_TABLE(rsModAVUMetadata)
#define RS_MOD_COLL NULLPTR_FOR_CLIENT_TABLE(rsModColl)
#define RS_MOD_DATA_OBJ_META NULLPTR_FOR_CLIENT_TABLE(rsModDataObjMeta)
#define RS_OBJ_STAT NULLPTR_FOR_CLIENT_TABLE(rsObjStat)
#define RS_OPEN_COLLECTION NULLPTR_FOR_CLIENT_TABLE(rsOpenCollection)
#define RS_OPR_COMPLETE NULLPTR_FOR_CLIENT_TABLE(rsOprComplete)
#define RS_PAM_AUTH_REQUEST NULLPTR_FOR_CLIENT_TABLE(rsPamAuthRequest)
#define RS_PHY_PATH_REG NULLPTR_FOR_CLIENT_TABLE(rsPhyPathReg)
#define RS_PROC_STAT NULLPTR_FOR_CLIENT_TABLE(rsProcStat)
#define RS_QUERY_SPEC_COLL NULLPTR_FOR_CLIENT_TABLE(rsQuerySpecColl)
#define RS_READ_COLLECTION NULLPTR_FOR_CLIENT_TABLE(rsReadCollection)
#define RS_REG_COLL NULLPTR_FOR_CLIENT_TABLE(rsRegColl)
#define RS_REG_DATA_OBJ NULLPTR_FOR_CLIENT_TABLE(rsRegDataObj)
#define RS_REG_REPLICA NULLPTR_FOR_CLIENT_TABLE(rsRegReplica)
#define RS_RM_COLL NULLPTR_FOR_CLIENT_TABLE(rsRmColl)
#define RS_RULE_EXEC_DEL NULLPTR_FOR_CLIENT_TABLE(rsRuleExecDel)
#define RS_RULE_EXEC_MOD NULLPTR_FOR_CLIENT_TABLE(rsRuleExecMod)
#define RS_RULE_EXEC_SUBMIT NULLPTR_FOR_CLIENT_TABLE(rsRuleExecSubmit)
#define RS_SERVER_REPORT NULLPTR_FOR_CLIENT_TABLE(rsServerReport)
#define RS_SIMPLE_QUERY NULLPTR_FOR_CLIENT_TABLE(rsSimpleQuery)
#define RS_SPECIFIC_QUERY NULLPTR_FOR_CLIENT_TABLE(rsSpecificQuery)
#define RS_SSL_END NULLPTR_FOR_CLIENT_TABLE(rsSslEnd)
#define RS_SSL_START NULLPTR_FOR_CLIENT_TABLE(rsSslStart)
#define RS_STREAM_CLOSE NULLPTR_FOR_CLIENT_TABLE(rsStreamClose)
#define RS_STREAM_READ NULLPTR_FOR_CLIENT_TABLE(rsStreamRead)
#define RS_STRUCT_FILE_BUNDLE NULLPTR_FOR_CLIENT_TABLE(rsStructFileBundle)
#define RS_STRUCT_FILE_EXTRACT NULLPTR_FOR_CLIENT_TABLE(rsStructFileExtract)
#define RS_STRUCT_FILE_EXT_AND_REG NULLPTR_FOR_CLIENT_TABLE(rsStructFileExtAndReg)
#define RS_STRUCT_FILE_SYNC NULLPTR_FOR_CLIENT_TABLE(rsStructFileSync)
#define RS_SUB_STRUCT_FILE_CLOSE NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileClose)
#define RS_SUB_STRUCT_FILE_CLOSEDIR NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileClosedir)
#define RS_SUB_STRUCT_FILE_CREATE NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileCreate)
#define RS_SUB_STRUCT_FILE_GET NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileGet)
#define RS_SUB_STRUCT_FILE_LSEEK NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileLseek)
#define RS_SUB_STRUCT_FILE_MKDIR NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileMkdir)
#define RS_SUB_STRUCT_FILE_OPEN NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileOpen)
#define RS_SUB_STRUCT_FILE_OPENDIR NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileOpendir)
#define RS_SUB_STRUCT_FILE_PUT NULLPTR_FOR_CLIENT_TABLE(rsSubStructFilePut)
#define RS_SUB_STRUCT_FILE_READ NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileRead)
#define RS_SUB_STRUCT_FILE_READDIR NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileReaddir)
#define RS_SUB_STRUCT_FILE_RENAME NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileRename)
#define RS_SUB_STRUCT_FILE_RMDIR NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileRmdir)
#define RS_SUB_STRUCT_FILE_STAT NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileStat)
#define RS_SUB_STRUCT_FILE_TRUNCATE NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileTruncate)
#define RS_SUB_STRUCT_FILE_UNLINK NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileUnlink)
#define RS_SUB_STRUCT_FILE_WRITE NULLPTR_FOR_CLIENT_TABLE(rsSubStructFileWrite)
#define RS_SYNC_MOUNTED_COLL NULLPTR_FOR_CLIENT_TABLE(rsSyncMountedColl)
#define RS_TICKET_ADMIN NULLPTR_FOR_CLIENT_TABLE(rsTicketAdmin)
#define RS_UNBUN_AND_REG_PHY_BUNFILE NULLPTR_FOR_CLIENT_TABLE(rsUnbunAndRegPhyBunfile)
#define RS_UNREG_DATA_OBJ NULLPTR_FOR_CLIENT_TABLE(rsUnregDataObj)
#define RS_USER_ADMIN NULLPTR_FOR_CLIENT_TABLE(rsUserAdmin)
#define RS_ZONE_REPORT NULLPTR_FOR_CLIENT_TABLE(rsZoneReport)

#if defined(CREATE_API_TABLE_FOR_SERVER) && !defined(CREATE_API_TABLE_FOR_CLIENT)
static irods::apidef_t server_api_table_inp[] = {
#elif !defined(CREATE_API_TABLE_FOR_SERVER) && defined(CREATE_API_TABLE_FOR_CLIENT)
static irods::apidef_t client_api_table_inp[] = {
#else
#  error "exactly one of {CREATE_API_TABLE_FOR_SERVER, CREATE_API_TABLE_FOR_CLIENT} must be defined"
#endif
    // clang-format off
    {
        GET_MISC_SVR_INFO_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        NULL, 0, "MiscSvrInfo_PI", 0,
        boost::any(std::function<int(rsComm_t*,miscSvrInfo_t**)>(RS_GET_MISC_SVR_INFO)),
        "api_get_misc_svr_info", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_MISCSVRINFOOUT
    },
    {
        FILE_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, "fileCreateOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileCreateInp_t*,fileCreateOut_t**)>(RS_FILE_CREATE)),
        "api_file_create", clearFileOpenInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILECREATEINP_FILECREATEOUT
    },
    {
        FILE_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileOpenInp_t*)>(RS_FILE_OPEN)),
        "api_file_open", clearFileOpenInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEOPENINP
    },
    {
        CHK_N_V_PATH_PERM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileOpenInp_t*)>(RS_CHK_NV_PATH_PERM)),
        "api_check_nv_path_perm", clearFileOpenInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEOPENINP
    },
    {
        FILE_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileWriteInp_PI", 1, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileWriteInp_t*,bytesBuf_t*)>(RS_FILE_WRITE)),
        "api_file_write", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEWRITEINP_BYTESBUFINP
    },
    {
        FILE_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileCloseInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileCloseInp_t*)>(RS_FILE_CLOSE)),
        "api_file_close", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILECLOSEINP
    },
    {
        FILE_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileLseekInp_PI", 0, "fileLseekOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileLseekInp_t*,fileLseekOut_t**)>(RS_FILE_LSEEK)),
        "api_file_lseek", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILELSEEKINP_FILELSEEKOUT
    },
    {
        FILE_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileReadInp_PI", 0, NULL, 1,
        boost::any(std::function<int(rsComm_t*,fileReadInp_t*,bytesBuf_t*)>(RS_FILE_READ)),
        "api_file_read", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEREADINP_BYTESBUFOUT
    },
    {
        FILE_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileUnlinkInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileUnlinkInp_t*)>(RS_FILE_UNLINK)),
        "api_file_unlink", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEUNLINKINP
    },
    {
        FILE_MKDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileMkdirInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileMkdirInp_t*)>(RS_FILE_MKDIR)),
        "api_file_mkdir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEMKDIRINP
    },
    {
        FILE_CHMOD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileChmodInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileChmodInp_t*)>(RS_FILE_CHMOD)),
        "api_file_chmod", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILECHMODINP
    },
    {
        FILE_RMDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileRmdirInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileRmdirInp_t*)>(RS_FILE_RMDIR)),
        "api_file_rmdir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILERMDIRINP
    },
    {
        FILE_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStatInp_PI", 0,  "RODS_STAT_T_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileStatInp_t*,rodsStat_t**)>(RS_FILE_STAT)),
        "api_file_stat", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILESTATINP_RODSSTATOUT
    },
    {
        FILE_GET_FS_FREE_SPACE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileGetFsFreeSpaceInp_PI", 0, "fileGetFsFreeSpaceOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileGetFsFreeSpaceInp_t*,fileGetFsFreeSpaceOut_t**)>(RS_FILE_GET_FS_FREE_SPACE)),
        "api_file_get_fs_free_space", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEGETFREESPACEINP_FILEGETFREESPACEOUT
    },
    {
        FILE_OPENDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpendirInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileOpendirInp_t*)>(RS_FILE_OPENDIR)),
        "api_file_opendir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEOPENDIRINP
    },
    {
        FILE_CLOSEDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileClosedirInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileClosedirInp_t*)>(RS_FILE_CLOSEDIR)),
        "api_file_closedir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILECLOSEDIRINP
    },
    {
        FILE_READDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileReaddirInp_PI", 0, "RODS_DIRENT_T_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileReaddirInp_t*,rodsDirent_t**)>(RS_FILE_READDIR)),
        "api_file_readdir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEREADDIRINP_RODSDIRENTOUT
    },
    {
        FILE_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileRenameInp_PI", 0, "fileRenameOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileRenameInp_t*,fileRenameOut_t**)>(RS_FILE_RENAME)),
        "api_file_rename", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILERENAMEINP_FILERENAMEOUT
    },
    {
        FILE_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileOpenInp_t*)>(RS_FILE_TRUNCATE)),
        "api_file_truncate", clearFileOpenInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEOPENINP
    },
    {
        FILE_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 1, "filePutOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileOpenInp_t*,bytesBuf_t*,filePutOut_t**)>(RS_FILE_PUT)),
        "api_file_put", clearFileOpenInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEOPENINP_BYTESBUFINP_FILEPUTOUT
    },
    {
        FILE_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 1,
        boost::any(std::function<int(rsComm_t*,fileOpenInp_t*,bytesBuf_t*)>(RS_FILE_GET)),
        "api_file_get", clearFileOpenInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEOPENINP_BYTESBUFOUT
    },
    {
        FILE_STAGE_TO_CACHE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStageSyncInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileStageSyncInp_t*)>(RS_FILE_STAGE_TO_CACHE)),
        "api_file_stage_to_cache", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILESTAGESYNCINP
    },
    {
        FILE_SYNC_TO_ARCH_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStageSyncInp_PI", 0, "fileSyncOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileStageSyncInp_t*,fileSyncOut_t**)>(RS_FILE_SYNC_TO_ARCH)),
        "api_file_sync_to_arch", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILESTAGESYNCINP_FILESYNCOUT
    },
    {
        DATA_OBJ_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_DATA_OBJ_CREATE)),
        "api_data_obj_create", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        DATA_OBJ_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_DATA_OBJ_OPEN)),
        "api_data_obj_open", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        DATA_OBJ_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, NULL, 1,
        boost::any(std::function<int(rsComm_t*,openedDataObjInp_t*,bytesBuf_t*)>(RS_DATA_OBJ_READ)),
        "api_data_obj_read", clearOpenedDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_OPENEDDATAOBJINP_BYTESBUFOUT
    },
    {
        DATA_OBJ_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 1, NULL, 0,
        boost::any(std::function<int(rsComm_t*,openedDataObjInp_t*,bytesBuf_t*)>(RS_DATA_OBJ_WRITE)),
        "api_data_obj_write", clearOpenedDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_OPENEDDATAOBJINP_BYTESBUFOUT
    },
    {
        DATA_OBJ_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,openedDataObjInp_t*)>(RS_DATA_OBJ_CLOSE)),
        "api_data_obj_close", clearOpenedDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_OPENEDDATAOBJINP
    },
    {
        DATA_OBJ_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 1, "PortalOprOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,bytesBuf_t*,portalOprOut_t**)>(RS_DATA_OBJ_PUT)),
        "api_data_obj_put", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_BYTESBUFINP_PORTALOPROUT
    },
    {
        DATA_OBJ_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "PortalOprOut_PI", 1,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,portalOprOut_t**,bytesBuf_t*)>(RS_DATA_OBJ_GET)),
        "api_data_obj_get", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_PORTALOPROUT_BYTESBUFOUT
    },
    {
        DATA_OBJ_REPL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "TransferStat_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,transferStat_t**)>(RS_DATA_OBJ_REPL)),
        "api_data_obj_repl", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_TRANSFERSTATOUT
    },
    {
        DATA_OBJ_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, "fileLseekOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,openedDataObjInp_t*,fileLseekOut_t**)>(RS_DATA_OBJ_LSEEK)),
        "api_data_obj_lseek", clearOpenedDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_OPENEDDATAOBJINP_FILELSEEKOUT
    },
    {
        DATA_OBJ_COPY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjCopyInp_PI", 0, "TransferStat_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjCopyInp_t*,transferStat_t**)>(RS_DATA_OBJ_COPY)),
        "api_data_obj_copy", clearDataObjCopyInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJCOPYINP_TRANFERSTATOUT
    },
    {
        DATA_OBJ_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_DATA_OBJ_UNLINK)),
        "api_data_obj_unlink", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        DATA_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataOprInp_PI", 0, "PortalOprOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataOprInp_t*,portalOprOut_t**)>(RS_DATA_PUT)),
        "api_data_put", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOPRINP_PORTALOPROUT
    },
    {
        DATA_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataOprInp_PI", 0, "PortalOprOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataOprInp_t*,portalOprOut_t**)>(RS_DATA_GET)),
        "api_data_get", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOPRINP_PORTALOPROUT
    },
    {
        DATA_COPY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataCopyInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataCopyInp_t*)>(RS_DATA_COPY)),
        "api_data_copy", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATACOPYINP
    },
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    {
        SIMPLE_QUERY_AN, RODS_API_VERSION, LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "simpleQueryInp_PI", 0,  "simpleQueryOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,simpleQueryInp_t*,simpleQueryOut_t**)>(RS_SIMPLE_QUERY)),
        "api_simple_query", irods::clearInStruct_noop, clearSimpleQueryOut,
        (funcPtr)CALL_SIMPLEQUERYINP_SIMPLEQUERYOUT
    },
#pragma clang diagnostic pop
    {
        GENERAL_ADMIN_AN, RODS_API_VERSION, LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalAdminInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,generalAdminInp_t*)>(RS_GENERAL_ADMIN)),
        "api_general_admin", clearGeneralAdminInput, irods::clearOutStruct_noop,
        (funcPtr)CALL_GENERALADMININP
    },
    {
        GEN_QUERY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "GenQueryInp_PI", 0, "GenQueryOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,genQueryInp_t*,genQueryOut_t**)>(RS_GEN_QUERY)),
        "api_gen_query", clearGenQueryInp, clearGenQueryOut,
        (funcPtr)CALL_GENQUERYINP_GENQUERYOUT
    },
    {
        AUTH_REQUEST_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        NULL, 0,  "authRequestOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,authRequestOut_t**)>(RS_AUTH_REQUEST)),
        "api_auth_request", irods::clearInStruct_noop, clearAuthRequestOut,
        (funcPtr)CALL_AUTHREQUESTOUT
    },
    {
        AUTH_RESPONSE_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "authResponseInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,authResponseInp_t*)>(RS_AUTH_RESPONSE)),
        "api_auth_response", clearAuthResponseInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_AUTHRESPONSEINP
    },
    /* AUTH_CHECK might need to be NO_USER_AUTH but it would be better
    if we could restrict it some; if the servers can authenticate
    themselves first (if they need to eventually, probably, anyway)
    then this should work OK.  If not, we can change REMOTE_USER_AUTH
    to NO_USER_AUTH.
    Need to set it to NO_USER_AUTH for cross zone auth to prevent ping-pong
    effect */
    {
        AUTH_CHECK_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "authCheckInp_PI", 0,  "authCheckOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,authCheckInp_t*,authCheckOut_t**)>(RS_AUTH_CHECK)),
        "api_auth_check", irods::clearInStruct_noop, clearAuthCheckOut,
        (funcPtr)CALL_AUTHCHECKINP_AUTHCHECKOUT
    },
    {
        END_TRANSACTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "endTransactionInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,endTransactionInp_t*)>(RS_END_TRANSACTION)),
        "api_end_transaction", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_ENDTRANSACTIONINP
    },
    {
        SPECIFIC_QUERY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "specificQueryInp_PI", 0, "GenQueryOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,specificQueryInp_t*,genQueryOut_t**)>(RS_SPECIFIC_QUERY)),
        "api_specific_query", clearSpecificQueryInp, clearGenQueryOut,
        (funcPtr)CALL_SPECIFICQUERYINP_GENQUERYOUT
    },
    {
        TICKET_ADMIN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ticketAdminInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,ticketAdminInp_t*)>(RS_TICKET_ADMIN)),
        "api_ticket_admin", clearTicketAdminInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_TICKETADMININP
    },
    {
        GET_TEMP_PASSWORD_FOR_OTHER_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "getTempPasswordForOtherInp_PI", 0,  "getTempPasswordForOtherOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,getTempPasswordForOtherInp_t*,getTempPasswordForOtherOut_t**)>(RS_GET_TEMP_PASSWORD_FOR_OTHER)),
        "api_get_temp_password_for_other", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_GETTEMPPASSWORDFOROTHERINP_GETTEMPPASSWORDFOROTHEROUT
    },
    {
        PAM_AUTH_REQUEST_AN, RODS_API_VERSION,
        NO_USER_AUTH, NO_USER_AUTH,
        "pamAuthRequestInp_PI", 0,  "pamAuthRequestOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,pamAuthRequestInp_t*,pamAuthRequestOut_t**)>(RS_PAM_AUTH_REQUEST)),
        "api_pam_auth_request", irods::clearInStruct_noop, clearPamAuthRequestOut,
        (funcPtr)CALL_PAMAUTHREQUESTINP_PAMAUTHREQUESTOUT
    },
    {
        GET_LIMITED_PASSWORD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "getLimitedPasswordInp_PI", 0,  "getLimitedPasswordOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,getLimitedPasswordInp_t*,getLimitedPasswordOut_t**)>(RS_GET_LIMITED_PASSWORD)),
        "api_get_limited_password", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_GETLIMITEDPASSWORDINP_GETLIMITEDPASSWORDOUT
    },
    {
        OPEN_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,collInp_t*)>(RS_OPEN_COLLECTION)),
        "api_open_collection", clearCollInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_COLLINP
    },
    {
        READ_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, "CollEnt_PI", 0,
        boost::any(std::function<int(rsComm_t*,int*, collEnt_t**)>(RS_READ_COLLECTION)),
        "api_read_collection", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_INTP_COLLENTOUT
    },
    {
        USER_ADMIN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "userAdminInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,userAdminInp_t*)>(RS_USER_ADMIN)),
        "api_user_admin", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_USERADMININP
    },
    {
        GENERAL_ROW_INSERT_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalRowInsertInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,generalRowInsertInp_t*)>(RS_GENERAL_ROW_INSERT)),
        "api_general_row_insert", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_GENERALROWINSERTINP
    },
    {
        GENERAL_ROW_PURGE_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalRowPurgeInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,generalRowPurgeInp_t*)>(RS_GENERAL_ROW_PURGE)),
        "api_general_row_purge", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_GENERALROWPURGEINP
    },
    {
        CLOSE_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,int*)>(RS_CLOSE_COLLECTION)),
        "close_collection", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_INTP
    },
    {
        COLL_REPL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, "CollOprStat_PI", 0,
        boost::any(std::function<int(rsComm_t*,collInp_t*,collOprStat_t**)>(RS_COLL_REPL)),
        "api_coll_repl", clearCollInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_COLLINP_COLLOPRSTATOUT
    },
    {
        RM_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, "CollOprStat_PI", 0,
        boost::any(std::function<int(rsComm_t*,collInp_t*,collOprStat_t**)>(RS_RM_COLL)),
        "api_rm_coll", clearCollInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_COLLINP_COLLOPRSTATOUT
    },
    {
        MOD_AVU_METADATA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ModAVUMetadataInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,modAVUMetadataInp_t*)>(RS_MOD_AVU_METADATA)),
        "api_mod_avu_metadata", clearModAVUMetadataInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_MODAVUMETADATAINP
    },
    {
        MOD_ACCESS_CONTROL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "modAccessControlInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,modAccessControlInp_t*)>(RS_MOD_ACCESS_CONTROL)),
        "api_mod_access_control", clearModAccessControlInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_MODACCESSCONTROLINP
    },
    {
        RULE_EXEC_MOD_AN, RODS_API_VERSION, LOCAL_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "RULE_EXEC_MOD_INP_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,ruleExecModInp_t*)>(RS_RULE_EXEC_MOD)),
        "api_rule_exec_mod", clearRuleExecModifyInput, irods::clearOutStruct_noop,
        (funcPtr)CALL_RULEEXECMODINP
    },
    {
        GET_TEMP_PASSWORD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        NULL, 0,  "getTempPasswordOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,getTempPasswordOut_t**)>(RS_GET_TEMP_PASSWORD)),
        "api_get_temp_password", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_GETTEMPPASSWORDOUT
    },
    {
        GENERAL_UPDATE_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "GeneralUpdateInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,generalUpdateInp_t*)>(RS_GENERAL_UPDATE)),
        "api_general_update", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_GENERALUPDATEINP
    },
    {
        MOD_DATA_OBJ_META_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ModDataObjMeta_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,modDataObjMeta_t*)>(RS_MOD_DATA_OBJ_META)),
        "api_mod_data_obj_meta", clearModDataObjMetaInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_MODDATAOBJMETAINP
    },
    {
        RULE_EXEC_SUBMIT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "RULE_EXEC_SUBMIT_INP_PI", 0, "IRODS_STR_PI", 0,
        boost::any(std::function<int(rsComm_t*,ruleExecSubmitInp_t*,char**)>(RS_RULE_EXEC_SUBMIT)),
        "api_rule_exec_submit", clearRuleExecSubmitInput, irods::clearOutStruct_noop,
        (funcPtr)CALL_RULEEXECSUBMITINP_CHAROUT
    },
    {
        RULE_EXEC_DEL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, LOCAL_USER_AUTH,
        "RULE_EXEC_DEL_INP_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,ruleExecDelInp_t*)>(RS_RULE_EXEC_DEL)),
        "api_rule_exec_del", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_RULEEXECDELINP
    },
    {
        EXEC_MY_RULE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ExecMyRuleInp_PI", 0, "MsParamArray_PI", 0,
        boost::any(std::function<int(rsComm_t*,execMyRuleInp_t*,msParamArray_t**)>(RS_EXEC_MY_RULE)),
        "api_exec_my_rule", clearExecMyRuleInp, [](void *_p) { clearMsParamArray(static_cast<MsParamArray*>(_p), /* freeStruct */ 1); },
        (funcPtr)CALL_EXECMYRULEINP_MSPARAMARRAYOUT
    },
    {
        OPR_COMPLETE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,int*)>(RS_OPR_COMPLETE)),
        "api_opr_complete", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_INTP
    },
    {
        DATA_OBJ_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjCopyInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjCopyInp_t*)>(RS_DATA_OBJ_RENAME)),
        "api_data_obj_rename", clearDataObjCopyInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJCOPYINP
    },
    {
        DATA_OBJ_RSYNC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "MsParamArray_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,msParamArray_t**)>(RS_DATA_OBJ_RSYNC)),
        "api_data_obj_rsync", clearDataObjInp, [](void *_p) { clearMsParamArray(static_cast<MsParamArray*>(_p), /* freeStruct */ 1); },
        (funcPtr)  CALL_DATAOBJINP_MSPARAMARRAYOUT
    },
    {
        DATA_OBJ_CHKSUM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,char**)>(RS_DATA_OBJ_CHKSUM)),
        "api_data_obj_chksum", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_CHAROUT
    },
    {
        PHY_PATH_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_PHY_PATH_REG)),
        "api_phy_path_reg", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        DATA_OBJ_PHYMV_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "TransferStat_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,transferStat_t**)>(RS_DATA_OBJ_PHYMV)),
        "api_data_obj_phymv", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_TRANSFERSTATOUT
    },
    {
        DATA_OBJ_TRIM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_DATA_OBJ_TRIM)),
        "api_data_obj_trim", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        OBJ_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "RodsObjStat_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,rodsObjStat_t**)>(RS_OBJ_STAT)),
        "api_obj_stat", clearDataObjInp, clearRodsObjStat,
        (funcPtr)CALL_DATAOBJINP_RODSOBJSTATOUT
    },
    {
        EXEC_CMD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ExecCmd_PI", 0, "ExecCmdOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,execCmd_t*,execCmdOut_t**)>(RS_EXEC_CMD)),
        "api_exec_cmd", clearExecCmd, clearExecCmdOut,
        (funcPtr)CALL_EXECCMDINP_EXECCMDOUT
    },
    {
        STREAM_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileCloseInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,fileCloseInp_t*)>(RS_STREAM_CLOSE)),
        "api_stream_close", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILECLOSEINP
    },
    {
        GET_HOST_FOR_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,char**)>(RS_GET_HOST_FOR_GET)),
        "api_get_host_for_get", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_CHAROUT
    },
    {
        DATA_OBJ_LOCK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_DATA_OBJ_LOCK)),
        "api_data_obj_lock", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        DATA_OBJ_UNLOCK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_DATA_OBJ_UNLOCK)),
        "api_data_obj_unlock", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        SUB_STRUCT_FILE_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubFile_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*)>(RS_SUB_STRUCT_FILE_CREATE)),
        "api_sub_struct_file_create", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP
    },
    {
        SUB_STRUCT_FILE_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubFile_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*)>(RS_SUB_STRUCT_FILE_OPEN)),
        "api_sub_struct_file_open", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP
    },
    {
        SUB_STRUCT_FILE_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubStructFileFdOpr_PI", 0, NULL, 1,
        boost::any(std::function<int(rsComm_t*,subStructFileFdOprInp_t*,bytesBuf_t*)>(RS_SUB_STRUCT_FILE_READ)),
        "api_sub_struct_file_read", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBSTRUCTFILEFDOPRINP_BYTESBUFOUT
    },
    {
        SUB_STRUCT_FILE_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubStructFileFdOpr_PI", 1, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subStructFileFdOprInp_t*,bytesBuf_t*)>(RS_SUB_STRUCT_FILE_WRITE)),
        "api_sub_struct_file_write", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBSTRUCTFILEFDOPRINP_BYTESBUFOUT
    },
    {
        SUB_STRUCT_FILE_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubStructFileFdOpr_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subStructFileFdOprInp_t*)>(RS_SUB_STRUCT_FILE_CLOSE)),
        "api_sub_struct_file_close", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBSTRUCTFILEFDOPRINP
    },
    {
        SUB_STRUCT_FILE_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubFile_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*)>(RS_SUB_STRUCT_FILE_UNLINK)),
        "api_sub_struct_file_unlink", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP
    },
    {
        SUB_STRUCT_FILE_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubFile_PI", 0, "RODS_STAT_T_PI", 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*,rodsStat_t**)>(RS_SUB_STRUCT_FILE_STAT)),
        "api_sub_struct_file_stat", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP_RODSSTATOUT
    },
    {
        SUB_STRUCT_FILE_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubStructFileLseekInp_PI", 0, "fileLseekOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,subStructFileLseekInp_t*,fileLseekOut_t**)>(RS_SUB_STRUCT_FILE_LSEEK)),
        "api_sub_struct_file_lseek", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBSTRUCTLSEEKINP_FILELSEEKOUT
    },
    {
        SUB_STRUCT_FILE_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubStructFileRenameInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subStructFileRenameInp_t*)>(RS_SUB_STRUCT_FILE_RENAME)),
        "api_sub_struct_file_rename", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBSTRUCTFILERENAMEINP
    },
    {
        QUERY_SPEC_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "GenQueryOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,genQueryOut_t**)>(RS_QUERY_SPEC_COLL)),
        "api_query_spec_coll", clearDataObjInp, clearGenQueryOut,
        (funcPtr)CALL_DATAOBJINP_GENQUERYOUT
    },
    {
        MOD_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,collInp_t*)>(RS_MOD_COLL)),
        "api_mod_coll", clearCollInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_COLLINP
    },
    {
        SUB_STRUCT_FILE_MKDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubFile_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*)>(RS_SUB_STRUCT_FILE_MKDIR)),
        "api_sub_struct_file_mkdir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP
    },
    {
        SUB_STRUCT_FILE_RMDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubFile_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*)>(RS_SUB_STRUCT_FILE_RMDIR)),
        "api_sub_struct_file_rmdir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP
    },
    {
        SUB_STRUCT_FILE_OPENDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubFile_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*)>(RS_SUB_STRUCT_FILE_OPENDIR)),
        "api_sub_struct_file_opendir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP
    },
    {
        SUB_STRUCT_FILE_READDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubStructFileFdOpr_PI", 0, "RODS_DIRENT_T_PI", 0,
        boost::any(std::function<int(rsComm_t*,subStructFileFdOprInp_t*,rodsDirent_t**)>(RS_SUB_STRUCT_FILE_READDIR)),
        "api_sub_struct_file_readdir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBSTRUCTFILEFDOPRINP_RODSDIRENTOUT
    },
    {
        SUB_STRUCT_FILE_CLOSEDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubStructFileFdOpr_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subStructFileFdOprInp_t*)>(RS_SUB_STRUCT_FILE_CLOSEDIR)),
        "api_sub_struct_file_closedir", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBSTRUCTFILEFDOPRINP
    },
    {
        DATA_OBJ_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_DATA_OBJ_TRUNCATE)),
        "api_data_obj_truncate", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        SUB_STRUCT_FILE_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "SubFile_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*)>(RS_SUB_STRUCT_FILE_TRUNCATE)),
        "api_sub_struct_file_truncate", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP
    },
    {
        SUB_STRUCT_FILE_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "SubFile_PI", 0, NULL, 1,
        boost::any(std::function<int(rsComm_t*,subFile_t*,bytesBuf_t*)>(RS_SUB_STRUCT_FILE_GET)),
        "api_sub_struct_file_get", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP_BYTESBUFOUT
    },
    {
        SUB_STRUCT_FILE_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "SubFile_PI", 1, NULL, 0,
        boost::any(std::function<int(rsComm_t*,subFile_t*,bytesBuf_t*)>(RS_SUB_STRUCT_FILE_PUT)),
        "api_sub_struct_file_put", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SUBFILEINP_BYTESBUFOUT
    },
    {
        SYNC_MOUNTED_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_SYNC_MOUNTED_COLL)),
        "api_sync_mounted_coll", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        STRUCT_FILE_SYNC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileOprInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,structFileOprInp_t*)>(RS_STRUCT_FILE_SYNC)),
        "api_struct_file_sync", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_STRUCTFILEOPRINP
    },
    {
        COLL_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,collInp_t*)>(RS_COLL_CREATE)),
        "api_coll_create", clearCollInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_COLLINP
    },
    {
        STRUCT_FILE_EXTRACT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileOprInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,structFileOprInp_t*)>(RS_STRUCT_FILE_EXTRACT)),
        "api_struct_file_extract", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_STRUCTFILEOPRINP
    },
    {
        STRUCT_FILE_EXT_AND_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileExtAndRegInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,structFileExtAndRegInp_t*)>(RS_STRUCT_FILE_EXT_AND_REG)),
        "api_struct_file_ext_and_reg", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_STRUCTFILEEXTANDREGINP
    },
    {
        STRUCT_FILE_BUNDLE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileExtAndRegInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,structFileExtAndRegInp_t*)>(RS_STRUCT_FILE_BUNDLE)),
        "api_struct_file_bundle", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_STRUCTFILEEXTANDREGINP
    },
    {
        CHK_OBJ_PERM_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ChkObjPermAndStat_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,chkObjPermAndStat_t*)>(RS_CHK_OBJ_PERM_AND_STAT)),
        "api_chk_obj_perm_and_stat", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_CHKOBJPERMANDSTATINP
    },
    {
        GET_REMOTE_ZONE_RESC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "RHostAddr_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,rodsHostAddr_t**)>(RS_GET_REMOTE_ZONE_RESC)),
        "api_get_remote_zone_resc", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr) CALL_DATAOBJINP_RODSHOSTADDROUT
    },
    {
        DATA_OBJ_OPEN_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "OpenStat_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,openStat_t**)>(RS_DATA_OBJ_OPEN_AND_STAT)),
        "api_data_obj_open_and_stat", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_OPENSTATOUT
    },
    {
        L3_FILE_GET_SINGLE_BUF_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 1,
        boost::any(std::function<int(rsComm_t*,int*,bytesBuf_t*)>(RS_L3_FILE_GET_SINGLE_BUF)),
        "api_l3_file_get_single_buf", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_INTINP_BYTESBUFOUT
    },
    {
        L3_FILE_PUT_SINGLE_BUF_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 1, NULL, 0,
        boost::any(std::function<int(rsComm_t*,int*,bytesBuf_t*)>(RS_L3_FILE_PUT_SINGLE_BUF)),
        "api_l3_file_put_single_buf", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_INTINP_BYTESBUFOUT
    },
    {
        DATA_OBJ_CREATE_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "OpenStat_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,openStat_t**)>(RS_DATA_OBJ_CREATE_AND_STAT)),
        "api_data_obj_create_and_stat", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_OPENSTATOUT
    },
    {
        UNBUN_AND_REG_PHY_BUNFILE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*)>(RS_UNBUN_AND_REG_PHY_BUNFILE)),
        "api_unbun_and_reg_phy_bunfile", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP
    },
    {
        GET_HOST_FOR_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInp_t*,char**)>(RS_GET_HOST_FOR_PUT)),
        "api_get_host_for_put", clearDataObjInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_DATAOBJINP_CHAROUT
    },
    {
        GET_RESC_QUOTA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "getRescQuotaInp_PI", 0, "rescQuota_PI", 0,
        boost::any(std::function<int(rsComm_t*,getRescQuotaInp_t*,rescQuota_t**)>(RS_GET_RESC_QUOTA)),
        "api_get_resc_quota", irods::clearInStruct_noop, clearDataObjInfo,
        (funcPtr)CALL_GETRESCQUOTAINP_RESCQUOTAOUT
    },
    {
        BULK_DATA_OBJ_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "GenQueryOut_PI", 0, "GenQueryOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,genQueryOut_t*,genQueryOut_t**)>(RS_BULK_DATA_OBJ_REG)),
        "api_bulk_data_obj_reg", clearGenQueryOut, clearGenQueryOut,
        (funcPtr)CALL_GENQUERYOUTINP_GENQUERYOUT
    },
    {
        BULK_DATA_OBJ_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "BulkOprInp_PI", 1, NULL, 0,
        boost::any(std::function<int(rsComm_t*,bulkOprInp_t*,bytesBuf_t*)>(RS_BULK_DATA_OBJ_PUT)),
        "api_bulk_data_obj_put", clearBulkOprInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_BULKOPRINP_BYTESBUFOUT
    },
    {
        PROC_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ProcStatInp_PI", 0, "GenQueryOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,procStatInp_t*,genQueryOut_t**)>(RS_PROC_STAT)),
        "api_proc_stat", irods::clearInStruct_noop, clearGenQueryOut,
        (funcPtr)CALL_PROCSTATINP_GENQUERYOUT
    },
    {
        STREAM_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileReadInp_PI", 0, NULL, 1,
        boost::any(std::function<int(rsComm_t*,fileReadInp_t*,bytesBuf_t*)>(RS_STREAM_READ)),
        "api_stream_read", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILEREADINP_BYTESBUFOUT
    },
    {
        REG_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,collInp_t*)>(RS_REG_COLL)),
        "api_reg_coll", clearCollInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_COLLINP
    },
    {
        REG_DATA_OBJ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInfo_PI", 0, "DataObjInfo_PI", 0,
        boost::any(std::function<int(rsComm_t*,dataObjInfo_t*,dataObjInfo_t**)>(RS_REG_DATA_OBJ)),
        "api_reg_data_obj", irods::clearInStruct_noop, [](void* _p) { freeDataObjInfo(static_cast<DataObjInfo*>(_p)); },
        (funcPtr)CALL_DATAOBJINFOINP_DATAOBJINFOOUT
    },
    {
        UNREG_DATA_OBJ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "UnregDataObj_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,unregDataObj_t*)>(RS_UNREG_DATA_OBJ)),
        "api_unreg_data_obj", clearUnregDataObj, irods::clearOutStruct_noop,
        (funcPtr)CALL_UNREGDATAOBJINP
    },
    {
        REG_REPLICA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "RegReplica_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,regReplica_t*)>(RS_REG_REPLICA)),
        "api_reg_replica", clearRegReplicaInp, irods::clearOutStruct_noop,
        (funcPtr)CALL_REGREPLICAINP
    },
    {
        FILE_CHKSUM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileChksumInp_PI", 0, "fileChksumOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,fileChksumInp_t*,char**)>(RS_FILE_CHKSUM)),
        "api_file_chksum", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_FILECHKSUMINP_CHAROUT
    },
    {
        SSL_START_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "sslStartInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,sslStartInp_t*)>(RS_SSL_START)),
        "api_ssl_start", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SSLSTARTINP
    },
    {
        SSL_END_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "sslEndInp_PI", 0, NULL, 0,
        boost::any(std::function<int(rsComm_t*,sslEndInp_t*)>(RS_SSL_END)),
        "api_ssl_end", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_SSLENDINP
    },
    {
        AUTH_PLUG_REQ_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "authPlugReqInp_PI", 0, "authPlugReqOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,authPluginReqInp_t*,authPluginReqOut_t**)>(RS_AUTH_PLUG_REQ)),
        "api_auth_plugin_request", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_AUTHPLUGINREQINP_AUTHPLUGINREQOUT
    },
    {
        GET_HIER_FOR_RESC_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "getHierarchyForRescInp_PI", 0, "getHierarchyForRescOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,getHierarchyForRescInp_t*,getHierarchyForRescOut_t**)>(RS_GET_HIER_FOR_RESC)),
        "api_get_hierarchy_for_resc", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_GETHIERARCHYFORRESCINP_GETHIERARCHYFORRESCOUT
    },
    {
        ZONE_REPORT_AN, RODS_API_VERSION, LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        NULL, 0,  "BytesBuf_PI", 0,
        boost::any(std::function<int(rsComm_t*,bytesBuf_t**)>(RS_ZONE_REPORT)),
        "api_zone_report", irods::clearInStruct_noop, clearBytesBuffer,
        (funcPtr)CALL_BYTESBUFOUT
    },
    {
        SERVER_REPORT_AN, RODS_API_VERSION, REMOTE_PRIV_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        NULL, 0,  "BytesBuf_PI", 0,
        boost::any(std::function<int(rsComm_t*,bytesBuf_t**)>(RS_SERVER_REPORT)),
        "api_server_report", irods::clearInStruct_noop, clearBytesBuffer,
        (funcPtr)CALL_BYTESBUFOUT
    },
    {
        CLIENT_HINTS_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        NULL, 0,  "BytesBuf_PI", 0,
        boost::any(std::function<int(rsComm_t*,bytesBuf_t**)>(RS_CLIENT_HINTS)),
        "api_client_hints", irods::clearInStruct_noop, clearBytesBuffer,
        (funcPtr)CALL_BYTESBUFOUT
    },
    {
        IES_CLIENT_HINTS_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        NULL, 0,  "BytesBuf_PI", 0,
        boost::any(std::function<int(rsComm_t*,bytesBuf_t**)>(RS_IES_CLIENT_HINTS)),
        "api_ies_client_hints", irods::clearInStruct_noop, clearBytesBuffer,
        (funcPtr)CALL_BYTESBUFOUT
    },
    {
        GET_HIER_FROM_LEAF_ID_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "GetHierInp_PI", 0,  "GetHierOut_PI", 0,
        boost::any(std::function<int(rsComm_t*,get_hier_inp_t*,get_hier_out_t**)>(RS_GET_HIER_FROM_LEAF_ID)),
        "api_get_hier_from_leaf_id", irods::clearInStruct_noop, irods::clearOutStruct_noop,
        (funcPtr)CALL_GETHIERINP_GETHIEROUT
    },
    {
        EXEC_RULE_EXPRESSION_AN, RODS_API_VERSION, LOCAL_USER_AUTH, LOCAL_USER_AUTH,
        "ExecRuleExpression_PI", 0,  NULL, 0,
        boost::any(std::function<int(rsComm_t*,exec_rule_expression_t*)>(RS_EXEC_RULE_EXPRESSION)),
        "api_exec_rule_expression", clearExecRuleExpressionInput, irods::clearOutStruct_noop,
        (funcPtr)CALL_EXECRULEEXPRESSIONINP
    },
    // clang-format on
}; // _api_table_inp

#endif // IRODS_API_TABLE_HPP
