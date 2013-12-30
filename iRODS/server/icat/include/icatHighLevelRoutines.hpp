/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */


/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*****************************************************************************/

#ifndef ICAT_HIGHLEVEL_ROUTINES_HPP
#define ICAT_HIGHLEVEL_ROUTINES_HPP

#include "objInfo.hpp"
#include "ruleExecSubmit.hpp"
#include "rcConnect.hpp"
#include "icatStructs.hpp"
#include "rodsGeneralUpdate.hpp"
#include "specificQuery.hpp"
#include "phyBundleColl.hpp"
#include "readServerConfig.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <vector>
#include <map>

#include <boost/tuple/tuple.hpp>

extern icatSessionStruct icss;

int chlOpen( rodsServerConfig* );
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
int chlRegResc( rsComm_t *rsComm, rescInfo_t *rescInfo );
int chlAddChildResc( rsComm_t* rsComm, rescInfo_t* rescInfo );
int chlDelResc( rsComm_t *rsComm, rescInfo_t *rescInfo, int _dryrun = 0 ); // JMC
int chlDelChildResc( rsComm_t* rsComm, rescInfo_t* rescInfo );
int chlRollback( rsComm_t *rsComm );
int chlCommit( rsComm_t *rsComm );
int chlDelUserRE( rsComm_t *rsComm, userInfo_t *userInfo );
int chlRegCollByAdmin( rsComm_t *rsComm, collInfo_t *collInfo );
int chlRegColl( rsComm_t *rsComm, collInfo_t *collInfo );
int chlModColl( rsComm_t *rsComm, collInfo_t *collInfo );

int chlSimpleQuery( rsComm_t *rsComm, char *sql,
                    char *arg1, char *arg2, char *arg3, char *arg4,
                    int format,
                    int *control, char *outBuf, int maxOutBuf );
int chlGenQuery( genQueryInp_t genQueryInp, genQueryOut_t *result );
int chlGenQueryAccessControlSetup( char *user, char *zone, char *host,
                                   int priv, int controlFlag );
int chlGenQueryTicketSetup( char *ticket, char *clientAddr );
int chlSpecificQuery( specificQueryInp_t specificQueryInp,
                      genQueryOut_t *genQueryOut );

int chlGeneralUpdate( generalUpdateInp_t generalUpdateInp );

int chlDelCollByAdmin( rsComm_t *rsComm, collInfo_t *collInfo );
int chlDelColl( rsComm_t *rsComm, collInfo_t *collInfo );
int chlCheckAuth( rsComm_t *rsComm, const char* scheme, char *challenge, char *response,
                  char *username, int *userPrivLevel, int *clientPrivLevel );
int chlMakeTempPw( rsComm_t *rsComm, char *pwValueToHash, char *otherUser );
int chlMakeLimitedPw( rsComm_t *rsComm, int ttl, char *pwValueToHash );
int decodePw( rsComm_t *rsComm, char *in, char *out );
int chlModUser( rsComm_t *rsComm, char *userName, char *option,
                char *newValue );
int chlModGroup( rsComm_t *rsComm, char *groupName, char *option,
                 char *userName, char *userZone );
int chlModResc( rsComm_t *rsComm, char *rescName, char *option,
                char *optionValue );
int chlModRescDataPaths( rsComm_t *rsComm, char *rescName, char *oldPath,
                         char *newPath, char *userName );
int chlModRescFreeSpace( rsComm_t *rsComm, char *rescName,
                         int updateValue );
int chlRegUserRE( rsComm_t *rsComm, userInfo_t *userInfo );
rodsLong_t checkAndGetObjectId( rsComm_t *rsComm, char *type,
                                char *name, char *access );
int chlAddAVUMetadata( rsComm_t *rsComm, int adminMode, char *type,
                       char *name, char *attribute, char *value,  char *units );
int chlAddAVUMetadataWild( rsComm_t *rsComm, int adminMode, char *type,
                           char *name, char *attribute, char *value,  char *units );
int chlDeleteAVUMetadata( rsComm_t *rsComm, int option, char *type,
                          char *name, char *attribute, char *value,  char *units, int noCommit );
int chlSetAVUMetadata( rsComm_t *rsComm, char *type, // JMC - backport 4836
                       char *name, char *attribute, char *newValue, char *newUnit );
int chlCopyAVUMetadata( rsComm_t *rsComm, char *type1,  char *type2,
                        char *name1, char *name2 );
int chlModAVUMetadata( rsComm_t *rsComm, char *type, char *name,
                       char *attribute, char *value, char *unitsOrChange0,
                       char *change1, char *change2, char *change3 );
int chlModAccessControl( rsComm_t *rsComm, int recursiveFlag,
                         char* accessLevel, char *userName, char *zone,
                         char* pathName );

#ifdef RESC_GROUP
int chlModRescGroup( rsComm_t *rsComm, char *rescGroupName, char *option,
                     char *rescName );
#endif

int chlRegRuleExec( rsComm_t *rsComm, ruleExecSubmitInp_t *ruleExecSubmitInp );
int chlModRuleExec( rsComm_t *rsComm, char *ruleExecId, keyValPair_t *regParam );
int chlDelRuleExec( rsComm_t *rsComm, char *ruleExecId );

int chlRenameObject( rsComm_t *rsComm, rodsLong_t objId, char *newName );
int chlMoveObject( rsComm_t *rsComm, rodsLong_t objId, rodsLong_t targetCollId );

int chlRegToken( rsComm_t *rsComm, char *nameSpace, char *name, char *value,
                 char *value2, char *value3, char *comment );
int chlDelToken( rsComm_t *rsComm, char *nameSpace, char *Name );

int chlRegZone( rsComm_t *rsComm, char *zoneName, char *zoneType,
                char *zoneConnInfo, char *zoneComment );
int chlModZone( rsComm_t *rsComm, char *zoneName, char *option,
                char *optionValue );
int chlModZoneCollAcl( rsComm_t *rsComm, char* accessLevel, char *userName,
                       char* pathName );
int chlDelZone( rsComm_t *rsComm, char *zoneName );
int chlRenameLocalZone( rsComm_t *rsComm, char *oldZoneName, char *newZoneName );
int chlRenameColl( rsComm_t *rsComm, char *oldName, char *newName );

int chlRegServerLoad( rsComm_t *rsComm,
                      char *hostName, char *rescName,
                      char *cpuUsed, char *memUsed, char *swapUsed, char *runqLoad,
                      char *diskSpace, char *netInput, char *netOutput );
int chlPurgeServerLoad( rsComm_t *rsComm, char *secondsAgo );
int chlRegServerLoadDigest( rsComm_t *rsComm, char *rescName, char *loadFactor );
int chlPurgeServerLoadDigest( rsComm_t *rsComm, char *secondsAgo );

int chlCalcUsageAndQuota( rsComm_t *rsComm );
int chlSetQuota( rsComm_t *rsComm, char *type, char *name, char *rescName,
                 char *limit );
int chlCheckQuota( rsComm_t *rsComm, char *userName, char *rescName,
                   rodsLong_t *userQuota, int *quotaStatus );
int chlDelUnusedAVUs( rsComm_t *rsComm );
int chlAddSpecificQuery( rsComm_t *rsComm, char *alias, char *sql );
int chlDelSpecificQuery( rsComm_t *rsComm, char *sqlOrAlias );

int chlGetLocalZone( std::string& );

int sTableInit();
int sFklink( char *table1, char *table2, char *connectingSQL );
int sTable( char *tableName, char *tableAlias, int cycler );
int sColumn( int defineVal, char *tableName, char *columnName );

int chlDebug( char *debugMode );
int chlDebugGenQuery( int mode );
int chlDebugGenUpdate( int mode );
int chlInsRuleTable( rsComm_t *rsComm,
                     char *baseName, char *priorityStr, char *ruleName,
                     char *ruleHead, char *ruleCondition,
                     char *ruleAction,
                     char *ruleRecovery, char *ruleIdStr, char *myTime );
int chlVersionRuleBase( rsComm_t *rsComm,
                        char *baseName, char *myTime );
int chlVersionDvmBase( rsComm_t *rsComm,
                       char *baseName, char *myTime );
/*int chlDatabaseObjectAdmin(rsComm_t *rsComm,
  databaseObjectAdminInp_t *databaseObjectAdminInp,
  databaseObjectAdminOut_t *databaseObjectAdminOut);*/
int chlInsDvmTable( rsComm_t *rsComm,
                    char *baseName, char *varName, char *action,
                    char *var2CMap, char *myTime );
int chlInsFnmTable( rsComm_t *rsComm,
                    char *baseName, char *funcName,
                    char *func2CMap, char *myTime );
int chlInsMsrvcTable( rsComm_t *rsComm,
                      char *moduleName,
                      char *msrvcName,
                      char *msrvcSignature,
                      char *msrvcVersion,
                      char *msrvcHost,
                      char *msrvcLocation,
                      char *msrvcLanguage,
                      char *msrvcTypeName,
                      char *msrvcStatus,
                      char *myTime );
int chlVersionDvmBase( rsComm_t *rsComm,
                       char *baseName, char *myTime );
int chlVersionFnmBase( rsComm_t *rsComm,
                       char *baseName, char *myTime );
int chlModTicket( rsComm_t *rsComm, char *opName, char *ticket,
                  char *arg1, char *arg2, char *arg3 );
int chlUpdateIrodsPamPassword( rsComm_t *rsComm, char *userName,
                               int timeToLive, char *testTime,
                               char **irodsPassword );


irods::error chlRescObjCount( const std::string& _resc_name, int& _rtn_obj_count );

int chlSubstituteResourceHierarchies( rsComm_t *rsComm, const char *old_hier, const char *new_hier );

/// =-=-=-=-=-=-=-
/// @brief typedefs and prototype for query used for rebalancing operation
typedef std::vector< int > dist_child_result_t;

/// =-=-=-=-=-=-=-
/// @brief query which distinct data objects do not existin on a
///        given child resource which do exist on the parent
int chlGetDistinctDataObjsMissingFromChildGivenParent(
    const std::string&   _parent,
    const std::string&   _child,
    int                  _limit,
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

#endif /* ICAT_HIGHLEVEL_ROUTINES_H */
