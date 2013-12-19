/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* apiTable.h - header file for The global API table
 */



#ifndef API_TABLE_HPP
#define API_TABLE_HPP

#include "rods.hpp"
#include "apiHandler.hpp"
#include "apiNumber.hpp"
#include "rodsUser.hpp"

/* need to include a header for for each API */
#include "apiHeaderAll.hpp"

#if defined(RODS_SERVER)
apidef_t RsApiTable[] = {
#else	/* client */
apidef_t RcApiTable[] = {
#endif
    {
        GET_MISC_SVR_INFO_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH, NULL,
        0, "MiscSvrInfo_PI", 0, ( funcPtr ) RS_GET_MISC_SVR_INFO
    },
    {
        FILE_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_CREATE
    },
    {
        FILE_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_OPEN
    },
    {
        CHK_N_V_PATH_PERM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0, ( funcPtr ) RS_CHK_NV_PATH_PERM
    },
    {
        FILE_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileWriteInp_PI", 1, NULL, 0, ( funcPtr ) RS_FILE_WRITE
    },
    {
        FILE_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileCloseInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_CLOSE
    },
    {
        FILE_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileLseekInp_PI", 0, "fileLseekOut_PI", 0, ( funcPtr ) RS_FILE_LSEEK
    },
    {
        FILE_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileReadInp_PI", 0, NULL, 1, ( funcPtr ) RS_FILE_READ
    },
    {
        FILE_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileUnlinkInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_UNLINK
    },
    {
        FILE_MKDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileMkdirInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_MKDIR
    },
    {
        FILE_CHMOD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileChmodInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_CHMOD
    },
    {
        FILE_RMDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileRmdirInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_RMDIR
    },
    {
        FILE_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStatInp_PI", 0,  "RODS_STAT_T_PI", 0, ( funcPtr ) RS_FILE_STAT
    },
    {
        FILE_GET_FS_FREE_SPACE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileGetFsFreeSpaceInp_PI", 0, "fileGetFsFreeSpaceOut_PI", 0, ( funcPtr ) RS_FILE_GET_FS_FREE_SPACE
    },
    {
        FILE_OPENDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpendirInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_OPENDIR
    },
    {
        FILE_CLOSEDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileClosedirInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_CLOSEDIR
    },
    {
        FILE_READDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileReaddirInp_PI", 0, "RODS_DIRENT_T_PI", 0, ( funcPtr ) RS_FILE_READDIR
    },
    {
        FILE_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileRenameInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_RENAME
    },
    {
        FILE_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_TRUNCATE
    },
    {
        FILE_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 1, NULL, 0, ( funcPtr ) RS_FILE_PUT
    },
    {
        FILE_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileOpenInp_PI", 0, NULL, 1, ( funcPtr ) RS_FILE_GET
    },
    {
        FILE_STAGE_TO_CACHE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStageSyncInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_STAGE_TO_CACHE
    },
    {
        FILE_SYNC_TO_ARCH_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "fileStageSyncInp_PI", 0, NULL, 0, ( funcPtr ) RS_FILE_SYNC_TO_ARCH
    },
    {
        DATA_OBJ_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_CREATE
    },
    {
        DATA_OBJ_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_OPEN
    },
    {
        DATA_OBJ_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, NULL, 1, ( funcPtr ) RS_DATA_OBJ_READ
    },
    {
        DATA_OBJ_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 1, NULL, 0, ( funcPtr ) RS_DATA_OBJ_WRITE
    },
    {
        DATA_OBJ_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_CLOSE
    },
    {
        DATA_OBJ_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 1,  "PortalOprOut_PI", 0, ( funcPtr ) RS_DATA_OBJ_PUT
    },
    {
        DATA_OBJ_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "PortalOprOut_PI", 1, ( funcPtr ) RS_DATA_OBJ_GET
    },
    {
        DATA_OBJ_REPL250_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "TransStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_REPL250
    },
    {
        DATA_OBJ_REPL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "TransferStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_REPL
    },
    {
        DATA_OBJ_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OpenedDataObjInp_PI", 0, "fileLseekOut_PI", 0,
        ( funcPtr ) RS_DATA_OBJ_LSEEK
    },
    {
        DATA_OBJ_COPY250_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjCopyInp_PI", 0, "TransStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_COPY250
    },
    {
        DATA_OBJ_COPY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjCopyInp_PI", 0, "TransferStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_COPY
    },
    {
        DATA_OBJ_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_UNLINK
    },
    {
        DATA_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataOprInp_PI", 0, "PortalOprOut_PI", 0, ( funcPtr ) RS_DATA_PUT
    },
    {
        DATA_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataOprInp_PI", 0, "PortalOprOut_PI", 0, ( funcPtr ) RS_DATA_GET
    },
    {
        DATA_COPY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataCopyInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_COPY
    },
    {
        SIMPLE_QUERY_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "simpleQueryInp_PI", 0,  "simpleQueryOut_PI", 0, ( funcPtr ) RS_SIMPLE_QUERY
    },
    {
        GENERAL_ADMIN_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalAdminInp_PI", 0, NULL, 0, ( funcPtr ) RS_GENERAL_ADMIN
    },
    {
        GEN_QUERY_AN, RODS_API_VERSION,
#ifdef STORAGE_ADMIN_ROLE
        REMOTE_USER_AUTH | STORAGE_ADMIN_USER, REMOTE_USER_AUTH | STORAGE_ADMIN_USER,
#else
        REMOTE_USER_AUTH, REMOTE_USER_AUTH,
#endif
        "GenQueryInp_PI", 0, "GenQueryOut_PI", 0, ( funcPtr ) RS_GEN_QUERY
    },
    {
        AUTH_REQUEST_AN, RODS_API_VERSION, NO_USER_AUTH | XMSG_SVR_ALSO,
        NO_USER_AUTH | XMSG_SVR_ALSO,
        NULL, 0,  "authRequestOut_PI", 0, ( funcPtr ) RS_AUTH_REQUEST
    },
    {
        AUTH_RESPONSE_AN, RODS_API_VERSION, NO_USER_AUTH | XMSG_SVR_ALSO,
        NO_USER_AUTH | XMSG_SVR_ALSO,
        "authResponseInp_PI", 0, NULL, 0, ( funcPtr ) RS_AUTH_RESPONSE
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
        "authCheckInp_PI", 0,  "authCheckOut_PI", 0, ( funcPtr ) RS_AUTH_CHECK
    },
    {
        GSI_AUTH_REQUEST_AN, RODS_API_VERSION, NO_USER_AUTH | XMSG_SVR_ALSO,
        NO_USER_AUTH | XMSG_SVR_ALSO,
        NULL, 0,  "gsiAuthRequestOut_PI", 0, ( funcPtr ) RS_GSI_AUTH_REQUEST
    },
    {
        KRB_AUTH_REQUEST_AN, RODS_API_VERSION,
        NO_USER_AUTH | XMSG_SVR_ALSO, NO_USER_AUTH | XMSG_SVR_ALSO,
        NULL, 0, "krbAuthRequestOut_PI", 0, ( funcPtr ) RS_KRB_AUTH_REQUEST
    },
    {
        END_TRANSACTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "endTransactionInp_PI", 0, NULL, 0, ( funcPtr ) RS_END_TRANSACTION
    },
    {
        SPECIFIC_QUERY_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "specificQueryInp_PI", 0, "GenQueryOut_PI", 0, ( funcPtr ) RS_SPECIFIC_QUERY
    },
    {
        TICKET_ADMIN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ticketAdminInp_PI", 0, NULL, 0, ( funcPtr ) RS_TICKET_ADMIN
    },
    {
        GET_TEMP_PASSWORD_FOR_OTHER_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "getTempPasswordForOtherInp_PI", 0,  "getTempPasswordForOtherOut_PI", 0, ( funcPtr ) RS_GET_TEMP_PASSWORD_FOR_OTHER
    },
    {
        PAM_AUTH_REQUEST_AN, RODS_API_VERSION,
        NO_USER_AUTH | XMSG_SVR_ALSO, NO_USER_AUTH | XMSG_SVR_ALSO,
        "pamAuthRequestInp_PI", 0,  "pamAuthRequestOut_PI", 0, ( funcPtr ) RS_PAM_AUTH_REQUEST
    },
    {
        GET_LIMITED_PASSWORD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "getLimitedPasswordInp_PI", 0,  "getLimitedPasswordOut_PI",
        0, ( funcPtr ) RS_GET_LIMITED_PASSWORD
    },
    {
        OPEN_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_OPEN_COLLECTION
    },
    {
        READ_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, "CollEnt_PI", 0, ( funcPtr ) RS_READ_COLLECTION
    },
    {
        USER_ADMIN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "userAdminInp_PI", 0, NULL, 0, ( funcPtr ) RS_USER_ADMIN
    },
    {
        GENERAL_ROW_INSERT_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalRowInsertInp_PI", 0, NULL, 0, ( funcPtr ) RS_GENERAL_ROW_INSERT
    },
    {
        GENERAL_ROW_PURGE_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "generalRowPurgeInp_PI", 0, NULL, 0, ( funcPtr ) RS_GENERAL_ROW_PURGE
    },
    {
        CLOSE_COLLECTION_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 0, ( funcPtr ) RS_CLOSE_COLLECTION
    },
    {
        COLL_REPL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, "CollOprStat_PI", 0, ( funcPtr ) RS_COLL_REPL
    },
    {
        RM_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, "CollOprStat_PI", 0, ( funcPtr ) RS_RM_COLL
    },
    {
        MOD_AVU_METADATA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ModAVUMetadataInp_PI", 0, NULL, 0, ( funcPtr ) RS_MOD_AVU_METADATA
    },
    {
        MOD_ACCESS_CONTROL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "modAccessControlInp_PI", 0, NULL, 0, ( funcPtr ) RS_MOD_ACCESS_CONTROL
    },
    {
        RULE_EXEC_MOD_AN, RODS_API_VERSION, LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "RULE_EXEC_MOD_INP_PI", 0, NULL, 0, ( funcPtr ) RS_RULE_EXEC_MOD
    },
    {
        GET_TEMP_PASSWORD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        NULL, 0,  "getTempPasswordOut_PI", 0, ( funcPtr ) RS_GET_TEMP_PASSWORD
    },
    {
        GENERAL_UPDATE_AN, RODS_API_VERSION,
        LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "GeneralUpdateInp_PI", 0, NULL, 0, ( funcPtr ) RS_GENERAL_UPDATE
    },
    {
        MOD_DATA_OBJ_META_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ModDataObjMeta_PI", 0, NULL, 0, ( funcPtr ) RS_MOD_DATA_OBJ_META
    },
    {
        RULE_EXEC_SUBMIT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "RULE_EXEC_SUBMIT_INP_PI", 0, "IRODS_STR_PI", 0, ( funcPtr ) RS_RULE_EXEC_SUBMIT
    },
    {
        RULE_EXEC_DEL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, LOCAL_USER_AUTH,
        "RULE_EXEC_DEL_INP_PI", 0, NULL, 0, ( funcPtr ) RS_RULE_EXEC_DEL
    },
    {
        EXEC_MY_RULE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ExecMyRuleInp_PI", 0, "MsParamArray_PI", 0, ( funcPtr ) RS_EXEC_MY_RULE
    },
    {
        OPR_COMPLETE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 0, ( funcPtr ) RS_OPR_COMPLETE
    },
    {
        DATA_OBJ_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjCopyInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_RENAME
    },
    {
        DATA_OBJ_RSYNC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "MsParamArray_PI", 0, ( funcPtr ) RS_DATA_OBJ_RSYNC
    },
    {
        DATA_OBJ_CHKSUM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0, ( funcPtr ) RS_DATA_OBJ_CHKSUM
    },
    {
        PHY_PATH_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_PHY_PATH_REG
    },
    {
        DATA_OBJ_PHYMV250_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "TransStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_PHYMV250
    },
    {
        DATA_OBJ_PHYMV_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "TransferStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_PHYMV
    },
    {
        DATA_OBJ_TRIM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_TRIM
    },
    {
        OBJ_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "RodsObjStat_PI", 0, ( funcPtr ) RS_OBJ_STAT
    },
    {
        EXEC_CMD241_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ExecCmd241_PI", 0, "ExecCmdOut_PI", 0, ( funcPtr ) RS_EXEC_CMD241
    },
    {
        EXEC_CMD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ExecCmd_PI", 0, "ExecCmdOut_PI", 0, ( funcPtr ) RS_EXEC_CMD
    },
    {
        STREAM_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileCloseInp_PI", 0, NULL, 0, ( funcPtr ) RS_STREAM_CLOSE
    },
    {
        GET_HOST_FOR_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0, ( funcPtr ) RS_GET_HOST_FOR_GET
    },
    {
        DATA_OBJ_LOCK_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_LOCK
    },
    {
        SUB_STRUCT_FILE_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_CREATE
    },
    {
        SUB_STRUCT_FILE_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_OPEN
    },
    {
        SUB_STRUCT_FILE_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 0, NULL, 1,
        ( funcPtr ) RS_SUB_STRUCT_FILE_READ
    },
    {
        SUB_STRUCT_FILE_WRITE_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 1, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_WRITE
    },
    {
        SUB_STRUCT_FILE_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_CLOSE
    },
    {
        SUB_STRUCT_FILE_UNLINK_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_UNLINK
    },
    {
        SUB_STRUCT_FILE_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, "RODS_STAT_T_PI", 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_STAT
    },
    {
        SUB_STRUCT_FILE_LSEEK_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileLseekInp_PI", 0, "fileLseekOut_PI", 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_LSEEK
    },
    {
        SUB_STRUCT_FILE_RENAME_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileRenameInp_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_RENAME
    },
    {
        QUERY_SPEC_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "GenQueryOut_PI", 0,
        ( funcPtr ) RS_QUERY_SPEC_COLL
    },
    {
        MOD_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_MOD_COLL
    },
    {
        SUB_STRUCT_FILE_MKDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_MKDIR
    },
    {
        SUB_STRUCT_FILE_RMDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_RMDIR
    },
    {
        SUB_STRUCT_FILE_OPENDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_OPENDIR
    },
    {
        SUB_STRUCT_FILE_READDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 0, "RODS_DIRENT_T_PI", 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_READDIR
    },
    {
        SUB_STRUCT_FILE_CLOSEDIR_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubStructFileFdOpr_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_CLOSEDIR
    },
    {
        DATA_OBJ_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_DATA_OBJ_TRUNCATE
    },
    {
        SUB_STRUCT_FILE_TRUNCATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH,
        REMOTE_PRIV_USER_AUTH, "SubFile_PI", 0, NULL, 0,
        ( funcPtr ) RS_SUB_STRUCT_FILE_TRUNCATE
    },
    {
        GET_XMSG_TICKET_AN, RODS_API_VERSION, REMOTE_USER_AUTH | XMSG_SVR_ONLY,
        REMOTE_USER_AUTH | XMSG_SVR_ONLY,
        "GetXmsgTicketInp_PI", 0, "XmsgTicketInfo_PI", 0,
        ( funcPtr ) RS_GET_XMSG_TICKET
    },
    {
        SEND_XMSG_AN, RODS_API_VERSION, XMSG_SVR_ONLY, XMSG_SVR_ONLY,
        "SendXmsgInp_PI", 0, NULL, 0, ( funcPtr ) RS_SEND_XMSG
    },
    {
        RCV_XMSG_AN, RODS_API_VERSION, XMSG_SVR_ONLY, XMSG_SVR_ONLY,
        "RcvXmsgInp_PI", 0, "RcvXmsgOut_PI", 0, ( funcPtr ) RS_RCV_XMSG
    },
    {
        SUB_STRUCT_FILE_GET_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "SubFile_PI", 0, NULL, 1, ( funcPtr ) RS_SUB_STRUCT_FILE_GET
    },
    {
        SUB_STRUCT_FILE_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "SubFile_PI", 1, NULL, 0, ( funcPtr ) RS_SUB_STRUCT_FILE_PUT
    },
    {
        SYNC_MOUNTED_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_SYNC_MOUNTED_COLL
    },
    {
        STRUCT_FILE_SYNC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileOprInp_PI", 0, NULL, 0, ( funcPtr ) RS_STRUCT_FILE_SYNC
    },
    {
        COLL_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_COLL_CREATE
    },
    {
        RM_COLL_OLD_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_RM_COLL_OLD
    },
    {
        STRUCT_FILE_EXTRACT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileOprInp_PI", 0, NULL, 0, ( funcPtr ) RS_STRUCT_FILE_EXTRACT
    },
    {
        STRUCT_FILE_EXT_AND_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileExtAndRegInp_PI", 0, NULL, 0, ( funcPtr ) RS_STRUCT_FILE_EXT_AND_REG
    },
    {
        STRUCT_FILE_BUNDLE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "StructFileExtAndRegInp_PI", 0, NULL, 0, ( funcPtr ) RS_STRUCT_FILE_BUNDLE
    },
    {
        CHK_OBJ_PERM_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ChkObjPermAndStat_PI", 0, NULL, 0, ( funcPtr ) RS_CHK_OBJ_PERM_AND_STAT
    },
    {
        GET_REMOTE_ZONE_RESC_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "RHostAddr_PI", 0, ( funcPtr ) RS_GET_REMOTE_ZONE_RESC
    },
    {
        DATA_OBJ_OPEN_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "OpenStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_OPEN_AND_STAT
    },
    {
        L3_FILE_GET_SINGLE_BUF_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 0, NULL, 1, ( funcPtr ) RS_L3_FILE_GET_SINGLE_BUF
    },
    {
        L3_FILE_PUT_SINGLE_BUF_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "INT_PI", 1, NULL, 0, ( funcPtr ) RS_L3_FILE_PUT_SINGLE_BUF
    },
    {
        DATA_OBJ_CREATE_AND_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "OpenStat_PI", 0, ( funcPtr ) RS_DATA_OBJ_CREATE_AND_STAT
    },
    {
        PHY_BUNDLE_COLL_AN, RODS_API_VERSION, LOCAL_PRIV_USER_AUTH, LOCAL_PRIV_USER_AUTH,
        "StructFileExtAndRegInp_PI", 0, NULL, 0, ( funcPtr ) RS_PHY_BUNDLE_COLL
    },
    {
        UNBUN_AND_REG_PHY_BUNFILE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "DataObjInp_PI", 0, NULL, 0, ( funcPtr ) RS_UNBUN_AND_REG_PHY_BUNFILE
    },
    {
        GET_HOST_FOR_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInp_PI", 0, "STR_PI", 0, ( funcPtr ) RS_GET_HOST_FOR_PUT
    },
    {
        GET_RESC_QUOTA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "getRescQuotaInp_PI", 0, "rescQuota_PI", 0, ( funcPtr ) RS_GET_RESC_QUOTA
    },
    {
        BULK_DATA_OBJ_REG_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "GenQueryOut_PI", 0, "GenQueryOut_PI", 0, ( funcPtr ) RS_BULK_DATA_OBJ_REG
    },
    {
        BULK_DATA_OBJ_PUT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "BulkOprInp_PI", 1, NULL, 0, ( funcPtr ) RS_BULK_DATA_OBJ_PUT
    },
    {
        PROC_STAT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "ProcStatInp_PI", 0, "GenQueryOut_PI", 0, ( funcPtr ) RS_PROC_STAT
    },
    {
        STREAM_READ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileReadInp_PI", 0, NULL, 1, ( funcPtr ) RS_STREAM_READ
    },
    {
        REG_COLL_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "CollInpNew_PI", 0, NULL, 0, ( funcPtr ) RS_REG_COLL
    },
    {
        REG_DATA_OBJ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "DataObjInfo_PI", 0, "DataObjInfo_PI", 0, ( funcPtr ) RS_REG_DATA_OBJ
    },
    {
        UNREG_DATA_OBJ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "UnregDataObj_PI", 0, NULL, 0, ( funcPtr ) RS_UNREG_DATA_OBJ
    },
    {
        REG_REPLICA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_PRIV_USER_AUTH,
        "RegReplica_PI", 0, NULL, 0, ( funcPtr ) RS_REG_REPLICA
    },
    {
        FILE_CHKSUM_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "fileChksumInp_PI", 0, "fileChksumOut_PI", 0, ( funcPtr ) RS_FILE_CHKSUM
    },
#ifdef NETCDF_CLIENT
    {
        NC_OPEN_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcOpenInp_PI", 0, "INT_PI", 0, ( funcPtr ) RS_NC_OPEN
    },
    {
        NC_CREATE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcOpenInp_PI", 0, "INT_PI", 0, ( funcPtr ) RS_NC_CREATE
    },
    {
        NC_CLOSE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcCloseInp_PI", 0, NULL, 0, ( funcPtr ) RS_NC_CLOSE
    },
    {
        NC_INQ_ID_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcInqIdInp_PI", 0, "INT_PI", 0, ( funcPtr ) RS_NC_INQ_ID
    },
    {
        NC_INQ_WITH_ID_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcInqIdInp_PI", 0, "NcInqWithIdOut_PI", 0, ( funcPtr ) RS_NC_INQ_WITH_ID
    },
    {
        NC_GET_VARS_BY_TYPE_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcGetVarInp_PI", 0, "NcGetVarOut_PI", 0, ( funcPtr ) RS_NC_GET_VARS_BY_TYPE
    },
#ifdef LIB_CF
    {
        NCCF_GET_VARA_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NccfGetVarInp_PI", 0, "NccfGetVarOut_PI", 0, ( funcPtr ) RS_NCCF_GET_VARA
    },
#endif
    {
        NC_INQ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcInqInp_PI", 0, "NcInqOut_PI", 0, ( funcPtr ) RS_NC_INQ
    },
#ifdef NETCDF4_API
    {
        NC_OPEN_GROUP_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcOpenInp_PI", 0, "INT_PI", 0, ( funcPtr ) RS_NC_OPEN_GROUP
    },
    {
        NC_INQ_GRPS_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcInqGrpsInp_PI", 0, "NcInqGrpsOut_PI", 0, ( funcPtr ) RS_NC_INQ_GRPS
    },
#endif
    {
        NC_REG_GLOBAL_ATTR_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcRegGlobalAttrInp_PI", 0, NULL, 0, ( funcPtr ) RS_NC_REG_GLOBAL_ATTR
    },
    {
        NC_GET_AGG_ELEMENT_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcOpenInp_PI", 0, "NcAggElement_PI", 0, ( funcPtr ) RS_NC_GET_AGG_ELEMENT
    },
    {
        NC_GET_AGG_INFO_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcOpenInp_PI", 0, "NcAggInfo_PI", 0, ( funcPtr ) RS_NC_GET_AGG_INFO
    },
    {
        NC_ARCH_TIME_SERIES_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "NcArchTimeSeriesInp_PI", 0, NULL, 0, ( funcPtr ) RS_NC_ARCH_TIME_SERIES
    },
#endif
#ifdef OOI_CI
    {
        OOI_GEN_SERV_REQ_AN, RODS_API_VERSION, REMOTE_USER_AUTH, REMOTE_USER_AUTH,
        "OoiGenServReqInp_PI", 0, "OoiGenServReqOut_PI", 0, ( funcPtr ) RS_OOI_GEN_SERV_REQ
    },
#endif
    {
        SSL_START_AN, RODS_API_VERSION,
        NO_USER_AUTH | XMSG_SVR_ALSO, NO_USER_AUTH | XMSG_SVR_ALSO,
        "sslStartInp_PI", 0, NULL, 0, ( funcPtr ) RS_SSL_START
    },
    {
        SSL_END_AN, RODS_API_VERSION,
        NO_USER_AUTH | XMSG_SVR_ALSO, NO_USER_AUTH | XMSG_SVR_ALSO,
        "sslEndInp_PI", 0, NULL, 0, ( funcPtr ) RS_SSL_END
    },
    {
        AUTH_PLUG_REQ_AN, RODS_API_VERSION, NO_USER_AUTH, NO_USER_AUTH,
        "authPlugReqInp_PI", 0, "authPlugReqOut_PI", 0, ( funcPtr ) RS_AUTH_PLUG_REQ
    },
};

#ifdef RODS_SERVER	/* depends on client lib for NumOfApi */
#else
int NumOfApi = sizeof( RcApiTable ) / sizeof( apidef_t );
#endif

#endif	/* API_TABLE_H */
