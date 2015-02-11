/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* miscUtil.h - Header file for miscUtil.c */

#ifndef MISC_UTIL_HPP
#define MISC_UTIL_HPP

#include "rodsPath.hpp"
#include "parseCommandLine.hpp"
#include "guiProgressCallback.hpp"

#define	INIT_UMASK_VAL	99999999
typedef struct CollSqlResult {
    int rowCnt;
    int attriCnt;
    int continueInx;
    int totalRowCount;
    sqlResult_t collName;
    sqlResult_t collType;
    sqlResult_t collInfo1;
    sqlResult_t collInfo2;
    sqlResult_t collOwner;
    sqlResult_t collCreateTime;
    sqlResult_t collModifyTime;
} collSqlResult_t;

typedef struct CollMetaInfo {
    char *collName;
    char *collOwner;
    specColl_t specColl;
} collMetaInfo_t;

typedef struct DataObjSqlResult {
    int rowCnt;
    int attriCnt;
    int continueInx;
    int totalRowCount;
    sqlResult_t collName;
    sqlResult_t dataName;
    sqlResult_t dataMode;
    sqlResult_t dataSize;
    sqlResult_t createTime;
    sqlResult_t modifyTime;
    sqlResult_t chksum;		/* chksum, replStatus and dataId are used only
				 * for rsync */
    sqlResult_t replStatus;
    sqlResult_t dataId;
    sqlResult_t resource;
    sqlResult_t resc_hier;
    sqlResult_t phyPath;
    sqlResult_t ownerName;
    sqlResult_t replNum;
    sqlResult_t dataType; // JMC - backport 4636
} dataObjSqlResult_t;

typedef struct DataObjMetaInfo {
    char *collName;
    char *dataName;
    char *dataSize;
    char *createTime;
    char *modifyTime;
    char *chksum;
    char *replStatus;
    char *dataId;
} dataObjMetaInfo_t;

/* definition for state in collHandle_t */
typedef enum {
    COLL_CLOSED,
    COLL_OPENED,
    COLL_DATA_OBJ_QUERIED,
    COLL_COLL_OBJ_QUERIED
} collState_t;

typedef enum {
    RC_COMM,
    RS_COMM
} connType_t;

/* struct for query by both client and server */
typedef struct QueryHandle {
    void *conn;         /* either rsComm or rcComm */
    connType_t connType;
    funcPtr querySpecColl;      /* rcQuerySpecColl or rsQuerySpecColl */
    funcPtr genQuery;           /* rcGenQuery or rsGenQuery */
} queryHandle_t;

/* definition for flag in rclOpenCollection and collHandle_t */
#define LONG_METADATA_FG     0x1     /* get verbose metadata */
#define VERY_LONG_METADATA_FG     0x2   /* get verbose metadata */
#define RECUR_QUERY_FG       0x4     /* get recursive query */
#define DATA_QUERY_FIRST_FG       0x8     /* get data res first */
#define NO_TRIM_REPL_FG       0x10     /* don't trim the replica */
#define INCLUDE_CONDINPUT_IN_QUERY       0x20  /* include the cond in condInput
* in the query */

typedef struct CollHandle {
    collState_t state;
    int inuseFlag;
    int flags;
    int rowInx;
    rodsObjStat_t *rodsObjStat;
    queryHandle_t queryHandle;
    genQueryInp_t genQueryInp;
    dataObjInp_t dataObjInp;
    dataObjSqlResult_t dataObjSqlResult;
    collSqlResult_t collSqlResult;
    char linkedObjPath[MAX_NAME_LEN];
    char prevdataId[NAME_LEN];
} collHandle_t;

/* the output of rclReadCollection */
typedef struct CollEnt {
    objType_t objType;
    int replNum;
    int replStatus;
    uint dataMode;
    rodsLong_t dataSize;
    char *collName;		/* valid for dataObj and collection */
    char *dataName;
    char *dataId;
    char *createTime;
    char *modifyTime;
    char *chksum;
    char *resource;
    char *resc_hier;
    char *phyPath;
    char *ownerName;    	 /* valid for dataObj and collection */
    char *dataType;		 // JMC - backport 4636
    specColl_t specColl;	 /* valid only for collection */
} collEnt_t;

/* used to store regex patterns used to match pathnames */
typedef struct {
    char *pattern_buf;
    char **patterns;
    int num_patterns;
} pathnamePatterns_t;


#ifdef __cplusplus
extern "C" {
#endif

int
mkdirR( char *startDir, char *destDir, int mode );
int
rmdirR( char *startDir, char *destDir );
int
mkColl( rcComm_t *conn, char *collection );
int
mkCollR( rcComm_t *conn, char *startColl, char *destColl );
int
getRodsObjType( rcComm_t *conn, rodsPath_t *rodsPath );
int
genAllInCollQCond( char *collection, char *collQCond );
int
queryCollInColl( queryHandle_t *queryHandle, char *collection,
                 int flags, genQueryInp_t *genQueryInp,
                 genQueryOut_t **genQueryOut );
int
queryDataObjInColl( queryHandle_t *queryHandle, char *collection,
                    int flags, genQueryInp_t *genQueryInp,
                    genQueryOut_t **genQueryOut, keyValPair_t *condInput );
int
setQueryInpForData( int flags, genQueryInp_t *genQueryInp );

int
printTiming( rcComm_t *conn, char *objPath, rodsLong_t fileSize,
             char *localFile, struct timeval *startTime, struct timeval *endTime );
int
printTime( char *objPath, struct timeval *startTime,
           struct timeval *endTime );
int
initSysTiming( char *procName, char *action, int envVarFlag );
int
printSysTiming( char *procName, char *action, int envVarFlag );
int
printNoSync( char *objPath, rodsLong_t fileSize, char *reason );
int
queryDataObjAcl( rcComm_t *conn, char *dataId, char *zoneHint,
                 genQueryOut_t **genQueryOut ); // JMC - backport 4516
int
queryCollAcl( rcComm_t *conn, char *collName, char *zoneHint,
              genQueryOut_t **genQueryOut ); // JMC - backport 4516
int
queryCollAclSpecific( rcComm_t *conn, char *collName, char *zoneHint,
                      genQueryOut_t **genQueryOut );
int
queryCollInheritance( rcComm_t *conn, char *collName,
                      genQueryOut_t **genQueryOut );
int
extractRodsObjType( rodsPath_t *rodsPath, sqlResult_t *dataId,
                    sqlResult_t *replStatus, sqlResult_t *chksum, sqlResult_t *dataSize,
                    int inx, int rowCnt );
int
genQueryOutToCollRes( genQueryOut_t **genQueryOut,
                      collSqlResult_t *collSqlResult );
int
setSqlResultValue( sqlResult_t *sqlResult, int attriInx, char *valueStr,
                   int rowCnt );
int
clearCollSqlResult( collSqlResult_t *collSqlResult );
int
clearDataObjSqlResult( dataObjSqlResult_t *dataObjSqlResult );
int
genQueryOutToDataObjRes( genQueryOut_t **genQueryOut,
                         dataObjSqlResult_t *dataObjSqlResult );
int
rclOpenCollection( rcComm_t *conn, char *collection,
                   int flag, collHandle_t *collHandle );
int
rclReadCollection( rcComm_t *conn, collHandle_t *collHandle,
                   collEnt_t *collEnt );
int
readCollection( collHandle_t *collHandle, collEnt_t *collEnt );
int
clearCollHandle( collHandle_t *collHandle, int freeSpecColl );
int
rclCloseCollection( collHandle_t *collHandle );
int
getNextCollMetaInfo( collHandle_t *collHandle, collEnt_t *outCollEnt );
int
getNextDataObjMetaInfo( collHandle_t *collHandle, collEnt_t *outCollEnt );
int
genCollResInColl( queryHandle_t *queryHandle, collHandle_t *collHandle );
int
genDataResInColl( queryHandle_t *queryHandle, collHandle_t *collHandle );
int
setQueryFlag( rodsArguments_t *rodsArgs );
int
rclInitQueryHandle( queryHandle_t *queryHandle, rcComm_t *conn );
int
freeCollEnt( collEnt_t *collEnt );
int
clearCollEnt( collEnt_t *collEnt );
int
myChmod( char *inPath, uint dataMode );
char *
getZoneHintForGenQuery( genQueryInp_t *genQueryInp );
int
getZoneType( rcComm_t *conn, char *icatZone, char *inZoneName,
             char *outZoneType );
int
getCollSizeForProgStat( rcComm_t *conn, char *srcColl,
                        operProgress_t *operProgress );
int
getDirSizeForProgStat( rodsArguments_t *rodsArgs, char *srcDir,
                       operProgress_t *operProgress );
guiProgressCallback
iCommandProgStat( operProgress_t *operProgress );
int
getOpenedCollLen( collHandle_t *collHandle );
int
rmSubDir( char *mydir );
int
rmFilesInDir( char *mydir );
int
mkdirForFilePath( char* filePath );
int
getFileMetaFromPath( char *srcPath, keyValPair_t *condInput );
int
getFileMetaFromStat( rodsStat_t *statbuf, keyValPair_t *condInput );
pathnamePatterns_t *
readPathnamePatterns( char *buf, int buflen );
void
freePathnamePatterns( pathnamePatterns_t *pp );
int
matchPathname( pathnamePatterns_t *pp, char *name, char *dirname );
#ifdef __cplusplus
}
#endif
#endif	/* MISC_UTIL_H */
