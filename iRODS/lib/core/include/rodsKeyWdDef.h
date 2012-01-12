/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rodsKeyWdDef.h - global definition for client API */

#ifndef RODS_KEYWD_DEF
#define RODS_KEYWD_DEF

/* The following are the keyWord definition for the condInput key/value pair */

#define ALL_KW 		"all" 		/* operation done on all replica */
#define COPIES_KW	"copies" 	/* the number of copies */
#define EXEC_LOCALLY_KW	"execLocally" 	/* execute locally */
#define FORCE_FLAG_KW 	"forceFlag" 	/* force update */
#define CLI_IN_SVR_FIREWALL_KW "cliInSvrFirewall" /* cli behind same firewall */
#define REG_CHKSUM_KW	"regChksum" 	/* register checksum */
#define VERIFY_CHKSUM_KW "verifyChksum"	/* verify checksum */
#define VERIFY_BY_SIZE_KW "verifyBySize" /* verify by size - used by irsync */
#define OBJ_PATH_KW	"objPath"	/* logical path of the object */ 
#define RESC_NAME_KW	"rescName"	/* resource name */
#define DEST_RESC_NAME_KW	"destRescName"	/* destination resource name */
#define DEF_RESC_NAME_KW	"defRescName"	/* default resource name */
#define BACKUP_RESC_NAME_KW "backupRescName" /* destination resource name */
#define DATA_TYPE_KW	"dataType"	/* data type */
#define DATA_SIZE_KW	"dataSize"
#define CHKSUM_KW	"chksum"
#define VERSION_KW	"version"
#define FILE_PATH_KW	"filePath"	/* the physical file path */
#define REPL_NUM_KW	"replNum"	/* replica number */
#define WRITE_FLAG_KW	"writeFlag"	/* whether it is opened for write */
#define REPL_STATUS_KW	"replStatus"	/* status of the replica */
#define ALL_REPL_STATUS_KW	"allReplStatus"	/* update all replStatus */
#define DATA_INCLUDED_KW "dataIncluded"	/* data included in the input buffer */
#define DATA_OWNER_KW	"dataOwner"	
#define DATA_OWNER_ZONE_KW	"dataOwnerZone" 
#define DATA_EXPIRY_KW	"dataExpiry"
#define DATA_COMMENTS_KW "dataComments"
#define DATA_CREATE_KW	"dataCreate"
#define DATA_MODIFY_KW	"dataModify"
#define DATA_ACCESS_KW	"dataAccess"
#define DATA_ACCESS_INX_KW	"dataAccessInx"
#define NO_OPEN_FLAG_KW	"noOpenFlag"
#define PHYOPEN_BY_SIZE_KW "phyOpenBySize"
#define STREAMING_KW	"streaming"
#define DATA_ID_KW     "dataId"
#define COLL_ID_KW     "collId"
#define RESC_GROUP_NAME_KW     "rescGroupName"
#define DATA_MODE_KW   "dataMode"
#define STATUS_STRING_KW     "statusString"
#define DATA_MAP_ID_KW    "dataMapId"
#define NO_PARA_OP_KW    "noParaOpr"
#define LOCAL_PATH_KW    "localPath"
#define RSYNC_MODE_KW    "rsyncMode"
#define RSYNC_DEST_PATH_KW    "rsyncDestPath"
#define RSYNC_CHKSUM_KW    "rsyncChksum"
#define CHKSUM_ALL_KW    "ChksumAll"
#define FORCE_CHKSUM_KW    "forceChksum"
#define COLLECTION_KW    "collection"
#define IRODS_ADMIN_KW    "irodsAdmin"
#define IRODS_ADMIN_RMTRASH_KW "irodsAdminRmTrash"
#define IRODS_RMTRASH_KW "irodsRmTrash"
#define RECURSIVE_OPR__KW    "recursiveOpr"
#define COLLECTION_TYPE_KW    "collectionType"
#define COLLECTION_INFO1_KW    "collectionInfo1"
#define COLLECTION_INFO2_KW    "collectionInfo2"
#define SEL_OBJ_TYPE_KW    "selObjType"
#define STRUCT_FILE_OPR_KW    	"structFileOpr"
#define ALL_MS_PARAM_KW    	"allMsParam"
#define UNREG_COLL_KW    	"unregColl"
#define UPDATE_REPL_KW    	"updateRepl"
#define RBUDP_TRANSFER_KW    	"rbudpTransfer"
#define VERY_VERBOSE_KW    	"veryVerbose"
#define RBUDP_SEND_RATE_KW    	"rbudpSendRate"
#define RBUDP_PACK_SIZE_KW    	"rbudpPackSize"
#define ZONE_KW    		"zone"
#define REMOTE_ZONE_OPR_KW    	"remoteZoneOpr"
#define REPL_DATA_OBJ_INP_KW   	"replDataObjInp"
#define CROSS_ZONE_CREATE_KW   	"replDataObjInp"  /* use the same for backward
						   * compatibility */
#define QUERY_BY_DATA_ID_KW   	"queryByDataID"
#define SU_CLIENT_USER_KW   	"suClientUser"
#define RM_BUN_COPY_KW   	"rmBunCopy"
#define KEY_WORD_KW   		"keyWord"   /* the msKeyValStr is a keyword */ 
#define CREATE_MODE_KW   	"createMode" /* a msKeyValStr keyword */ 
#define OPEN_FLAGS_KW   	"openFlags" /* a msKeyValStr keyword */ 
#define OFFSET_KW   		"offset" /* a msKeyValStr keyword */ 
/* DATA_SIZE_KW already defined */ 
#define NUM_THREADS_KW   	"numThreads" /* a msKeyValStr keyword */ 
#define OPR_TYPE_KW   		"oprType" /* a msKeyValStr keyword */ 
#define COLL_FLAGS_KW  		"collFlags" /* a msKeyValStr keyword */ 
#define TRANSLATED_PATH_KW	"translatedPath"  /* the path translated */
#define NO_TRANSLATE_LINKPT_KW	"noTranslateMntpt"  /* don't translate mntpt */
#define BULK_OPR_KW		"bulkOpr"  /* the bulk operation */
#define NON_BULK_OPR_KW		"nonBulkOpr"  /* non bulk operation */
#define EXEC_CMD_RULE_KW	"execCmdRule" /* the rule that invoke execCmd */
#define EXEC_MY_RULE_KW	"execMyRule" /* the rule is invoked by rsExecMyRule */
#define STREAM_STDOUT_KW	"streamStdout"   /* the stream stdout for 
						  * execCmd */ 
#define REG_REPL_KW		"regRepl"  /* register replica */
#define AGE_KW			"age"  /* age of the file for itrim */
#define DRYRUN_KW		"dryrun"  /* do a dry run */
#define ACL_COLLECTION_KW	"aclCollection"  /* the collection from which 
						  * the ACL should be used */
#define NO_CHK_COPY_LEN_KW	"noChkCopyLen"  /* Don't check the len 
						  * when transfering  */
/* OBJ_PATH_KW already defined */ 
/* COLL_NAME_KW already defined */ 

/* The following are the keyWord definition for the rescCond key/value pair */
/* RESC_NAME_KW is defined above */

#define RESC_ZONE_KW      "zoneName"
#define RESC_LOC_KW 	"rescLoc"   /* resc_net in DB */
#define RESC_TYPE_KW 	"rescType"
#define RESC_CLASS_KW	"rescClass"
#define RESC_VAULT_PATH_KW	"rescVaultPath" /* resc_def_path in DB */
#define RESC_STATUS_KW	"rescStatus"
#define PARA_OPR_KW	"paraOpr"
#define GATEWAY_ADDR_KW	"gateWayAddr"   
#define RESC_MAX_OBJ_SIZE_KW "rescMaxObjSize"
#define FREE_SPACE_KW	"freeSpace"
#define FREE_SPACE_TIME_KW	"freeSpaceTime"
#define FREE_SPACE_TIMESTAMP_KW	"freeSpaceTimeStamp"
#define RESC_TYPE_INX_KW 	"rescTypeInx"
#define RESC_CLASS_INX_KW	"rescClassInx"
#define RESC_ID_KW	"rescId"
#define RESC_INFO_KW     "rescInfo"
#define RESC_COMMENTS_KW     "rescComments"
#define RESC_CREATE_KW     "rescCreate"
#define RESC_MODIFY_KW     "rescModify"

/* The following are the keyWord definition for the userCond key/value pair */

#define USER_NAME_CLIENT_KW     "userNameClient"
#define RODS_ZONE_CLIENT_KW     "rodsZoneClient"
#define HOST_CLIENT_KW           "hostClient"
#define CLIENT_ADDR_KW           "clientAddr"
#define USER_TYPE_CLIENT_KW     "userTypeClient"
#define AUTH_STR_CLIENT_KW      "authStrClient" /* user distin name */
#define USER_AUTH_SCHEME_CLIENT_KW     "userAuthSchemeClient"
#define USER_INFO_CLIENT_KW            "userInfoClient"
#define USER_COMMENT_CLIENT_KW         "userCommentClient"
#define USER_CREATE_CLIENT_KW          "userCreateClient"
#define USER_MODIFY_CLIENT_KW          "userModifyClient"
#define USER_NAME_PROXY_KW      "userNameProxy"
#define RODS_ZONE_PROXY_KW      "rodsZoneProxy"
#define HOST_PROXY_KW           "hostProxy"
#define USER_TYPE_PROXY_KW      "userTypeProxy"
#define AUTH_STR_PROXY_KW       "authStrProxy" /* dn */
#define USER_AUTH_SCHEME_PROXY_KW    "userAuthSchemeProxy"
#define USER_INFO_PROXY_KW           "userInfoProxy"
#define USER_COMMENT_PROXY_KW        "userCommentProxy"
#define USER_CREATE_PROXY_KW         "userCreateProxy"
#define USER_MODIFY_PROXY_KW         "userModifyProxy"
#define ACCESS_PERMISSION_KW         "accessPermission"
#define NO_CHK_FILE_PERM_KW          "noChkFilePerm"

/* The following are the keyWord definition for the collCond key/value pair */

#define COLL_NAME_KW              "collName"
#define COLL_PARENT_NAME_KW         "collParentName" /* parent_coll_name in DB  */
#define COLL_OWNER_NAME_KW              "collOwnername"
#define COLL_OWNER_ZONE_KW         "collOwnerZone" 
#define COLL_MAP_ID_KW             "collMapId"
#define COLL_INHERITANCE_KW         "collInheritance"
#define COLL_COMMENTS_KW           "collComments"
#define COLL_EXPIRY_KW           "collExpiry"
#define COLL_CREATE_KW           "collCreate"
#define COLL_MODIFY_KW             "collModify"
#define COLL_ACCESS_KW             "collAccess"
#define COLL_ACCESS_INX_KW         "collAccessInx"
#define COLL_ID_KW               "collId"

/* 
  The following are the keyWord definitions for the keyValPair_t input
  to chlModRuleExec.
*/
#define RULE_NAME_KW                 "ruleName"
#define RULE_REI_FILE_PATH_KW        "reiFilePath"
#define RULE_USER_NAME_KW            "userName"
#define RULE_EXE_ADDRESS_KW          "exeAddress"
#define RULE_EXE_TIME_KW             "exeTime"
#define RULE_EXE_FREQUENCY_KW        "exeFrequency"
#define RULE_PRIORITY_KW             "priority"
#define RULE_ESTIMATE_EXE_TIME_KW    "estimateExeTime"
#define RULE_NOTIFICATION_ADDR_KW    "notificationAddr"
#define RULE_LAST_EXE_TIME_KW        "lastExeTime"
#define RULE_EXE_STATUS_KW           "exeStatus"

#endif	/* RODS_KEYWD_DEF */
