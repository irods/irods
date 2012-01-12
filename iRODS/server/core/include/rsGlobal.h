/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsGlobal.h - header file for globals in the server
 */

#ifndef RS_GLOBAL_H
#define RS_GLOBAL_H

#include "apiTable.h"	/* contains global RsApiTable */ 
#include "initServer.h"
#include "fileOpr.h"
#include "dataObjOpr.h"
#include "structFileDriver.h"
#ifdef TAR_STRUCT_FILE
#include "tarSubStructFileDriver.h"
#endif
#include "miscUtil.h"

int LogFd = -1;		/* the log file descriptor */
char *CurLogfileName = NULL;        /* the path of the current logfile */
char ProcLogDir[MAX_NAME_LEN];

rodsServerHost_t *LocalServerHost = NULL;
rodsServerHost_t *ServerHostHead = NULL;
rodsServerHost_t *HostConfigHead = NULL;
zoneInfo_t *ZoneInfoHead = NULL;
rescGrpInfo_t *RescGrpInfo = NULL;
rescGrpInfo_t *CachedRescGrpInfo = NULL;
int RescGrpInit = 0;	/* whether initRescGrp has been called */

/* global fileDesc */

fileDesc_t FileDesc[NUM_FILE_DESC];
l1desc_t L1desc[NUM_L1_DESC];
specCollDesc_t SpecCollDesc[NUM_SPEC_COLL_DESC];
collHandle_t CollHandle[NUM_COLL_HANDLE];

/* global Rule Engine File Initialization String */

char reRuleStr[LONG_NAME_LEN];
char reFuncMapStr[LONG_NAME_LEN];
char reVariableMapStr[LONG_NAME_LEN];

/* global Kerberos server-side name */
char KerberosName[MAX_NAME_LEN];

/* The stat of the Agent initialization */

int InitialState = INITIAL_NOT_DONE;
rsComm_t *ThisComm = NULL;

#ifdef RODS_CAT
int IcatConnState = INITIAL_NOT_DONE;
#endif

specCollCache_t *SpecCollCacheHead = NULL;

structFileDesc_t StructFileDesc[NUM_STRUCT_FILE_DESC];
#ifdef TAR_STRUCT_FILE
tarSubFileDesc_t TarSubFileDesc[NUM_TAR_SUB_FILE_DESC];
#endif

/* Server Authentication information */

char localSID[MAX_PASSWORD_LEN]; /* Local Zone Servers ID string */
char remoteSID[MAX_FED_RSIDS] [MAX_PASSWORD_LEN];  /* Remote Zone SIDs */

/* quota for all resources for this user in bytes */
rodsLong_t GlobalQuotaLimit;	/* quota for all resources for this user */
rodsLong_t GlobalQuotaOverrun;  /* quota overrun for this user */
int RescQuotaPolicy;		/* can be RESC_QUOTA_UNINIT, RESC_QUOTA_OFF or
				 * RESC_QUOTA_ON */
/* connection control config */
struct allowedUser *AllowedUserHead = NULL;
struct allowedUser *DisallowedUserHead = NULL;
int MaxConnections = DEF_MAX_CONNECTION;		/* no control */

time_t LastRescUpdateTime;

/* manage server process permissions */
#ifdef RUN_SERVER_AS_ROOT
uid_t ServiceUid = 0;
#endif

/* Flag for whether the read/write rule should be executed */
irodsStateFlag_t ReadWriteRuleState = UNINIT_STATE;

#endif	/* RS_GLOBAL_H */


