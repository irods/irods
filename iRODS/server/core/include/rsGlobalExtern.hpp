/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsGlobalExtern.h - header file for global extern declaration for the server
 * modules
 */

#ifndef RS_GLOBAL_EXTERN_HPP
#define RS_GLOBAL_EXTERN_HPP

#include "rods.hpp"
#include "apiHandler.hpp"
#include "fileOpr.hpp"
#include "objDesc.hpp"
#include "querySpecColl.hpp"
#include "miscUtil.hpp"
#include "authenticate.hpp"
#include "openCollection.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_manager.hpp"

// =-=-=-=-=-=-=-
// externs to singleton plugin managers
extern irods::resource_manager resc_mgr;

extern int LogFd;         		/* the log file descriptor */
extern char *CurLogfileName;         	/* the path of the current logfile */
extern char ProcLogDir[MAX_NAME_LEN];
extern irods::api_entry_table RsApiTable;
extern rodsServerHost_t *LocalServerHost;
extern rodsServerHost_t *ServerHostHead;
extern rodsServerHost_t *HostConfigHead;
extern zoneInfo_t *ZoneInfoHead;
extern int RescGrpInit;
extern fileDesc_t FileDesc[NUM_FILE_DESC];
extern l1desc_t L1desc[NUM_L1_DESC];
extern specCollDesc_t SpecCollDesc[NUM_SPEC_COLL_DESC];
extern collHandle_t CollHandle[NUM_COLL_HANDLE];

/* global Rule Engine File Initialization String */

extern char reRuleStr[LONG_NAME_LEN];
extern char reFuncMapStr[LONG_NAME_LEN];
extern char reVariableMapStr[LONG_NAME_LEN];

/* Kerberos server name */
extern char KerberosName[MAX_NAME_LEN];

extern int InitialState;
extern rsComm_t *ThisComm;

#ifdef RODS_CAT
extern int IcatConnState;
#endif

extern specCollCache_t *SpecCollCacheHead;

int initRuleEngine( int processType, rsComm_t *svrComm, char *ruleSet, char *dvmSet, char* fnmSet );
int clearCoreRule();
int finalizeRuleEngine();

extern char localSID[MAX_PASSWORD_LEN];
extern irods::lookup_table <std::pair <std::string, std::string> > remote_SID_key_map; // remote zone SIDs and negotiation keys

/* quota for all resources for this user in bytes */
extern rodsLong_t GlobalQuotaLimit; /* quota for all resources for this user */
extern rodsLong_t GlobalQuotaOverrun;  /* quota overrun for this user */
extern int RescQuotaPolicy;
/* connection control config */
extern struct allowedUser *AllowedUserHead;
extern struct allowedUser *DisallowedUserHead;
extern int MaxConnections;          /* no control */
extern time_t LastRescUpdateTime;

/* manage server process permissions */
extern uid_t ServiceUid;
extern gid_t ServiceGid;

extern irodsStateFlag_t ReadWriteRuleState;

#endif  /* RS_GLOBAL_EXTERN_H */

