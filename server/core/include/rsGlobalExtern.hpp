#ifndef RS_GLOBAL_EXTERN_HPP
#define RS_GLOBAL_EXTERN_HPP

/// \file

#include "rods.h"
#include "apiHandler.hpp"
#include "fileOpr.hpp"
#include "objDesc.hpp"
#include "querySpecColl.h"
#include "miscUtil.h"
#include "authenticate.h"
#include "openCollection.h"

#include "irods_resource_manager.hpp"

// externs to singleton plugin managers
extern irods::resource_manager resc_mgr;

extern int LogFd;         		/* the log file descriptor */
extern char *CurLogfileName;         	/* the path of the current logfile */
extern char ProcLogDir[MAX_NAME_LEN];
extern irods::api_entry_table RsApiTable;
extern int RescGrpInit;
extern specCollDesc_t SpecCollDesc[NUM_SPEC_COLL_DESC];
extern std::vector<collHandle_t> CollHandle;;

/// \brief Global table of L1 descriptors which contains information about open replicas.
extern l1desc_t L1desc[NUM_L1_DESC];

/// \brief Global table of file descriptors which contains information about open files.
extern fileDesc_t FileDesc[NUM_FILE_DESC];

/// \brief Global linked list of iRODS servers to which the agent has connected.
///
/// \parblock
/// `LocalServerHost` is added as the head of this list in `initLocalServerHost()`.
///
/// Whenever a new connection is made to an iRODS server, its `rodsServerHost` information
/// should be added to this list.
///
/// After the connection is no longer needed, it should be left in the list for reuse within
/// the agent should a later redirection to the same server be needed later. All of the
/// connections held open in this linked list will be connected on agent teardown via
/// `cleanup()`.
/// \endparblock
extern rodsServerHost_t *ServerHostHead;

/// \brief Global head of a linked list of connected iRODS servers, `ServerHostHead`.
///
/// \parblock
/// This `rodsServerHost*` represents the local server and is always the head of the linked list
/// of connected servers.
/// \endparblock
extern rodsServerHost_t *LocalServerHost;

/// \brief Global linked list of zones. The head is always the local zone.
///
/// \p `queZone` is used to add zones to this list.
extern zoneInfo_t *ZoneInfoHead;

extern rodsServerHost_t *HostConfigHead;

/* global Rule Engine File Initialization String */

extern char reRuleStr[LONG_NAME_LEN];
extern char reFuncMapStr[LONG_NAME_LEN];
extern char reVariableMapStr[LONG_NAME_LEN];

/* Kerberos server name */
extern char KerberosName[MAX_NAME_LEN];

extern int InitialState;
extern rsComm_t *ThisComm;

extern int IcatConnState;

extern specCollCache_t *SpecCollCacheHead;

extern char localSID[MAX_PASSWORD_LEN];
extern irods::lookup_table <std::pair <std::string, std::string> > remote_SID_key_map; // remote zone SIDs and negotiation keys

/* quota for all resources for this user in bytes */
extern rodsLong_t GlobalQuotaLimit; /* quota for all resources for this user */
extern rodsLong_t GlobalQuotaOverrun;  /* quota overrun for this user */
extern int RescQuotaPolicy;
extern time_t LastRescUpdateTime;

/* manage server process permissions */
extern uid_t ServiceUid;
extern gid_t ServiceGid;

extern irodsStateFlag_t ReadWriteRuleState;

#endif // RS_GLOBAL_EXTERN_HPP

