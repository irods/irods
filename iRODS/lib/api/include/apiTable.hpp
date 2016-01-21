/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* apiTable.h - header file for The global API table
 */



#ifndef API_TABLE_HPP
#define API_TABLE_HPP

#include "rcMisc.h"
#include "rods.h"
#include "apiHandler.hpp"
#include "apiNumber.h"
#include "rodsUser.h"



/* need to include a header for for each API */
#include "apiHeaderAll.h"


#include "server_report.h"
#include "zone_report.h"
#include "client_hints.h"
#include "ies_client_hints.h"



#if defined(RODS_SERVER)
static irods::apidef_t server_api_table_inp[] = {
#else	/* client */
static irods::apidef_t client_api_table_inp[] = {
#endif
    {
        GET_MISC_SVR_INFO_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH, NULL,
        0, "MiscSvrInfo_PI", 0, ( funcPtr ) RS_GET_MISC_SVR_INFO, irods::clearInStruct_noop
    },
    {
        FILE_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, "fileCreateOut_PI", 0, ( funcPtr ) RS_FILE_CREATE, clearFileOpenInp
    },
    {
        FILE_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_OPEN, clearFileOpenInp
    },
    {
        CHK_N_V_PATH_PERM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0, ( funcPtr ) RS_CHK_NV_PATH_PERM, clearFileOpenInp
    },
    {
        FILE_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileWriteInp_PI", 1, NULL, 0, ( funcPtr ) RS_FILE_WRITE, irods::clearInStruct_noop
    },
    {
        FILE_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileCloseInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_CLOSE, irods::clearInStruct_noop
    },
    {
        FILE_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileLseekInp_PI", 0, "fileLseekOut_PI", 0, ( funcPtr ) RS_FILE_LSEEK, irods::clearInStruct_noop
    },
    {
        FILE_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileReadInp_PI", 0, NULL, 1, ( funcPtr ) RS_FILE_READ, irods::clearInStruct_noop
    },
    {
        FILE_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileUnlinkInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_UNLINK, irods::clearInStruct_noop
    },
    {
        FILE_MKDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileMkdirInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_MKDIR, irods::clearInStruct_noop
    },
    {
        FILE_CHMOD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileChmodInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_CHMOD, irods::clearInStruct_noop
    },
    {
        FILE_RMDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileRmdirInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_RMDIR, irods::clearInStruct_noop
    },
    {
        FILE_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStatInp_PI", 0,  "RODS_STAT_T_PI", 0, ( funcPtr ) RS_FILE_STAT, irods::clearInStruct_noop
    },
    {
        FILE_GET_FS_FREE_SPACE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileGetFsFreeSpaceInp_PI", 0, "fileGetFsFreeSpaceOut_PI", 0, ( funcPtr ) RS_FILE_GET_FS_FREE_SPACE, irods::clearInStruct_noop
    },
    {
        FILE_OPENDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpendirInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_OPENDIR, irods::clearInStruct_noop
    },
    {
        FILE_CLOSEDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileClosedirInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_CLOSEDIR, irods::clearInStruct_noop
    },
    {
        FILE_READDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileReaddirInp_PI", 0, "RODS_DIRENT_T_PI", 0, ( funcPtr ) RS_FILE_READDIR, irods::clearInStruct_noop
    },
    {
        FILE_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileRenameInp_PI", 0, "fileRenameOut_PI", 0, ( funcPtr ) RS_FILE_RENAME, irods::clearInStruct_noop
    },
    {
        FILE_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_TRUNCATE, clearFileOpenInp
    },
    {
        FILE_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 1, "filePutOut_PI", 0, ( funcPtr ) RS_FILE_PUT, clearFileOpenInp
    },
    {
        FILE_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 1, ( funcPtr ) RS_FILE_GET, clearFileOpenInp
    },
    {
        FILE_STAGE_TO_CACHE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStageSyncInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_STAGE_TO_CACHE, irods::clearInStruct_noop
    },
    {
        FILE_SYNC_TO_ARCH_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStageSyncInp_PI", 0, "fileSyncOut_PI", 0, ( funcPtr ) RS_FILE_SYNC_TO_ARCH, irods::clearInStruct_noop
    },
    {
        DATA_OBJ_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_CREATE, clearDataObjInp
    },
    {
        DATA_OBJ_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_OPEN, clearDataObjInp
    },
    {
        DATA_OBJ_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, NULL, 1, ( funcPtr ) RS_DATA_OBJ_READ, irods::clearInStruct_noop
    },
    {
        DATA_OBJ_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 1, NULL, 0, ( funcPtr ) RS_DATA_OBJ_WRITE, irods::clearInStruct_noop
    },
    {
        DATA_OBJ_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_CLOSE, irods::clearInStruct_noop
    },
    {
        DATA_OBJ_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 1,  "PortalOprOut_PI", 0, ( funcPtr ) RS_DATA_OBJ_PUT, clearDataObjInp
    },
    {
        DATA_OBJ_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "PortalOprOut_PI", 1, ( funcPtr ) RS_DATA_OBJ_GET, clearDataObjInp
    },
    {
        DATA_OBJ_REPL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "TransferStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_REPL, clearDataObjInp
    },
    {
        DATA_OBJ_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, "fileLseekOut_PI", 0,
        ( funcPtr ) RS_DATA_OBJ_LSEEK, irods::clearInStruct_noop
    },
    {
        DATA_OBJ_COPY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjCopyInp_PI", 0, "TransferStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_COPY, clearDataObjCopyInp
    },
    {
        DATA_OBJ_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_UNLINK, clearDataObjInp
    },
    {
        DATA_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataOprInp_PI", 0, "PortalOprOut_PI", 0, ( funcPtr ) RS_DATA_PUT, irods::clearInStruct_noop
    },
    {
        DATA_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataOprInp_PI", 0, "PortalOprOut_PI", 0, ( funcPtr ) RS_DATA_GET, irods::clearInStruct_noop
    },
    {
        DATA_COPY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataCopyInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_COPY, irods::clearInStruct_noop
    },
    {
        SIMPLE_QUERY_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "simpleQueryInp_PI", 0,  "simpleQueryOut_PI", 0, ( funcPtr ) RS_SIMPLE_QUERY, irods::clearInStruct_noop
    },
    {
        GENERAL_ADMIN_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalAdminInp_PI", 0, NULL, 0, ( funcPtr ) RS_GENERAL_ADMIN, irods::clearInStruct_noop
    },
    {
        GEN_QUERY_AN, RODS_API_VERSION,
        REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "GenQueryInp_PI", 0, "GenQueryOut_PI", 0, ( funcPtr ) RS_GEN_QUERY, clearGenQueryInp
    },
    {
        AUTH_REQUEST_AN, RODS_API_VERSION, NO_USER_AUTH | XMSG_SVR_ALSO,
        NO_USER_AUTH | XMSG_SVR_ALSO,
        NULL, 0,  "authRequestOut_PI", 0, ( funcPtr ) RS_AUTH_REQUEST, irods::clearInStruct_noop
    },
    {
        AUTH_RESPONSE_AN, RODS_API_VERSION, NO_USER_AUTH | XMSG_SVR_ALSO,
        NO_USER_AUTH | XMSG_SVR_ALSO,
        "authResponseInp_PI", 0, NULL, 0, ( funcPtr ) RS_AUTH_RESPONSE, clearAuthResponseInp
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
        "authCheckInp_PI", 0,  "authCheckOut_PI", 0, ( funcPtr ) RS_AUTH_CHECK, irods::clearInStruct_noop
    },
    {
        END_TRANSACTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "endTransactionInp_PI", 0, NULL, 0, ( funcPtr ) RS_END_TRANSACTION, irods::clearInStruct_noop
    },
    {
        SPECIFIC_QUERY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "specificQueryInp_PI", 0, "GenQueryOut_PI", 0, ( funcPtr ) RS_SPECIFIC_QUERY, irods::clearInStruct_noop
    },
    {
        TICKET_ADMIN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ticketAdminInp_PI", 0, NULL, 0, ( funcPtr ) RS_TICKET_ADMIN, irods::clearInStruct_noop
    },
    {
        GET_TEMP_PASSWORD_FOR_OTHER_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "getTempPasswordForOtherInp_PI", 0,  "getTempPasswordForOtherOut_PI", 0,
        ( funcPtr ) RS_GET_TEMP_PASSWORD_FOR_OTHER, irods::clearInStruct_noop
    },
    {
        PAM_AUTH_REQUEST_AN, RODS_API_VERSION,
        NO_USER_AUTH | XMSG_SVR_ALSO, NO_USER_AUTH | XMSG_SVR_ALSO,
        "pamAuthRequestInp_PI", 0,  "pamAuthRequestOut_PI", 0, ( funcPtr ) RS_PAM_AUTH_REQUEST, irods::clearInStruct_noop
    },
    {
        GET_LIMITED_PASSWORD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "getLimitedPasswordInp_PI", 0,  "getLimitedPasswordOut_PI",
        0, ( funcPtr ) RS_GET_LIMITED_PASSWORD, irods::clearInStruct_noop
    },
    {
        OPEN_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_OPEN_COLLECTION, clearCollInp
    },
    {
        READ_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, "CollEnt_PI", 0, ( funcPtr ) RS_READ_COLLECTION, irods::clearInStruct_noop
    },
    {
        USER_ADMIN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "userAdminInp_PI", 0, NULL, 0, ( funcPtr ) RS_USER_ADMIN, irods::clearInStruct_noop
    },
    {
        GENERAL_ROW_INSERT_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalRowInsertInp_PI", 0, NULL, 0, ( funcPtr ) RS_GENERAL_ROW_INSERT, irods::clearInStruct_noop
    },
    {
        GENERAL_ROW_PURGE_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalRowPurgeInp_PI", 0, NULL, 0, ( funcPtr ) RS_GENERAL_ROW_PURGE, irods::clearInStruct_noop
    },
    {
        CLOSE_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 0, ( funcPtr ) RS_CLOSE_COLLECTION, irods::clearInStruct_noop
    },
    {
        COLL_REPL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, "CollOprStat_PI", 0, ( funcPtr ) RS_COLL_REPL, clearCollInp
    },
    {
        RM_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, "CollOprStat_PI", 0, ( funcPtr ) RS_RM_COLL, clearCollInp
    },
    {
        MOD_AVU_METADATA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ModAVUMetadataInp_PI", 0, NULL, 0, ( funcPtr ) RS_MOD_AVU_METADATA, clearModAVUMetadataInp
    },
    {
        MOD_ACCESS_CONTROL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "modAccessControlInp_PI", 0, NULL, 0, ( funcPtr ) RS_MOD_ACCESS_CONTROL, irods::clearInStruct_noop
    },
    {
        RULE_EXEC_MOD_AN, RODS_API_VERSION, LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "RULE_EXEC_MOD_INP_PI", 0, NULL, 0, ( funcPtr ) RS_RULE_EXEC_MOD, irods::clearInStruct_noop
    },
    {
        GET_TEMP_PASSWORD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        NULL, 0,  "getTempPasswordOut_PI", 0, ( funcPtr ) RS_GET_TEMP_PASSWORD, irods::clearInStruct_noop
    },
    {
        GENERAL_UPDATE_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "GeneralUpdateInp_PI", 0, NULL, 0, ( funcPtr ) RS_GENERAL_UPDATE, irods::clearInStruct_noop
    },
    {
        MOD_DATA_OBJ_META_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ModDataObjMeta_PI", 0, NULL, 0, ( funcPtr ) RS_MOD_DATA_OBJ_META, clearModDataObjMetaInp
    },
    {
        RULE_EXEC_SUBMIT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "RULE_EXEC_SUBMIT_INP_PI", 0, "IRODS_STR_PI", 0, ( funcPtr ) RS_RULE_EXEC_SUBMIT, irods::clearInStruct_noop
    },
    {
        RULE_EXEC_DEL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, LOCAL_USER_AUTH,
        "RULE_EXEC_DEL_INP_PI", 0, NULL, 0, ( funcPtr ) RS_RULE_EXEC_DEL, irods::clearInStruct_noop
    },
    {
        EXEC_MY_RULE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ExecMyRuleInp_PI", 0, "MsParamArray_PI", 0, ( funcPtr ) RS_EXEC_MY_RULE, irods::clearInStruct_noop
    },
    {
        OPR_COMPLETE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 0, ( funcPtr ) RS_OPR_COMPLETE, irods::clearInStruct_noop
    },
    {
        DATA_OBJ_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjCopyInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_RENAME, clearDataObjCopyInp
    },
    {
        DATA_OBJ_RSYNC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "MsParamArray_PI", 0, ( funcPtr ) RS_DATA_OBJ_RSYNC, clearDataObjInp
    },
    {
        DATA_OBJ_CHKSUM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0, ( funcPtr ) RS_DATA_OBJ_CHKSUM, clearDataObjInp
    },
    {
        PHY_PATH_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_PHY_PATH_REG, clearDataObjInp
    },
    {
        DATA_OBJ_PHYMV_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "TransferStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_PHYMV, clearDataObjInp
    },
    {
        DATA_OBJ_TRIM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_TRIM, clearDataObjInp
    },
    {
        OBJ_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "RodsObjStat_PI", 0, ( funcPtr ) RS_OBJ_STAT, clearDataObjInp
    },
    {
        EXEC_CMD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ExecCmd_PI", 0, "ExecCmdOut_PI", 0, ( funcPtr ) RS_EXEC_CMD, irods::clearInStruct_noop
    },
    {
        STREAM_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileCloseInp_PI", 0, NULL, 0, ( funcPtr ) RS_STREAM_CLOSE, irods::clearInStruct_noop
    },
    {
        GET_HOST_FOR_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0, ( funcPtr ) RS_GET_HOST_FOR_GET, clearDataObjInp
    },
    {
        DATA_OBJ_LOCK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_LOCK, clearDataObjInp
    },
    {
        DATA_OBJ_UNLOCK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_UNLOCK, clearDataObjInp
    },
    {
        SUB_STRUCT_FILE_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_CREATE, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_OPEN, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 0, NULL, 1,
        ( funcPtr ) RS_SUB_STRUCT_FILE_READ, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 1, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_WRITE, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_CLOSE, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_UNLINK, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, "RODS_STAT_T_PI", 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_STAT, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileLseekInp_PI", 0, "fileLseekOut_PI", 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_LSEEK, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileRenameInp_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_RENAME, irods::clearInStruct_noop
    },
    {
        QUERY_SPEC_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "GenQueryOut_PI", 0,
        ( funcPtr ) RS_QUERY_SPEC_COLL, clearDataObjInp
    },
    {
        MOD_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_MOD_COLL, clearCollInp
    },
    {
        SUB_STRUCT_FILE_MKDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_MKDIR, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_RMDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_RMDIR, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_OPENDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_OPENDIR, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_READDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 0, "RODS_DIRENT_T_PI", 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_READDIR, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_CLOSEDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_CLOSEDIR, irods::clearInStruct_noop
    },
    {
        DATA_OBJ_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_TRUNCATE, clearDataObjInp
    },
    {
        SUB_STRUCT_FILE_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_TRUNCATE, irods::clearInStruct_noop
    },
    {
        GET_XMSG_TICKET_AN, RODS_API_VERSION, REMOTE_USER_AUTH | XMSG_SVR_ONLY,
        REMOTE_USER_AUTH | XMSG_SVR_ONLY,
        "GetXmsgTicketInp_PI", 0, "XmsgTicketInfo_PI", 0,
        ( funcPtr ) RS_GET_XMSG_TICKET, irods::clearInStruct_noop
    },
    {
        SEND_XMSG_AN, RODS_API_VERSION, XMSG_SVR_ONLY, XMSG_SVR_ONLY,
        "SendXmsgInp_PI", 0, NULL, 0, ( funcPtr ) RS_SEND_XMSG, irods::clearInStruct_noop
    },
    {
        RCV_XMSG_AN, RODS_API_VERSION, XMSG_SVR_ONLY, XMSG_SVR_ONLY,
        "RcvXmsgInp_PI", 0, "RcvXmsgOut_PI", 0, ( funcPtr ) RS_RCV_XMSG, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "SubFile_PI", 0, NULL, 1, ( funcPtr ) RS_SUB_STRUCT_FILE_GET, irods::clearInStruct_noop
    },
    {
        SUB_STRUCT_FILE_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "SubFile_PI", 1, NULL, 0, ( funcPtr ) RS_SUB_STRUCT_FILE_PUT, irods::clearInStruct_noop
    },
    {
        SYNC_MOUNTED_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_SYNC_MOUNTED_COLL, clearDataObjInp
    },
    {
        STRUCT_FILE_SYNC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileOprInp_PI", 0, NULL, 0, ( funcPtr ) RS_STRUCT_FILE_SYNC, irods::clearInStruct_noop
    },
    {
        COLL_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_COLL_CREATE, clearCollInp
    },
    {
        STRUCT_FILE_EXTRACT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileOprInp_PI", 0, NULL, 0, ( funcPtr ) RS_STRUCT_FILE_EXTRACT, irods::clearInStruct_noop
    },
    {
        STRUCT_FILE_EXT_AND_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileExtAndRegInp_PI", 0, NULL, 0, ( funcPtr ) RS_STRUCT_FILE_EXT_AND_REG, irods::clearInStruct_noop
    },
    {
        STRUCT_FILE_BUNDLE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileExtAndRegInp_PI", 0, NULL, 0, ( funcPtr ) RS_STRUCT_FILE_BUNDLE, irods::clearInStruct_noop
    },
    {
        CHK_OBJ_PERM_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ChkObjPermAndStat_PI", 0, NULL, 0, ( funcPtr ) RS_CHK_OBJ_PERM_AND_STAT, irods::clearInStruct_noop
    },
    {
        GET_REMOTE_ZONE_RESC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "RHostAddr_PI", 0, ( funcPtr ) RS_GET_REMOTE_ZONE_RESC, clearDataObjInp
    },
    {
        DATA_OBJ_OPEN_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "OpenStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_OPEN_AND_STAT, clearDataObjInp
    },
    {
        L3_FILE_GET_SINGLE_BUF_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 1, ( funcPtr ) RS_L3_FILE_GET_SINGLE_BUF, irods::clearInStruct_noop
    },
    {
        L3_FILE_PUT_SINGLE_BUF_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 1, NULL, 0, ( funcPtr ) RS_L3_FILE_PUT_SINGLE_BUF, irods::clearInStruct_noop
    },
    {
        DATA_OBJ_CREATE_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "OpenStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_CREATE_AND_STAT, clearDataObjInp
    },
    {
        PHY_BUNDLE_COLL_AN, RODS_API_VERSION, LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "StructFileExtAndRegInp_PI", 0, NULL, 0, ( funcPtr ) RS_PHY_BUNDLE_COLL, irods::clearInStruct_noop
    },
    {
        UNBUN_AND_REG_PHY_BUNFILE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_UNBUN_AND_REG_PHY_BUNFILE, clearDataObjInp
    },
    {
        GET_HOST_FOR_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0, ( funcPtr ) RS_GET_HOST_FOR_PUT, clearDataObjInp
    },
    {
        GET_RESC_QUOTA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "getRescQuotaInp_PI", 0, "rescQuota_PI", 0, ( funcPtr ) RS_GET_RESC_QUOTA, irods::clearInStruct_noop
    },
    {
        BULK_DATA_OBJ_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "GenQueryOut_PI", 0, "GenQueryOut_PI", 0, ( funcPtr ) RS_BULK_DATA_OBJ_REG, clearGenQueryOut
    },
    {
        BULK_DATA_OBJ_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "BulkOprInp_PI", 1, NULL, 0, ( funcPtr ) RS_BULK_DATA_OBJ_PUT, clearBulkOprInp
    },
    {
        PROC_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ProcStatInp_PI", 0, "GenQueryOut_PI", 0, ( funcPtr ) RS_PROC_STAT, irods::clearInStruct_noop
    },
    {
        STREAM_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileReadInp_PI", 0, NULL, 1, ( funcPtr ) RS_STREAM_READ, irods::clearInStruct_noop
    },
    {
        REG_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_REG_COLL, clearCollInp
    },
    {
        REG_DATA_OBJ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInfo_PI", 0, "DataObjInfo_PI", 0, ( funcPtr ) RS_REG_DATA_OBJ, irods::clearInStruct_noop
    },
    {
        UNREG_DATA_OBJ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "UnregDataObj_PI", 0, NULL, 0, ( funcPtr ) RS_UNREG_DATA_OBJ, clearUnregDataObj
    },
    {
        REG_REPLICA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "RegReplica_PI", 0, NULL, 0, ( funcPtr ) RS_REG_REPLICA, clearRegReplicaInp
    },
    {
        FILE_CHKSUM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileChksumInp_PI", 0, "fileChksumOut_PI", 0, ( funcPtr ) RS_FILE_CHKSUM, irods::clearInStruct_noop
    },
    {
        SSL_START_AN, RODS_API_VERSION,
        NO_USER_AUTH | XMSG_SVR_ALSO, NO_USER_AUTH | XMSG_SVR_ALSO,
        "sslStartInp_PI", 0, NULL, 0, ( funcPtr ) RS_SSL_START, irods::clearInStruct_noop
    },
    {
        SSL_END_AN, RODS_API_VERSION,
        NO_USER_AUTH | XMSG_SVR_ALSO, NO_USER_AUTH | XMSG_SVR_ALSO,
        "sslEndInp_PI", 0, NULL, 0, ( funcPtr ) RS_SSL_END, irods::clearInStruct_noop
    },
    {
        AUTH_PLUG_REQ_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "authPlugReqInp_PI", 0, "authPlugReqOut_PI", 0, ( funcPtr ) RS_AUTH_PLUG_REQ, irods::clearInStruct_noop
    },
    {
        GET_HIER_FOR_RESC_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "getHierarchyForRescInp_PI", 0, "getHierarchyForRescOut_PI", 0, ( funcPtr ) RS_GET_HIER_FOR_RESC, irods::clearInStruct_noop
    },
    {
        ZONE_REPORT_AN, RODS_API_VERSION, LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        NULL, 0,  "BytesBuf_PI", 0, ( funcPtr ) RS_ZONE_REPORT, irods::clearInStruct_noop
    },
    {
        SERVER_REPORT_AN, RODS_API_VERSION, REMOTE_PRIV_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        NULL, 0,  "BytesBuf_PI", 0, ( funcPtr ) RS_SERVER_REPORT, irods::clearInStruct_noop
    },
    {
        CLIENT_HINTS_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        NULL, 0,  "BytesBuf_PI", 0, ( funcPtr ) RS_CLIENT_HINTS, irods::clearInStruct_noop
    },
    {
        IES_CLIENT_HINTS_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        NULL, 0,  "BytesBuf_PI", 0, ( funcPtr ) RS_IES_CLIENT_HINTS, irods::clearInStruct_noop
    },
    {
        GET_HIER_FROM_LEAF_ID_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "GetHierInp_PI", 0,  "GetHierOut_PI", 0, ( funcPtr ) RS_GET_HIER_FROM_LEAF_ID,
        irods::clearInStruct_noop
    },
    {
        SET_RR_CTX_AN, RODS_API_VERSION, LOCAL_USER_AUTH, LOCAL_USER_AUTH,
        "SetRoundRobinContextInp_PI", 0,  NULL, 0, ( funcPtr ) RS_SET_ROUNDROBIN_CONTEXT,
        irods::clearInStruct_noop
    },
}; // _api_table_inp


#ifdef RODS_SERVER	/* depends on client lib for NumOfApi */
#else
//int NumOfApi = sizeof( RcApiTable ) / sizeof( apidef_t );
#endif


#endif	/* API_TABLE_H */
