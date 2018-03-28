/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*****************************************************************************/

#ifndef ICAT_HIGHLEVEL_ROUTINES_HPP
#define ICAT_HIGHLEVEL_ROUTINES_HPP

#include "objInfo.h"
#include "ruleExecSubmit.h"
#include "rcConnect.h"
#include "icatStructs.hpp"
#include "rodsGeneralUpdate.h"
#include "specificQuery.h"
#include "phyBundleColl.h"
#include "irods_resource_manager.hpp"
using leaf_bundle_t = irods::resource_manager::leaf_bundle_t;

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <vector>
#include <map>

#include <boost/tuple/tuple.hpp>

#include "irods_error.hpp"

extern icatSessionStruct icss;

int chlOpen();
int chlClose();
int chlIsConnected();
int chlModDataObjMeta( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                       keyValPair_t *regParam );
int chlUpdateRescObjCount( const std::string& _resc, int _delta );
int chlRegDataObj( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo );
int chlRegRuleExecObj( rsComm_t *rsComm,
                       ruleExecSubmitInp_t *ruleExecSubmitInp );
int chlRegReplica( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                   dataObjInfo_t *dstDataObjInfo, keyValPair_t *condInput );
int chlUnregDataObj( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                     keyValPair_t *condInput );
int chlRegResc( rsComm_t *rsComm, std::map<std::string, std::string>& _resc_input );
int chlAddChildResc( rsComm_t* rsComm, std::map<std::string, std::string>& _resc_input );
int chlDelResc( rsComm_t *rsComm, const std::string& _resc_name, int _dryrun = 0 ); // JMC
int chlDelChildResc( rsComm_t* rsComm, std::map<std::string, std::string>& _resc_input );
int chlRollback( rsComm_t *rsComm );
int chlCommit( rsComm_t *rsComm );
int chlDelUserRE( rsComm_t *rsComm, userInfo_t *userInfo );
int chlRegCollByAdmin( rsComm_t *rsComm, collInfo_t *collInfo );
int chlRegColl( rsComm_t *rsComm, collInfo_t *collInfo );
int chlModColl( rsComm_t *rsComm, collInfo_t *collInfo );

int chlSimpleQuery( rsComm_t *rsComm, const char *sql,
                    const char *arg1, const char *arg2, const char *arg3, const char *arg4,
                    int format,
                    int *control, char *outBuf, int maxOutBuf );
int chlGenQuery( genQueryInp_t genQueryInp, genQueryOut_t *result );
int chlGenQueryAccessControlSetup( const char *user, const char *zone, const char *host,
                                   int priv, int controlFlag );
int chlGenQueryTicketSetup( const char *ticket, const char *clientAddr );
int chlSpecificQuery( specificQueryInp_t specificQueryInp,
                      genQueryOut_t *genQueryOut );

int chlGeneralUpdate( generalUpdateInp_t generalUpdateInp );

int chlDelCollByAdmin( rsComm_t *rsComm, collInfo_t *collInfo );
int chlDelColl( rsComm_t *rsComm, collInfo_t *collInfo );
int chlCheckAuth( rsComm_t *rsComm, const char* scheme, const char *challenge, const char *response,
                  const char *username, int *userPrivLevel, int *clientPrivLevel );
int chlMakeTempPw( rsComm_t *rsComm, char *pwValueToHash, const char *otherUser );
int chlMakeLimitedPw( rsComm_t *rsComm, int ttl, char *pwValueToHash );
int decodePw( rsComm_t *rsComm, const char *in, char *out );
int chlModUser( rsComm_t *rsComm, const char *userName, const char *option,
                const char *newValue );
int chlModGroup( rsComm_t *rsComm, const char *groupName, const char *option,
                 const char *userName, const char *userZone );
int chlModResc( rsComm_t *rsComm, const char *rescName, const char *option,
                const char *optionValue );
int chlModRescDataPaths( rsComm_t *rsComm, const char *rescName, const char *oldPath,
                         const char *newPath, const char *userName );
int chlModRescFreeSpace( rsComm_t *rsComm, const char *rescName,
                         int updateValue );
int chlRegUserRE( rsComm_t *rsComm, userInfo_t *userInfo );
rodsLong_t checkAndGetObjectId( rsComm_t *rsComm, const char *type,
                                const char *name, const char *access );
int chlAddAVUMetadata( rsComm_t *rsComm, int adminMode, const char *type,
                       const char *name, const char *attribute, const char *value, const char *units );
int chlAddAVUMetadataWild( rsComm_t *rsComm, int adminMode, const char *type,
                           const char *name, const char *attribute, const char *value, const char *units );
int chlDeleteAVUMetadata( rsComm_t *rsComm, int option, const char *type,
                          const char *name, const char *attribute, const char *value, const char *units, int noCommit );
int chlSetAVUMetadata( rsComm_t *rsComm, const char *type, // JMC - backport 4836
                       const char *name, const char *attribute, const char *newValue, const char *newUnit );
int chlCopyAVUMetadata( rsComm_t *rsComm, const char *type1,  const char *type2,
                        const char *name1, const char *name2 );
int chlModAVUMetadata( rsComm_t *rsComm, const char *type, const char *name,
                       const char *attribute, const char *value, const char *unitsOrChange0,
                       const char *change1, const char *change2, const char *change3 );
int chlModAccessControl( rsComm_t *rsComm, int recursiveFlag,
                         const char* accessLevel, const char *userName, const char *zone,
                         const char* pathName );

int chlRegRuleExec( rsComm_t *rsComm, ruleExecSubmitInp_t *ruleExecSubmitInp );
int chlModRuleExec( rsComm_t *rsComm, const char *ruleExecId, keyValPair_t *regParam );
int chlDelRuleExec( rsComm_t *rsComm, const char *ruleExecId );

int chlRenameObject( rsComm_t *rsComm, rodsLong_t objId, const char *newName );
int chlMoveObject( rsComm_t *rsComm, rodsLong_t objId, rodsLong_t targetCollId );

int chlRegToken( rsComm_t *rsComm, const char *nameSpace, const char *name, const char *value,
                 const char *value2, const char *value3, const char *comment );
int chlDelToken( rsComm_t *rsComm, const char *nameSpace, const char *Name );

int chlRegZone( rsComm_t *rsComm, const char *zoneName, const char *zoneType,
                const char *zoneConnInfo, const char *zoneComment );
int chlModZone( rsComm_t *rsComm, const char *zoneName, const char *option,
                const char *optionValue );
int chlModZoneCollAcl( rsComm_t *rsComm, const char* accessLevel, const char *userName,
                       const char* pathName );
int chlDelZone( rsComm_t *rsComm, const char *zoneName );
int chlRenameLocalZone( rsComm_t *rsComm, const char *oldZoneName, const char *newZoneName );
int chlRenameColl( rsComm_t *rsComm, const char *oldName, const char *newName );

int chlRegServerLoad( rsComm_t *rsComm,
                      const char *hostName, const char *rescName,
                      const char *cpuUsed, const char *memUsed, const char *swapUsed, const char *runqLoad,
                      const char *diskSpace, const char *netInput, const char *netOutput );
int chlPurgeServerLoad( rsComm_t *rsComm, const char *secondsAgo );
int chlRegServerLoadDigest( rsComm_t *rsComm, const char *rescName, const char *loadFactor );
int chlPurgeServerLoadDigest( rsComm_t *rsComm, const char *secondsAgo );

int chlCalcUsageAndQuota( rsComm_t *rsComm );
int chlSetQuota( rsComm_t *rsComm, const char *type, const char *name, const char *rescName,
                 const char *limit );
int chlCheckQuota( rsComm_t *rsComm, const char *userName, const char *rescName,
                   rodsLong_t *userQuota, int *quotaStatus );
int chlDelUnusedAVUs( rsComm_t *rsComm );
int chlAddSpecificQuery( rsComm_t *rsComm, const char *alias, const char *sql );
int chlDelSpecificQuery( rsComm_t *rsComm, const char *sqlOrAlias );

int chlGetLocalZone( std::string& );

int sTableInit();
int sFklink( const char *table1, const char *table2, const char *connectingSQL );
int sTable( const char *tableName, const char *tableAlias, int cycler );
int sColumn( int defineVal, const char *tableName, const char *columnName );

int chlDebug( const char *debugMode );
int chlDebugGenQuery( int mode );
int chlDebugGenUpdate( int mode );
int chlInsRuleTable( rsComm_t *rsComm,
                     const char *baseName, const char *priorityStr, const char *ruleName,
                     const char *ruleHead, const char *ruleCondition, const char *ruleAction,
                     const char *ruleRecovery, const char *ruleIdStr, const char *myTime );
int chlVersionRuleBase( rsComm_t *rsComm,
                        const char *baseName, const char *myTime );
int chlVersionDvmBase( rsComm_t *rsComm,
                       const char *baseName, const char *myTime );
/*int chlDatabaseObjectAdmin(rsComm_t *rsComm,
  databaseObjectAdminInp_t *databaseObjectAdminInp,
  databaseObjectAdminOut_t *databaseObjectAdminOut);*/
int chlInsDvmTable( rsComm_t *rsComm,
                    const char *baseName, const char *varName, const char *action,
                    const char *var2CMap, const char *myTime );
int chlInsFnmTable( rsComm_t *rsComm,
                    const char *baseName, const char *funcName,
                    const char *func2CMap, const char *myTime );
int chlInsMsrvcTable( rsComm_t *rsComm,
                      const char *moduleName,
                      const char *msrvcName,
                      const char *msrvcSignature,
                      const char *msrvcVersion,
                      const char *msrvcHost,
                      const char *msrvcLocation,
                      const char *msrvcLanguage,
                      const char *msrvcTypeName,
                      const char *msrvcStatus,
                      const char *myTime );
int chlVersionFnmBase( rsComm_t *rsComm,
                       const char *baseName, const char *myTime );
int chlModTicket( rsComm_t *rsComm, const char *opName, const char *ticket,
                  const char *arg1, const char *arg2, const char *arg3 );
int chlUpdateIrodsPamPassword( rsComm_t *rsComm, const char *userName,
                               int timeToLive, const char *testTime,
                               char **irodsPassword );

/// =-=-=-=-=-=-=-
/// @brief typedefs and prototype for query used for rebalancing operation
typedef std::vector< rodsLong_t > dist_child_result_t;

/// =-=-=-=-=-=-=-
/// @brief query which distinct data objects do not existin on a
///        given child resource which do exist on the parent
int chlGetDistinctDataObjsMissingFromChildGivenParent(
    const std::string&   _parent,
    const std::string&   _child,
    int                  _limit,
    const std::string&   _invocation_timestamp,
    dist_child_result_t& _results );

/// =-=-=-=-=-=-=-
/// @brief the the distinct data object count for a resource
int chlGetDistinctDataObjCountOnResource(
    const std::string&   _resc_name,
    long long&           _count );

int chlGetHierarchyForResc(
    const std::string&	resc_name,
    const std::string&	zone_name,
    std::string& hierarchy );

int chlCheckAndGetObjectID(
    rsComm_t*, // comm
    char*,     // type
    char*,     // name
    char* );   // access

int chlGetRcs( icatSessionStruct** );

int chlGetReplListForLeafBundles(
    rodsLong_t                  _count,
    size_t                      _child_idx,
    const std::vector<leaf_bundle_t>* _bundles,
    const std::string*          _invocation_timestamp,
    dist_child_result_t*        _results );

#endif /* ICAT_HIGHLEVEL_ROUTINES_H */
