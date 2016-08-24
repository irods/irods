/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* global definition for client API */

#ifndef RC_GLOBAL_H__
#define RC_GLOBAL_H__

#include "rods.h"
#include "msParam.h"
#include "rodsPackTable.h"		/* globally declare RodsPackTable */
#include "objInfo.h"
#include "rodsGenQuery.h"
#include "rodsGeneralUpdate.h"
#include "guiProgressCallback.h"

int ProcessType = CLIENT_PT;

char *dataObjCond[] = {
    ALL_KW, 		/* operation done on all replica */
    COPIES_KW, 		/* the number of copies */
    EXEC_LOCALLY_KW, 	/* execute locally */
    FORCE_FLAG_KW, 		/* force update */
    CLI_IN_SVR_FIREWALL_KW, /* client behind same firewall */
    REG_CHKSUM_KW, 		/* register checksum */
    VERIFY_CHKSUM_KW, 	/* verify checksum */
    OBJ_PATH_KW,		/* logical path of the object */
    RESC_NAME_KW,		/* resource name */
    DEST_RESC_NAME_KW,	/* destination resource name */
    BACKUP_RESC_NAME_KW,	/* destination resource name */
    DATA_TYPE_KW,		/* data type */
    DATA_SIZE_KW,
    CHKSUM_KW,
    VERSION_KW,
    FILE_PATH_KW,		/* the physical file path */
    REPL_NUM_KW,		/* replica number */
    REPL_STATUS_KW, 	/* status of the replica */
    DATA_INCLUDED_KW,	/* data included in the input bytes buffer */
    DATA_OWNER_KW,
    DATA_OWNER_ZONE_KW,
    DATA_EXPIRY_KW,
    DATA_COMMENTS_KW,
    DATA_CREATE_KW,
    DATA_MODIFY_KW,
    DATA_ACCESS_KW,
    DATA_ACCESS_INX_KW,
    NO_OPEN_FLAG_KW,
    STREAMING_KW,
    DATA_ID_KW,
    COLL_ID_KW,
    STATUS_STRING_KW,
    DATA_MAP_ID_KW,
    "ENDOFLIST"
};

char *compareOperator[]  = {
    ">", "<", "=",
    "like", "between", "LIKE", "BETWEEN",
    "NOT LIKE", "NOT BETWEEN", "NOT LIKE", "NOT BETWEEN",
    "ENDOFLIST"
};

char *rescCond[] = {
    RESC_ZONE_KW,
    RESC_NAME_KW,
    RESC_LOC_KW,
    RESC_TYPE_KW,
    RESC_CLASS_KW,
    RESC_VAULT_PATH_KW,
    RESC_STATUS_KW,
    GATEWAY_ADDR_KW,
    RESC_MAX_OBJ_SIZE_KW,
    FREE_SPACE_KW,
    FREE_SPACE_TIME_KW,
    FREE_SPACE_TIMESTAMP_KW,
    RESC_TYPE_INX_KW,
    RESC_CLASS_INX_KW,
    RESC_ID_KW,
    RESC_COMMENTS_KW,
    RESC_CREATE_KW,
    RESC_MODIFY_KW,
    "ENDOFLIST"
};

char *userCond[] = {
    USER_NAME_CLIENT_KW,
    RODS_ZONE_CLIENT_KW,
    HOST_CLIENT_KW,
    USER_TYPE_CLIENT_KW,
    AUTH_STR_CLIENT_KW,
    USER_AUTH_SCHEME_CLIENT_KW,
    USER_INFO_CLIENT_KW,
    USER_COMMENT_CLIENT_KW,
    USER_CREATE_CLIENT_KW,
    USER_MODIFY_CLIENT_KW,
    USER_NAME_PROXY_KW,
    RODS_ZONE_PROXY_KW,
    HOST_PROXY_KW,
    USER_TYPE_PROXY_KW,
    AUTH_STR_PROXY_KW,
    USER_AUTH_SCHEME_PROXY_KW,
    USER_INFO_PROXY_KW,
    USER_COMMENT_PROXY_KW,
    USER_CREATE_PROXY_KW,
    USER_MODIFY_PROXY_KW,
    "ENDOFLIST"
};

char *collCond[] = {
    COLL_NAME_KW,
    COLL_PARENT_NAME_KW,
    COLL_OWNER_NAME_KW,
    COLL_OWNER_ZONE_KW,
    COLL_MAP_ID_KW,
    COLL_INHERITANCE_KW,
    COLL_COMMENTS_KW,
    COLL_EXPIRY_KW,
    COLL_CREATE_KW,
    COLL_MODIFY_KW,
    COLL_ACCESS_KW,
    COLL_ACCESS_INX_KW,
    COLL_ID_KW,
    "ENDOFLIST"
};

/* Note; all structFile name must contain the word structFile */

structFileTypeDef_t StructFileTypeDef[] = {
    {HAAW_STRUCT_FILE_STR, HAAW_STRUCT_FILE_T},
    {TAR_STRUCT_FILE_STR, TAR_STRUCT_FILE_T},
    {MSSO_STRUCT_FILE_STR, MSSO_STRUCT_FILE_T},
};

int NumStructFileType = sizeof( StructFileTypeDef ) / sizeof( structFileTypeDef_t );

/* valid keyWds for dataObjInp_t */
validKeyWd_t DataObjInpKeyWd[] = {
    {RESC_NAME_FLAG,	RESC_NAME_KW},
    {DEST_RESC_NAME_FLAG,	DEST_RESC_NAME_KW},
    {BACKUP_RESC_NAME_FLAG,	BACKUP_RESC_NAME_KW},
    {DEF_RESC_NAME_FLAG,	DEF_RESC_NAME_KW},
    {FORCE_FLAG_FLAG,	FORCE_FLAG_KW},
    {ALL_FLAG,		ALL_KW},
    {LOCAL_PATH_FLAG,	LOCAL_PATH_KW},
    {VERIFY_CHKSUM_FLAG,	VERIFY_CHKSUM_KW},
    {ADMIN_FLAG,	ADMIN_KW},
    {UPDATE_REPL_FLAG,	UPDATE_REPL_KW},
    {REPL_NUM_FLAG,		REPL_NUM_KW},
    {DATA_TYPE_FLAG,	DATA_TYPE_KW},
    {CHKSUM_ALL_FLAG,	CHKSUM_ALL_KW},
    {FORCE_CHKSUM_FLAG,	FORCE_CHKSUM_KW},
    {FILE_PATH_FLAG,	FILE_PATH_KW},
    {CREATE_MODE_FLAG,	CREATE_MODE_KW},
    {OPEN_FLAGS_FLAG,	OPEN_FLAGS_KW},
    {DATA_SIZE_FLAGS,	DATA_SIZE_KW},
    {NUM_THREADS_FLAG,	NUM_THREADS_KW},
    {OPR_TYPE_FLAG,		OPR_TYPE_KW},
    {OBJ_PATH_FLAG,		OBJ_PATH_KW},
    {RMTRASH_FLAG,	RMTRASH_KW},
    {ADMIN_RMTRASH_FLAG,	ADMIN_RMTRASH_KW},
    {UNREG_FLAG,            UNREG_KW},
    {RBUDP_TRANSFER_FLAG,	RBUDP_TRANSFER_KW},
    {RBUDP_SEND_RATE_FLAG,	RBUDP_SEND_RATE_KW},
    {RBUDP_PACK_SIZE_FLAG,	RBUDP_PACK_SIZE_KW},
};

int NumDataObjInpKeyWd = sizeof( DataObjInpKeyWd ) / sizeof( validKeyWd_t );

/* valid keyWds for collInp_t */
validKeyWd_t CollInpKeyWd[] = {
    {RESC_NAME_FLAG,        RESC_NAME_KW},
    {DEST_RESC_NAME_FLAG,   DEST_RESC_NAME_KW},
    {DEF_RESC_NAME_FLAG,    DEF_RESC_NAME_KW},
    {BACKUP_RESC_NAME_FLAG, BACKUP_RESC_NAME_KW},
    {FORCE_FLAG_FLAG,       FORCE_FLAG_KW},
    {ALL_FLAG,		ALL_KW},
    {VERIFY_CHKSUM_FLAG,    VERIFY_CHKSUM_KW},
    {ADMIN_FLAG,      ADMIN_KW},
    {UPDATE_REPL_FLAG,      UPDATE_REPL_KW},
    {REPL_NUM_FLAG,         REPL_NUM_KW},
    {COLL_FLAGS_FLAG,       COLL_FLAGS_KW},
    {OPR_TYPE_FLAG,         OPR_TYPE_KW},
    {COLL_NAME_FLAG,         COLL_NAME_KW},
    {RMTRASH_FLAG,    RMTRASH_KW},
    {ADMIN_RMTRASH_FLAG,      ADMIN_RMTRASH_KW},
};

int NumCollInpKeyWd = sizeof( CollInpKeyWd ) / sizeof( validKeyWd_t );

/* valid keyWds for structFileExtAndRegInp */
validKeyWd_t StructFileExtAndRegInpKeyWd[] = {
    {RESC_NAME_FLAG,        RESC_NAME_KW},
    {DEST_RESC_NAME_FLAG,   DEST_RESC_NAME_KW},
    {DEF_RESC_NAME_FLAG,    DEF_RESC_NAME_KW},
    {COLL_FLAGS_FLAG,       COLL_FLAGS_KW},
    {OPR_TYPE_FLAG,         OPR_TYPE_KW},
    {OBJ_PATH_FLAG,		OBJ_PATH_KW},
    {COLL_NAME_FLAG,         COLL_NAME_KW},
};

int NumStructFileExtAndRegInpKeyWd = sizeof( StructFileExtAndRegInpKeyWd ) / sizeof( validKeyWd_t );

struct timeval SysTimingVal;

#ifdef __cplusplus
extern "C" {
#endif
guiProgressCallback gGuiProgressCB = NULL;

#ifdef __cplusplus
}
#endif
#endif	// RC_GLOBAL_H__
