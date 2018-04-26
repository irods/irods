/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef windows_platform
#include <sys/time.h>
#endif
#include <fnmatch.h>
#include "rodsClient.h"
#include "rodsLog.h"
#include "miscUtil.h"
#include "rcGlobalExtern.h"

#include "irods_stacktrace.hpp"

#include "get_hier_from_leaf_id.h"

#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

/* VERIFY_DIV - contributed by g.soudlenkov@auckland.ac.nz */
#define VERIFY_DIV(_v1_,_v2_) ((_v2_)? (float)(_v1_)/(_v2_):0.0)

static uint Myumask = INIT_UMASK_VAL;

const char NON_ROOT_COLL_CHECK_STR[] = "<>'/'";

int
mkColl( rcComm_t *conn, char *collection ) {
    int status;
    collInp_t collCreateInp;

    memset( &collCreateInp, 0, sizeof( collCreateInp ) );

    rstrcpy( collCreateInp.collName, collection, MAX_NAME_LEN );

    status = rcCollCreate( conn, &collCreateInp );

    if ( status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) {
        status = 0;
    }

    return status;
}

/* mk the directory recursively */

int
mkCollR( rcComm_t *conn, char *startColl, char *destColl ) {
    int status;
    int startLen;
    int pathLen, tmpLen;
    char tmpPath[MAX_NAME_LEN];
    rodsPath_t rodsPath;

    startLen = strlen( startColl );
    pathLen = strlen( destColl );

    rstrcpy( tmpPath, destColl, MAX_NAME_LEN );

    tmpLen = pathLen;

    memset( &rodsPath, 0, sizeof( rodsPath ) );
    while ( tmpLen > startLen ) {
        rodsPath.objType = COLL_OBJ_T;
        rodsPath.objState = UNKNOWN_ST;
        rstrcpy( rodsPath.outPath, tmpPath, MAX_NAME_LEN );
        status = getRodsObjType( conn, &rodsPath );
        if ( status >= 0 && rodsPath.objState == EXIST_ST ) {
            clearRodsPath( &rodsPath );
            break;
        }
        else {
            clearRodsPath( &rodsPath );
        }

        /* Go backward */

        while ( tmpLen && tmpPath[tmpLen] != '/' ) {
            tmpLen --;
        }
        tmpPath[tmpLen] = '\0';
    }

    /* Now we go forward and make the required coll */
    while ( tmpLen < pathLen ) {
        /* Put back the '/' */
        tmpPath[tmpLen] = '/';
        status = mkColl( conn, tmpPath );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "mkCollR: mkdir failed for %s, status =%d",
                     tmpPath, status );
            return status;
        }
        while ( tmpLen && tmpPath[tmpLen] != '\0' ) {
            tmpLen ++;
        }
    }
    return 0;
}

int
mkdirR( char *startDir, char *destDir, int mode ) {
    using namespace boost::filesystem;
    int status = 0;
    int startLen;
    int pathLen, tmpLen;
    char tmpPath[MAX_NAME_LEN];
    startLen = strlen( startDir );
    pathLen = strlen( destDir );

    rstrcpy( tmpPath, destDir, MAX_NAME_LEN );

    tmpLen = pathLen;

    while ( tmpLen > startLen ) {
        path p( tmpPath );
        if ( exists( p ) ) {
            break;
        }

        /* Go backward */

        while ( tmpLen && tmpPath[tmpLen] != '/' ) {
            tmpLen --;
        }
        tmpPath[tmpLen] = '\0';
    }

    /* Now we go forward and make the required coll */
    while ( tmpLen < pathLen ) {
        /* Put back the '/' */
        tmpPath[tmpLen] = '/';
#ifdef _WIN32
        status = iRODSNt_mkdir( tmpPath, mode );
#else

        status = mkdir( tmpPath, mode );
#endif
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "mkdirR: mkdir failed for %s, errno =%d",
                     tmpPath, errno );
            return UNIX_FILE_MKDIR_ERR - errno;
        }
        while ( tmpLen && tmpPath[tmpLen] != '\0' ) {
            tmpLen ++;
        }
    }
    return 0;
}

int
mkdirForFilePath( char* filePath ) {
    char child[MAX_NAME_LEN], parent[MAX_NAME_LEN];
    int status;

    if ( ( status = splitPathByKey( filePath, parent, MAX_NAME_LEN, child, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "mkdirForFilePath:: splitPathByKey for %s error, status = %d",
                      filePath, status );
        return status;
    }

    status =  mkdirR( "/", parent, DEFAULT_DIR_MODE );

    return status;
}

int
rmdirR( char *startDir, char *destDir ) {
    int startLen;
    int pathLen, tmpLen;
    char tmpPath[MAX_NAME_LEN];

    startLen = strlen( startDir );
    pathLen = strlen( destDir );

    rstrcpy( tmpPath, destDir, MAX_NAME_LEN );

    tmpLen = pathLen;

    while ( tmpLen > startLen ) {
        rmdir( tmpPath );

        /* Go backward */

        while ( tmpLen && tmpPath[tmpLen] != '/' ) {
            tmpLen --;
        }
        tmpPath[tmpLen] = '\0';
    }
    return 0;
}

int
getRodsObjType( rcComm_t *conn, rodsPath_t *rodsPath ) {
    int status;
    dataObjInp_t dataObjInp;
    rodsObjStat_t *rodsObjStatOut = nullptr;

    if ( rodsPath == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );

    rstrcpy( dataObjInp.objPath, rodsPath->outPath, MAX_NAME_LEN );
    status = rcObjStat( conn, &dataObjInp, &rodsObjStatOut );

    if ( status < 0 ) {

        rodsPath->objState = NOT_EXIST_ST;

        if ( status == OBJ_PATH_DOES_NOT_EXIST ||
                status == USER_FILE_DOES_NOT_EXIST ) {
            return NOT_EXIST_ST;
        }
        else {
            rodsLogError( LOG_ERROR, status,
                          "rcObjStat of %s failed", rodsPath->outPath );
            return status;
        }
    }
    else if ( rodsPath->objType == COLL_OBJ_T &&
              rodsObjStatOut->objType != COLL_OBJ_T ) {
        rodsPath->objState = NOT_EXIST_ST;
    }
    else if ( rodsPath->objType == DATA_OBJ_T &&
              rodsObjStatOut->objType != DATA_OBJ_T ) {
        rodsPath->objState = NOT_EXIST_ST;
    }
    else {
        if ( rodsObjStatOut->objType == UNKNOWN_OBJ_T ) {
            rodsPath->objState = NOT_EXIST_ST;
        }
        else {
            rodsPath->objState = EXIST_ST;
        }
        rodsPath->objType = rodsObjStatOut->objType;
        if ( rodsPath->objType == DATA_OBJ_T ) {
            rodsPath->objMode = rodsObjStatOut->dataMode;
            rstrcpy( rodsPath->dataId, rodsObjStatOut->dataId, NAME_LEN );
            rodsPath->size = rodsObjStatOut->objSize;
            rstrcpy( rodsPath->chksum, rodsObjStatOut->chksum, NAME_LEN );
        }
    }
    rodsPath->rodsObjStat = rodsObjStatOut;

    return rodsPath->objState;
}

/* genAllInCollQCond - Generate a sqlCondInp for querying every thing under
 * a collection.
 * The server will handle special char such as "-" and "%".
 */

int
genAllInCollQCond( char *collection, char *collQCond ) {
    if ( collection == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( strcmp( collection, "/" ) == 0 ) {
        snprintf( collQCond, MAX_NAME_LEN, " like '/%%' " );
    }
    else {

        snprintf( collQCond, MAX_NAME_LEN, " = '%s' || like '%s/%%' ",
                  collection, collection );
    }
    return 0;
}

/* queryCollInColl - query the subCollections in a collection.
 */

int
queryCollInColl( queryHandle_t *queryHandle, char *collection,
                 int flags, genQueryInp_t *genQueryInp,
                 genQueryOut_t **genQueryOut ) {
    char collQCond[MAX_NAME_LEN];
    int status;

    if ( collection == nullptr || genQueryOut == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( genQueryInp, 0, sizeof( genQueryInp_t ) );

    snprintf( collQCond, MAX_NAME_LEN, "%s", NON_ROOT_COLL_CHECK_STR);
    addInxVal( &genQueryInp->sqlCondInp, COL_COLL_NAME, collQCond );

    if ( ( flags & RECUR_QUERY_FG ) != 0 ) {
        genAllInCollQCond( collection, collQCond );
        addInxVal( &genQueryInp->sqlCondInp, COL_COLL_NAME, collQCond );
    }
    else {
        snprintf( collQCond, MAX_NAME_LEN, "='%s'", collection );
        addInxVal( &genQueryInp->sqlCondInp, COL_COLL_PARENT_NAME, collQCond );
    }
    addInxIval( &genQueryInp->selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp->selectInp, COL_COLL_OWNER_NAME, 1 );
    addInxIval( &genQueryInp->selectInp, COL_COLL_CREATE_TIME, 1 );
    addInxIval( &genQueryInp->selectInp, COL_COLL_MODIFY_TIME, 1 );
    addInxIval( &genQueryInp->selectInp, COL_COLL_TYPE, 1 );
    addInxIval( &genQueryInp->selectInp, COL_COLL_INFO1, 1 );
    addInxIval( &genQueryInp->selectInp, COL_COLL_INFO2, 1 );

    genQueryInp->maxRows = MAX_SQL_ROWS;

    status = ( *queryHandle->genQuery )(
                 ( rcComm_t * ) queryHandle->conn, genQueryInp, genQueryOut );

    return status;
}

/* queryDataObjInColl - query the DataObj in a collection.
 */
int
queryDataObjInColl( queryHandle_t *queryHandle, char *collection,
                    int flags, genQueryInp_t *genQueryInp,
                    genQueryOut_t **genQueryOut, keyValPair_t *condInput ) {
    char collQCond[MAX_NAME_LEN];
    int status;
    char *rescName = nullptr;
    if ( collection == nullptr || genQueryOut == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( genQueryInp, 0, sizeof( genQueryInp_t ) );

    if ( ( flags & RECUR_QUERY_FG ) != 0 ) {
        genAllInCollQCond( collection, collQCond );
        addInxVal( &genQueryInp->sqlCondInp, COL_COLL_NAME, collQCond );
    }
    else {
        snprintf( collQCond, MAX_NAME_LEN, " = '%s'", collection );
        addInxVal( &genQueryInp->sqlCondInp, COL_COLL_NAME, collQCond );
    }
    if ( ( flags & INCLUDE_CONDINPUT_IN_QUERY ) != 0 &&
            condInput != nullptr &&
            ( rescName = getValByKey( condInput, RESC_NAME_KW ) ) != nullptr ) {
        snprintf( collQCond, MAX_NAME_LEN, " = '%s'", rescName );
        addInxVal( &genQueryInp->sqlCondInp, COL_D_RESC_NAME, collQCond );
    }

    setQueryInpForData( flags, genQueryInp );

    genQueryInp->maxRows = MAX_SQL_ROWS;
    genQueryInp->options = RETURN_TOTAL_ROW_COUNT;

    status = ( *queryHandle->genQuery )(
                 ( rcComm_t * ) queryHandle->conn, genQueryInp, genQueryOut );

    return status;

}

int
setQueryInpForData( int flags, genQueryInp_t *genQueryInp ) {

    if ( genQueryInp == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    addInxIval( &genQueryInp->selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp->selectInp, COL_DATA_NAME, 1 );
    addInxIval( &genQueryInp->selectInp, COL_D_DATA_ID, 1 );
    addInxIval( &genQueryInp->selectInp, COL_DATA_MODE, 1 );
    addInxIval( &genQueryInp->selectInp, COL_DATA_SIZE, 1 );
    addInxIval( &genQueryInp->selectInp, COL_D_MODIFY_TIME, 1 );
    addInxIval( &genQueryInp->selectInp, COL_D_CREATE_TIME, 1 );
    if ( ( flags & LONG_METADATA_FG ) != 0 ||
            ( flags & VERY_LONG_METADATA_FG ) != 0 ) {
        addInxIval( &genQueryInp->selectInp, COL_D_RESC_NAME, 1 );
        addInxIval( &genQueryInp->selectInp, COL_D_RESC_HIER, 1 );
        addInxIval( &genQueryInp->selectInp, COL_D_OWNER_NAME, 1 );
        addInxIval( &genQueryInp->selectInp, COL_DATA_REPL_NUM, 1 );
        addInxIval( &genQueryInp->selectInp, COL_D_REPL_STATUS, 1 );

        if ( ( flags & VERY_LONG_METADATA_FG ) != 0 ) {
            addInxIval( &genQueryInp->selectInp, COL_D_DATA_PATH, 1 );
            addInxIval( &genQueryInp->selectInp, COL_D_DATA_CHECKSUM, 1 );
            addInxIval( &genQueryInp->selectInp, COL_DATA_TYPE_NAME, 1 );
        }
    }

    return 0;
}

int
printTiming( rcComm_t *conn, char *objPath, rodsLong_t fileSize,
             char *localFile, struct timeval *startTime, struct timeval *endTime ) {
    struct timeval diffTime;
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    float transRate, sizeInMb, timeInSec;
    int status;


    if ( ( status = splitPathByKey( objPath, myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLogError( LOG_NOTICE, status,
                      "printTiming: splitPathByKey for %s error, status = %d",
                      objPath, status );
        return status;
    }

    diffTime.tv_sec = endTime->tv_sec - startTime->tv_sec;
    diffTime.tv_usec = endTime->tv_usec - startTime->tv_usec;

    if ( diffTime.tv_usec < 0 ) {
        diffTime.tv_sec --;
        diffTime.tv_usec += 1000000;
    }

    timeInSec = ( float ) diffTime.tv_sec + ( ( float ) diffTime.tv_usec /
                1000000.0 );

    if ( fileSize < 0 ) {
        /* may be we can find it from the local file */

        if ( localFile != nullptr ) {
            fileSize = getFileSize( localFile );
        }
    }


    if ( fileSize <= 0 ) {
        transRate = 0.0;
        sizeInMb = 0.0;
    }
    else {
        sizeInMb = ( float ) fileSize / 1048576.0;
        if ( timeInSec == 0.0 ) {
            transRate = 0.0;
        }
        else {
            transRate = sizeInMb / timeInSec;
        }
    }

    if ( fileSize < 0 ) {
        fprintf( stdout,
                 "   %-25.25s  %.3f sec\n",
                 myFile, timeInSec );
    }
    else {
        fprintf( stdout,
                 "   %-25.25s  %10.3f MB | %.3f sec | %d thr | %6.3f MB/s\n",
                 myFile, sizeInMb, timeInSec, conn->transStat.numThreads, transRate );
    }

    return 0;
}

int
printTime( char *objPath, struct timeval *startTime,
           struct timeval *endTime ) {
    struct timeval diffTime;
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    float timeInSec;
    int status;


    if ( ( status = splitPathByKey( objPath, myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLogError( LOG_NOTICE, status,
                      "printTime: splitPathByKey for %s error, status = %d",
                      objPath, status );
        return status;
    }

    diffTime.tv_sec = endTime->tv_sec - startTime->tv_sec;
    diffTime.tv_usec = endTime->tv_usec - startTime->tv_usec;

    if ( diffTime.tv_usec < 0 ) {
        diffTime.tv_sec --;
        diffTime.tv_usec += 1000000;
    }
    timeInSec = ( float ) diffTime.tv_sec + ( ( float ) diffTime.tv_usec /
                1000000.0 );

    fprintf( stdout, "   %-25.25s  %.3f sec\n", myFile, timeInSec );

    return 0;
}

int
printNoSync( char *objPath, rodsLong_t fileSize, char *reason ) {
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    float sizeInMb;
    int status;


    if ( ( status = splitPathByKey( objPath, myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rstrcpy( myFile, objPath, MAX_NAME_LEN );
    }

    if ( fileSize <= 0 ) {
        sizeInMb = 0.0;
    }
    else {
        sizeInMb = ( float ) fileSize / 1048576.0;
    }

    fprintf( stdout,
             "   %-25.25s  %10.3f MB --- %s no sync required \n", myFile, sizeInMb, reason );

    return 0;
}

int
queryDataObjAcl( rcComm_t *conn, char *dataId, char *zoneHint, genQueryOut_t **genQueryOut ) { // JMC - bacport 4516
    genQueryInp_t genQueryInp;
    int status;
    char tmpStr[MAX_NAME_LEN];

    if ( dataId == nullptr || genQueryOut == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    if ( zoneHint != nullptr ) { // JMC - bacport 4516
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneHint );
    }

    addInxIval( &genQueryInp.selectInp, COL_USER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_USER_ZONE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_ACCESS_NAME, 1 );

    snprintf( tmpStr, MAX_NAME_LEN, " = '%s'", dataId );

    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_ACCESS_DATA_ID, tmpStr );

    snprintf( tmpStr, MAX_NAME_LEN, "='%s'", "access_type" );

    /* Currently necessary since other namespaces exist in the token table */
    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_TOKEN_NAMESPACE, tmpStr );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rcGenQuery( conn, &genQueryInp, genQueryOut );

    return status;

}

int
queryCollAclSpecific( rcComm_t *conn, char *collName, char *zoneHint,
                      genQueryOut_t **genQueryOut ) {
    genQueryOut_t *myGenQueryOut;
    int status;
    specificQueryInp_t specificQueryInp;

    if ( collName == nullptr || genQueryOut == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    myGenQueryOut = *genQueryOut =
                        ( genQueryOut_t * ) malloc( sizeof( genQueryOut_t ) );
    memset( myGenQueryOut, 0, sizeof( genQueryOut_t ) );

    memset( &specificQueryInp, 0, sizeof( specificQueryInp_t ) );
    if ( zoneHint != nullptr ) {
        addKeyVal( &specificQueryInp.condInput, ZONE_KW, zoneHint );
    }

    specificQueryInp.maxRows = MAX_SQL_ROWS;
    specificQueryInp.continueInx = 0;
    specificQueryInp.sql = "ShowCollAcls";
    specificQueryInp.args[0] = collName;
    status = rcSpecificQuery( conn, &specificQueryInp, genQueryOut );
    return status;
}


int
queryCollAcl( rcComm_t *conn, char *collName, char *zoneHint, genQueryOut_t **genQueryOut ) { // JMC - bacport 4516
    genQueryInp_t genQueryInp;
    genQueryOut_t *myGenQueryOut;
    int status;
    char tmpStr[MAX_NAME_LEN];

    if ( collName == nullptr || genQueryOut == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    myGenQueryOut = *genQueryOut =
                        ( genQueryOut_t * ) malloc( sizeof( genQueryOut_t ) );
    memset( myGenQueryOut, 0, sizeof( genQueryOut_t ) );

    clearGenQueryInp( &genQueryInp );

    if ( zoneHint != nullptr ) { // JMC - bacport 4516
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneHint );
    }

    addInxIval( &genQueryInp.selectInp, COL_COLL_USER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_USER_ZONE, 1 );

    addInxIval( &genQueryInp.selectInp, COL_COLL_ACCESS_NAME, 1 );

    snprintf( tmpStr, MAX_NAME_LEN, "='%s'", "access_type" );

    /* Currently necessary since other namespaces exist in the token table */
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_TOKEN_NAMESPACE, tmpStr );

    snprintf( tmpStr, MAX_NAME_LEN, " = '%s'", collName );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, tmpStr );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rcGenQuery( conn, &genQueryInp, genQueryOut );

    return status;

}

int
queryCollInheritance( rcComm_t *conn, char *collName,
                      genQueryOut_t **genQueryOut ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *myGenQueryOut;
    int status;
    char tmpStr[MAX_NAME_LEN];

    if ( collName == nullptr || genQueryOut == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    myGenQueryOut = *genQueryOut =
                        ( genQueryOut_t * ) malloc( sizeof( genQueryOut_t ) );
    memset( myGenQueryOut, 0, sizeof( genQueryOut_t ) );

    clearGenQueryInp( &genQueryInp );

    addInxIval( &genQueryInp.selectInp, COL_COLL_INHERITANCE, 1 );

    snprintf( tmpStr, MAX_NAME_LEN, " = '%s'", collName );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, tmpStr );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rcGenQuery( conn, &genQueryInp, genQueryOut );

    return status;

}

int
genQueryOutToCollRes( genQueryOut_t **genQueryOut,
                      collSqlResult_t *collSqlResult ) {
    genQueryOut_t *myGenQueryOut;
    sqlResult_t *collName, *collType, *collInfo1, *collInfo2, *collOwner,
                *collCreateTime, *collModifyTime, *tmpSqlResult;

    if ( genQueryOut == nullptr || ( myGenQueryOut = *genQueryOut ) == nullptr ||
            collSqlResult == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    collSqlResult->rowCnt = myGenQueryOut->rowCnt;
    collSqlResult->attriCnt = myGenQueryOut->attriCnt;
    collSqlResult->continueInx = myGenQueryOut->continueInx;
    collSqlResult->totalRowCount = myGenQueryOut->totalRowCount;

    if ( ( collName = getSqlResultByInx( myGenQueryOut, COL_COLL_NAME ) ) == nullptr ) {
        rodsLog( LOG_ERROR,
                 "genQueryOutToCollRes: getSqlResultByInx for COL_COLL_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else {
        collSqlResult->collName = *collName;
    }

    if ( ( collType = getSqlResultByInx( myGenQueryOut, COL_COLL_TYPE ) ) == nullptr ) {
        /* should inherit parent's specColl */
        setSqlResultValue( &collSqlResult->collType, COL_COLL_NAME,
                           INHERIT_PAR_SPEC_COLL_STR, myGenQueryOut->rowCnt );
        setSqlResultValue( &collSqlResult->collInfo1, COL_COLL_INFO1, "",
                           myGenQueryOut->rowCnt );
        setSqlResultValue( &collSqlResult->collInfo2, COL_COLL_INFO2, "",
                           myGenQueryOut->rowCnt );
        setSqlResultValue( &collSqlResult->collOwner, COL_COLL_OWNER_NAME, "",
                           myGenQueryOut->rowCnt );
        setSqlResultValue( &collSqlResult->collCreateTime, COL_COLL_CREATE_TIME,
                           "", myGenQueryOut->rowCnt );
        setSqlResultValue( &collSqlResult->collModifyTime, COL_COLL_MODIFY_TIME,
                           "", myGenQueryOut->rowCnt );
        /* myGenQueryOut could came from rcQuerySpecColl call */
        if ( ( tmpSqlResult = getSqlResultByInx( myGenQueryOut, COL_DATA_NAME ) )
                != nullptr ) {
            if ( tmpSqlResult->value != nullptr ) {
                free( tmpSqlResult->value );
            }
        }
        if ( ( tmpSqlResult = getSqlResultByInx( myGenQueryOut, COL_D_CREATE_TIME ) )
                != nullptr ) {
            if ( tmpSqlResult->value != nullptr ) {
                free( tmpSqlResult->value );
            }
        }
        if ( ( tmpSqlResult = getSqlResultByInx( myGenQueryOut, COL_D_MODIFY_TIME ) )
                != nullptr ) {
            if ( tmpSqlResult->value != nullptr ) {
                free( tmpSqlResult->value );
            }
        }
        if ( ( tmpSqlResult = getSqlResultByInx( myGenQueryOut, COL_DATA_SIZE ) )
                != nullptr ) {
            if ( tmpSqlResult->value != nullptr ) {
                free( tmpSqlResult->value );
            }
        }
    }
    else {
        collSqlResult->collType = *collType;
        if ( ( collInfo1 = getSqlResultByInx( myGenQueryOut, COL_COLL_INFO1 ) ) ==
                nullptr ) {
            rodsLog( LOG_ERROR,
                     "genQueryOutToCollRes: getSqlResultByInx COL_COLL_INFO1 failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            collSqlResult->collInfo1 = *collInfo1;
        }
        if ( ( collInfo2 = getSqlResultByInx( myGenQueryOut, COL_COLL_INFO2 ) ) ==
                nullptr ) {
            rodsLog( LOG_ERROR,
                     "genQueryOutToCollRes: getSqlResultByInx COL_COLL_INFO2 failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            collSqlResult->collInfo2 = *collInfo2;
        }
        if ( ( collOwner = getSqlResultByInx( myGenQueryOut,
                                              COL_COLL_OWNER_NAME ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "genQueryOutToCollRes: getSqlResultByInx COL_COLL_OWNER_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            collSqlResult->collOwner = *collOwner;
        }
        if ( ( collCreateTime = getSqlResultByInx( myGenQueryOut,
                                COL_COLL_CREATE_TIME ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "genQueryOutToCollRes: getSqlResultByInx COL_COLL_CREATE_TIME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            collSqlResult->collCreateTime = *collCreateTime;
        }
        if ( ( collModifyTime = getSqlResultByInx( myGenQueryOut,
                                COL_COLL_MODIFY_TIME ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "genQueryOutToCollRes: getSqlResultByInx COL_COLL_MODIFY_TIME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            collSqlResult->collModifyTime = *collModifyTime;
        }
    }

    free( *genQueryOut );
    *genQueryOut = nullptr;
    return 0;
}

int
setSqlResultValue( sqlResult_t *sqlResult, int attriInx, char *valueStr,
                   int rowCnt ) {
    if ( sqlResult == nullptr || rowCnt <= 0 ) {
        return 0;
    }

    sqlResult->attriInx = attriInx;
    if ( valueStr == nullptr ) {
        sqlResult->len = 1;
    }
    else {
        sqlResult->len = strlen( valueStr ) + 1;
    }
    if ( sqlResult->len == 1 ) {
        sqlResult->value = ( char * )malloc( rowCnt );
        memset( sqlResult->value, 0, rowCnt );
    }
    else {
        int i;
        char *tmpPtr;
        tmpPtr = sqlResult->value = ( char * )malloc( rowCnt * sqlResult->len );
        for ( i = 0; i < rowCnt; i++ ) {
            rstrcpy( tmpPtr, valueStr, sqlResult->len );
            tmpPtr += sqlResult->len;
        }
    }
    return 0;
}

int
clearCollSqlResult( collSqlResult_t *collSqlResult ) {
    if ( collSqlResult == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( collSqlResult->collName.value != nullptr ) {
        free( collSqlResult->collName.value );
    }
    if ( collSqlResult->collType.value != nullptr ) {
        free( collSqlResult->collType.value );
    }
    if ( collSqlResult->collInfo1.value != nullptr ) {
        free( collSqlResult->collInfo1.value );
    }
    if ( collSqlResult->collInfo2.value != nullptr ) {
        free( collSqlResult->collInfo2.value );
    }
    if ( collSqlResult->collOwner.value != nullptr ) {
        free( collSqlResult->collOwner.value );
    }
    if ( collSqlResult->collCreateTime.value != nullptr ) {
        free( collSqlResult->collCreateTime.value );
    }
    if ( collSqlResult->collModifyTime.value != nullptr ) {
        free( collSqlResult->collModifyTime.value );
    }

    memset( collSqlResult, 0, sizeof( collSqlResult_t ) );

    return 0;
}


int
clearDataObjSqlResult( dataObjSqlResult_t *dataObjSqlResult ) {
    if ( dataObjSqlResult == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( dataObjSqlResult->collName.value != nullptr ) {
        free( dataObjSqlResult->collName.value );
    }
    if ( dataObjSqlResult->dataName.value != nullptr ) {
        free( dataObjSqlResult->dataName.value );
    }
    if ( dataObjSqlResult->dataMode.value != nullptr ) {
        free( dataObjSqlResult->dataMode.value );
    }
    if ( dataObjSqlResult->dataSize.value != nullptr ) {
        free( dataObjSqlResult->dataSize.value );
    }
    if ( dataObjSqlResult->createTime.value != nullptr ) {
        free( dataObjSqlResult->createTime.value );
    }
    if ( dataObjSqlResult->modifyTime.value != nullptr ) {
        free( dataObjSqlResult->modifyTime.value );
    }
    if ( dataObjSqlResult->chksum.value != nullptr ) {
        free( dataObjSqlResult->chksum.value );
    }
    if ( dataObjSqlResult->replStatus.value != nullptr ) {
        free( dataObjSqlResult->replStatus.value );
    }
    if ( dataObjSqlResult->dataId.value != nullptr ) {
        free( dataObjSqlResult->dataId.value );
    }
    if ( dataObjSqlResult->resource.value != nullptr ) {
        free( dataObjSqlResult->resource.value );
    }
    if ( dataObjSqlResult->resc_id.value != nullptr ) {
        free( dataObjSqlResult->resc_id.value );
    }
    if ( dataObjSqlResult->resc_hier.value != nullptr ) {
        free( dataObjSqlResult->resc_hier.value );
    }
    if ( dataObjSqlResult->phyPath.value != nullptr ) {
        free( dataObjSqlResult->phyPath.value );
    }
    if ( dataObjSqlResult->ownerName.value != nullptr ) {
        free( dataObjSqlResult->ownerName.value );
    }
    if ( dataObjSqlResult->replNum.value != nullptr ) {
        free( dataObjSqlResult->replNum.value );
    }
    if ( dataObjSqlResult->dataType.value != nullptr ) {
	    free( dataObjSqlResult->dataType.value );
    }
    memset( dataObjSqlResult, 0, sizeof( dataObjSqlResult_t ) );

    return 0;
}

int
genQueryOutToDataObjRes( genQueryOut_t **genQueryOut,
                         dataObjSqlResult_t *dataObjSqlResult ) {
    genQueryOut_t *myGenQueryOut;
    sqlResult_t *collName, *dataName, *dataSize, *dataMode, *createTime,
                *modifyTime, *chksum, *replStatus, *dataId, *resource, *resc_hier, *phyPath,
                *ownerName, *replNum, *dataType;
    if ( genQueryOut == nullptr || ( myGenQueryOut = *genQueryOut ) == nullptr ||
            dataObjSqlResult == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    dataObjSqlResult->rowCnt = myGenQueryOut->rowCnt;
    dataObjSqlResult->attriCnt = myGenQueryOut->attriCnt;
    dataObjSqlResult->continueInx = myGenQueryOut->continueInx;
    dataObjSqlResult->totalRowCount = myGenQueryOut->totalRowCount;

    if ( ( collName = getSqlResultByInx( myGenQueryOut, COL_COLL_NAME ) ) == nullptr ) {
        rodsLog( LOG_ERROR,
                 "genQueryOutToDataObjRes: getSqlResultByInx for COL_COLL_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else {
        dataObjSqlResult->collName = *collName;
    }

    if ( ( dataName = getSqlResultByInx( myGenQueryOut, COL_DATA_NAME ) ) == nullptr ) {
        rodsLog( LOG_ERROR,
                 "genQueryOutToDataObjRes: getSqlResultByInx for COL_DATA_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else {
        dataObjSqlResult->dataName = *dataName;
    }

    if ( ( dataMode = getSqlResultByInx( myGenQueryOut, COL_DATA_MODE ) ) == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->dataMode, COL_DATA_MODE, "",
                           myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->dataMode = *dataMode;
    }

    if ( ( dataSize = getSqlResultByInx( myGenQueryOut, COL_DATA_SIZE ) ) == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->dataSize, COL_DATA_SIZE, "-1",
                           myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->dataSize = *dataSize;
    }

    if ( ( createTime = getSqlResultByInx( myGenQueryOut, COL_D_CREATE_TIME ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->createTime, COL_D_CREATE_TIME,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->createTime = *createTime;
    }

    if ( ( modifyTime = getSqlResultByInx( myGenQueryOut, COL_D_MODIFY_TIME ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->modifyTime, COL_D_MODIFY_TIME,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->modifyTime = *modifyTime;
    }

    if ( ( dataId = getSqlResultByInx( myGenQueryOut, COL_D_DATA_ID ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->dataId, COL_D_DATA_ID,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->dataId = *dataId;
    }

    if ( ( chksum = getSqlResultByInx( myGenQueryOut, COL_D_DATA_CHECKSUM ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->chksum, COL_D_DATA_CHECKSUM,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->chksum = *chksum;
    }

    if ( ( replStatus = getSqlResultByInx( myGenQueryOut, COL_D_REPL_STATUS ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->replStatus, COL_D_REPL_STATUS,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->replStatus = *replStatus;
    }

    if ( ( resource = getSqlResultByInx( myGenQueryOut, COL_D_RESC_NAME ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->resource, COL_D_RESC_NAME,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->resource = *resource;
    }
    if ( ( resc_hier = getSqlResultByInx( myGenQueryOut, COL_D_RESC_HIER ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->resc_hier, COL_D_RESC_HIER,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->resc_hier = *resc_hier;
    }


    if ( ( dataType = getSqlResultByInx( myGenQueryOut, COL_DATA_TYPE_NAME ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->dataType, COL_DATA_TYPE_NAME,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->dataType = *dataType;
    }

    if ( ( phyPath = getSqlResultByInx( myGenQueryOut, COL_D_DATA_PATH ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->phyPath, COL_D_DATA_PATH,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->phyPath = *phyPath;
    }

    if ( ( ownerName = getSqlResultByInx( myGenQueryOut, COL_D_OWNER_NAME ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->ownerName, COL_D_OWNER_NAME,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->ownerName = *ownerName;
    }

    if ( ( replNum = getSqlResultByInx( myGenQueryOut, COL_DATA_REPL_NUM ) )
            == nullptr ) {
        setSqlResultValue( &dataObjSqlResult->replNum, COL_DATA_REPL_NUM,
                           "", myGenQueryOut->rowCnt );
    }
    else {
        dataObjSqlResult->replNum = *replNum;
    }

    free( *genQueryOut );
    *genQueryOut = nullptr;

    return 0;
}

int
rclOpenCollection( rcComm_t *conn, char *collection, int flags,
                   collHandle_t *collHandle ) {
    rodsObjStat_t *rodsObjStatOut = nullptr;
    int status;

    if ( conn == nullptr || collection == nullptr || collHandle == nullptr ) {
        rodsLog( LOG_ERROR,
                 "rclOpenCollection: NULL conn, collection or collHandle input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( ( flags & INCLUDE_CONDINPUT_IN_QUERY ) == 0 ) {
        /* preserve collHandle->>dataObjInp.condInput if != 0 */
        memset( collHandle, 0, sizeof( collHandle_t ) );
    }
    rstrcpy( collHandle->dataObjInp.objPath, collection, MAX_NAME_LEN );
    status = rcObjStat( conn, &collHandle->dataObjInp, &rodsObjStatOut );


    if ( status < 0 ) {
        return status;
    }

    if ( rodsObjStatOut->objType != COLL_OBJ_T ) {
        free( rodsObjStatOut );
        return CAT_UNKNOWN_COLLECTION;
    }

    replSpecColl( rodsObjStatOut->specColl, &collHandle->dataObjInp.specColl );
    if ( rodsObjStatOut->specColl != nullptr &&
            rodsObjStatOut->specColl->collClass != STRUCT_FILE_COLL &&
            strlen( rodsObjStatOut->specColl->objPath ) > 0 ) {
        /* save the linked path */
        rstrcpy( collHandle->linkedObjPath, rodsObjStatOut->specColl->objPath,
                 MAX_NAME_LEN );
    };

    collHandle->rodsObjStat = rodsObjStatOut;

    collHandle->state = COLL_OPENED;
    collHandle->flags = flags;
    /* the collection exist. now query the data in it */
    status = rclInitQueryHandle( &collHandle->queryHandle, conn );
    if ( status < 0 ) {
        return status;
    }

    return 0;
}

int
rclReadCollection( rcComm_t *conn, collHandle_t *collHandle,
                   collEnt_t *collEnt ) {
    int status;

    collHandle->queryHandle.conn = conn;        /* in case it changed */
    status = readCollection( collHandle, collEnt );

    return status;
}

int
readCollection( collHandle_t *collHandle, collEnt_t *collEnt ) {
    int status = 0;
    int savedStatus = 0;
    collMetaInfo_t collMetaInfo;
    dataObjMetaInfo_t dataObjMetaInfo;
    queryHandle_t *queryHandle = &collHandle->queryHandle;

    if ( queryHandle == nullptr || collHandle == nullptr || collEnt == nullptr ) {
        rodsLog( LOG_ERROR,
                 "rclReadCollection: NULL queryHandle or collHandle input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( collEnt, 0, sizeof( collEnt_t ) );

    if ( collHandle->state == COLL_CLOSED ) {
        return CAT_NO_ROWS_FOUND;
    }

    if ( ( collHandle->flags & DATA_QUERY_FIRST_FG ) == 0 ) {
        /* recursive - coll first, dataObj second */
        if ( collHandle->state == COLL_OPENED ) {
            status = genCollResInColl( queryHandle, collHandle );
            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                rodsLog( LOG_ERROR, "genCollResInColl in readCollection failed with status %d", status );
            }
        }

        if ( collHandle->state == COLL_COLL_OBJ_QUERIED ) {
            memset( &collMetaInfo, 0, sizeof( collMetaInfo ) );
            status = getNextCollMetaInfo( collHandle, collEnt );
            if ( status >= 0 ) {
                return status;
            }
            else {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_ERROR,
                             "rclReadCollection: getNextCollMetaInfo error for %s. status = %d",
                             collHandle->dataObjInp.objPath, status );
                }
                if ( collHandle->dataObjInp.specColl == nullptr ) {
                    clearGenQueryInp( &collHandle->genQueryInp );
                }
            }
            status = genDataResInColl( queryHandle, collHandle );
            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                rodsLog( LOG_ERROR, "genDataResInColl in readCollection failed with status %d", status );
            }
        }
        if ( collHandle->state == COLL_DATA_OBJ_QUERIED ) {
            memset( &dataObjMetaInfo, 0, sizeof( dataObjMetaInfo ) );
            status = getNextDataObjMetaInfo( collHandle, collEnt );

            if ( status >= 0 ) {
                return status;
            }
            else {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_ERROR,
                             "rclReadCollection: getNextDataObjMetaInfo error for %s. status = %d",
                             collHandle->dataObjInp.objPath, status );
                }
                /* cleanup */
                if ( collHandle->dataObjInp.specColl == nullptr ) {
                    clearGenQueryInp( &collHandle->genQueryInp );
                }
                /* Nothing else to do. cleanup */
                collHandle->state = COLL_CLOSED;
            }
            return status;
        }
    }
    else {
        if ( collHandle->state == COLL_OPENED ) {
            status = genDataResInColl( queryHandle, collHandle );
            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                savedStatus = status;
            }

        }

        if ( collHandle->state == COLL_DATA_OBJ_QUERIED ) {
            memset( &dataObjMetaInfo, 0, sizeof( dataObjMetaInfo ) );
            status = getNextDataObjMetaInfo( collHandle, collEnt );

            if ( status >= 0 ) {
                return status;
            }
            else {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_ERROR,
                             "rclReadCollection: getNextDataObjMetaInfo error for %s. status = %d",
                             collHandle->dataObjInp.objPath, status );
                }
                /* cleanup */
                if ( collHandle->dataObjInp.specColl == nullptr ) {
                    clearGenQueryInp( &collHandle->genQueryInp );
                }
            }

            status = genCollResInColl( queryHandle, collHandle );
            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                savedStatus = status;
            }

        }

        if ( collHandle->state == COLL_COLL_OBJ_QUERIED ) {
            memset( &collMetaInfo, 0, sizeof( collMetaInfo ) );
            status = getNextCollMetaInfo( collHandle, collEnt );
            if ( status < 0 ) {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_ERROR,
                             "rclReadCollection: getNextCollMetaInfo error for %s. status = %d",
                             collHandle->dataObjInp.objPath, status );
                }
                /* cleanup */
                if ( collHandle->dataObjInp.specColl == nullptr ) {
                    clearGenQueryInp( &collHandle->genQueryInp );
                }
                /* Nothing else to do. cleanup */
                collHandle->state = COLL_CLOSED;
            }

            if ( savedStatus < 0 ) {
                return savedStatus;
            }
            else {
                return status;
            }

        }
    }
    return CAT_NO_ROWS_FOUND;
}


int
genCollResInColl( queryHandle_t *queryHandle, collHandle_t *collHandle ) {
    genQueryOut_t *genQueryOut = nullptr;
    int status = 0;

    /* query for sub-collections */
    if ( collHandle->dataObjInp.specColl != nullptr ) {
        if ( collHandle->dataObjInp.specColl->collClass == LINKED_COLL ) {
            memset( &collHandle->genQueryInp, 0, sizeof( genQueryInp_t ) );
            status = queryCollInColl( queryHandle,
                                      collHandle->linkedObjPath, collHandle->flags & ( ~RECUR_QUERY_FG ),
                                      &collHandle->genQueryInp, &genQueryOut );
        }
        else {
            if ( strlen( collHandle->linkedObjPath ) > 0 ) {
                rstrcpy( collHandle->dataObjInp.objPath,
                         collHandle->linkedObjPath, MAX_NAME_LEN );
            }
            addKeyVal( &collHandle->dataObjInp.condInput,
                       SEL_OBJ_TYPE_KW, "collection" );
            collHandle->dataObjInp.openFlags = 0;    /* start over */
            status = ( *queryHandle->querySpecColl )(
                         ( rcComm_t * ) queryHandle->conn, &collHandle->dataObjInp,
                         &genQueryOut );
        }
    }
    else {
        memset( &collHandle->genQueryInp, 0, sizeof( genQueryInp_t ) );
        status = queryCollInColl( queryHandle,
                                  collHandle->dataObjInp.objPath, collHandle->flags,
                                  &collHandle->genQueryInp, &genQueryOut );
    }

    collHandle->rowInx = 0;
    collHandle->state = COLL_COLL_OBJ_QUERIED;
    if ( status >= 0 ) {
        status = genQueryOutToCollRes( &genQueryOut,
                                       &collHandle->collSqlResult );
    }
    else if ( status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_ERROR,
                 "genCollResInColl: query collection error for %s. status = %d",
                 collHandle->dataObjInp.objPath, status );
    }
    freeGenQueryOut( &genQueryOut );
    return status;
}

int
genDataResInColl( queryHandle_t *queryHandle, collHandle_t *collHandle ) {
    genQueryOut_t *genQueryOut = nullptr;
    int status = 0;

    if ( collHandle->dataObjInp.specColl != nullptr ) {
        if ( collHandle->dataObjInp.specColl->collClass == LINKED_COLL ) {
            memset( &collHandle->genQueryInp, 0, sizeof( genQueryInp_t ) );
            status = queryDataObjInColl( queryHandle,
                                         collHandle->linkedObjPath, collHandle->flags & ( ~RECUR_QUERY_FG ),
                                         &collHandle->genQueryInp, &genQueryOut,
                                         &collHandle->dataObjInp.condInput );
        }
        else {
            if ( strlen( collHandle->linkedObjPath ) > 0 ) {
                rstrcpy( collHandle->dataObjInp.objPath,
                         collHandle->linkedObjPath, MAX_NAME_LEN );
            }
            addKeyVal( &collHandle->dataObjInp.condInput,
                       SEL_OBJ_TYPE_KW, "dataObj" );
            collHandle->dataObjInp.openFlags = 0;    /* start over */
            status = ( *queryHandle->querySpecColl )
                     ( ( rcComm_t * ) queryHandle->conn,
                       &collHandle->dataObjInp, &genQueryOut );
        }
    }
    else {
        memset( &collHandle->genQueryInp, 0, sizeof( genQueryInp_t ) );
        status = queryDataObjInColl( queryHandle,
                                     collHandle->dataObjInp.objPath, collHandle->flags,
                                     &collHandle->genQueryInp, &genQueryOut,
                                     &collHandle->dataObjInp.condInput );
    }

    collHandle->rowInx = 0;
    collHandle->state = COLL_DATA_OBJ_QUERIED;
    if ( status >= 0 ) {
        status = genQueryOutToDataObjRes( &genQueryOut,
                                          &collHandle->dataObjSqlResult );
    }
    else if ( status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_ERROR,
                 "genDataResInColl: query dataObj error for %s. status = %d",
                 collHandle->dataObjInp.objPath, status );
    }
    freeGenQueryOut( &genQueryOut );
    return status;
}

int
rclCloseCollection( collHandle_t *collHandle ) {
    return clearCollHandle( collHandle, 1 );
}

int
clearCollHandle( collHandle_t *collHandle, int freeSpecColl ) {
    if ( collHandle == nullptr ) {
        return 0;
    }
    if ( collHandle->dataObjInp.specColl == nullptr ) {
        clearGenQueryInp( &collHandle->genQueryInp );
    }
    if ( freeSpecColl != 0 && collHandle->dataObjInp.specColl != nullptr ) {
        free( collHandle->dataObjInp.specColl );
    }
    if ( collHandle->rodsObjStat != nullptr ) {
        freeRodsObjStat( collHandle->rodsObjStat );
        collHandle->rodsObjStat = nullptr;
    }
    clearKeyVal( &collHandle->dataObjInp.condInput );
    memset( &collHandle->dataObjInp, 0,  sizeof( dataObjInp_t ) );

    clearDataObjSqlResult( &collHandle->dataObjSqlResult );
    clearCollSqlResult( &collHandle->collSqlResult );

    collHandle->state = COLL_CLOSED;
    collHandle->rowInx = 0;

    return 0;
}

int
getNextCollMetaInfo( collHandle_t *collHandle, collEnt_t *outCollEnt ) {
    char *value;
    int len;
    char *collType, *collInfo1, *collInfo2;
    int status = 0;
    queryHandle_t *queryHandle = &collHandle->queryHandle;
    dataObjInp_t *dataObjInp =  &collHandle->dataObjInp;
    genQueryInp_t *genQueryInp = &collHandle->genQueryInp;
    collSqlResult_t *collSqlResult = &collHandle->collSqlResult;

    if ( outCollEnt == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( outCollEnt, 0, sizeof( collEnt_t ) );

    outCollEnt->objType = COLL_OBJ_T;
    if ( collHandle->rowInx >= collSqlResult->rowCnt ) {
        genQueryOut_t *genQueryOut = nullptr;
        int continueInx = collSqlResult->continueInx;
        clearCollSqlResult( collSqlResult );

        if ( continueInx > 0 ) {
            /* More to come */

            if ( dataObjInp->specColl != nullptr ) {
                dataObjInp->openFlags = continueInx;
                status = ( *queryHandle->querySpecColl )(
                             ( rcComm_t * ) queryHandle->conn, dataObjInp, &genQueryOut );
            }
            else {
                genQueryInp->continueInx = continueInx;
                status = ( *queryHandle->genQuery )(
                             ( rcComm_t * ) queryHandle->conn, genQueryInp, &genQueryOut );
            }
            if ( status < 0 ) {
                collHandle->rowInx = 0;
				genQueryInp->continueInx = 0;
				collSqlResult->continueInx = 0;
				freeGenQueryOut( &genQueryOut );
                return status;
            }
            else {
                status = genQueryOutToCollRes( &genQueryOut, collSqlResult );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "genQueryOutToCollRes in getNextCollMetaInfo failed with status %d", status );
                }
                collHandle->rowInx = 0;
                free( genQueryOut );
            }
        }
        else {
            return CAT_NO_ROWS_FOUND;
        }
    }
    value = collSqlResult->collName.value;
    len = collSqlResult->collName.len;
    outCollEnt->collName = &value[len * ( collHandle->rowInx )];

    value = collSqlResult->collOwner.value;
    len = collSqlResult->collOwner.len;
    outCollEnt->ownerName = &value[len * ( collHandle->rowInx )];

    value = collSqlResult->collCreateTime.value;
    len = collSqlResult->collCreateTime.len;
    outCollEnt->createTime = &value[len * ( collHandle->rowInx )];

    value = collSqlResult->collModifyTime.value;
    len = collSqlResult->collModifyTime.len;
    outCollEnt->modifyTime = &value[len * ( collHandle->rowInx )];

    value = collSqlResult->collType.value;
    len = collSqlResult->collType.len;
    collType = &value[len * ( collHandle->rowInx )];

    if ( *collType != '\0' ) {
        value = collSqlResult->collInfo1.value;
        len = collSqlResult->collInfo1.len;
        collInfo1 = &value[len * ( collHandle->rowInx )];

        value = collSqlResult->collInfo2.value;
        len = collSqlResult->collInfo2.len;
        collInfo2 = &value[len * ( collHandle->rowInx )];

        if ( strcmp( collType, INHERIT_PAR_SPEC_COLL_STR ) == 0 ) {
            if ( dataObjInp->specColl == nullptr ) {
                rodsLog( LOG_ERROR,
                         "getNextCollMetaInfo: parent specColl is NULL for %s",
                         outCollEnt->collName );
                outCollEnt->specColl.collClass = NO_SPEC_COLL;
            }
            else {
                outCollEnt->specColl = *dataObjInp->specColl;
            }
            status = 0;
        }
        else {
            status = resolveSpecCollType( collType, outCollEnt->collName,
                                          collInfo1, collInfo2, &outCollEnt->specColl );
        }
    }
    else {
        outCollEnt->specColl.collClass = NO_SPEC_COLL;
        status = 0;
    }
    ( collHandle->rowInx ) ++;
    return status;
}

int get_resc_hier_from_leaf_id(
    queryHandle_t* _query_handle,
    rodsLong_t     _resc_id,
    char*          _resc_hier ) {
    if( !_query_handle ) {
        rodsLog(
           LOG_ERROR,
          "null query_handle ptr");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if( !_resc_hier ) {
        rodsLog(
           LOG_ERROR,
          "null resc_hier ptr");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if( _resc_id <= 0 ) {
        rodsLog(
           LOG_ERROR,
          "resc_id is invalid");
        return SYS_INVALID_INPUT_PARAM;
    }

    get_hier_inp_t  inp;
    inp.resc_id_ = _resc_id;

    get_hier_out_t* out = nullptr;

    // due to client-server blending of query handles and
    // point of call.  sometimes its on the server, sometimes
    // it is called from the client.
    int status = _query_handle->getHierForId(
                     _query_handle->conn,
                     &inp,
                     &out );
    if( status < 0 ) {
        return status;
    }

    if(out) {
        rstrcpy( _resc_hier, out->hier_, MAX_NAME_LEN );
    }
    else {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    return 0;
} // get_resc_hier_from_leaf_id

int
getNextDataObjMetaInfo( collHandle_t *collHandle, collEnt_t *outCollEnt ) {
    int status;
    char *value;
    int len;
    char *replStatus, *dataId;
    int dataIdLen, replStatusLen;
    queryHandle_t *queryHandle = &collHandle->queryHandle;
    dataObjInp_t *dataObjInp =  &collHandle->dataObjInp;
    genQueryInp_t *genQueryInp = &collHandle->genQueryInp;
    dataObjSqlResult_t *dataObjSqlResult = &collHandle->dataObjSqlResult;
    rodsObjStat_t *rodsObjStat = collHandle->rodsObjStat;
    char *prevdataId;
    int selectedInx = -1;

    if ( outCollEnt == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }
    prevdataId = collHandle->prevdataId;
    memset( outCollEnt, 0, sizeof( collEnt_t ) );
    outCollEnt->objType = DATA_OBJ_T;
    if ( collHandle->rowInx >= dataObjSqlResult->rowCnt ) {
        genQueryOut_t *genQueryOut = nullptr;
        int continueInx = dataObjSqlResult->continueInx;
        clearDataObjSqlResult( dataObjSqlResult );

        if ( continueInx > 0 ) {
            /* More to come */

            if ( dataObjInp->specColl != nullptr ) {
                dataObjInp->openFlags = continueInx;
                status = ( *queryHandle->querySpecColl )(
                             ( rcComm_t * ) queryHandle->conn, dataObjInp, &genQueryOut );
            }
            else {
                genQueryInp->continueInx = continueInx;
                status = ( *queryHandle->genQuery )(
                             ( rcComm_t * ) queryHandle->conn, genQueryInp, &genQueryOut );
            }
            if ( status < 0 ) {
                collHandle->rowInx = 0;
				genQueryInp->continueInx = 0;
				dataObjSqlResult->continueInx = 0;
                freeGenQueryOut( &genQueryOut );
                return status;
            }
            else {
                status = genQueryOutToDataObjRes( &genQueryOut,
                                                  dataObjSqlResult );
                if ( status < 0 ) {
                    rodsLogError( LOG_ERROR, status, "genQueryOut failed in %s.", __PRETTY_FUNCTION__ );
                }
                collHandle->rowInx = 0;
                free( genQueryOut );
            }
        }
        else {
            return CAT_NO_ROWS_FOUND;
        }
    }

    dataId = dataObjSqlResult->dataId.value;
    dataIdLen = dataObjSqlResult->dataId.len;
    replStatus = dataObjSqlResult->replStatus.value;
    replStatusLen = dataObjSqlResult->replStatus.len;

    if ( strlen( dataId ) > 0 && ( collHandle->flags & NO_TRIM_REPL_FG ) == 0 ) {
        int i;
        int gotCopy = 0;

        /* rsync type query ask for dataId. Others don't. Need to
         * screen out dup copies */

        for ( i = collHandle->rowInx; i < dataObjSqlResult->rowCnt; i++ ) {
            if ( selectedInx < 0 ) {
                /* nothing selected yet. pick this if different */
                if ( strcmp( prevdataId, &dataId[dataIdLen * i] ) != 0 ) {
                    rstrcpy( prevdataId, &dataId[dataIdLen * i], NAME_LEN );
                    selectedInx = i;
                    if ( atoi( &dataId[dataIdLen * i] ) != 0 ) {
                        gotCopy = 1;
                    }
                }
            }
            else {
                /* skip i to the next object */
                if ( strcmp( prevdataId, &dataId[dataIdLen * i] ) != 0 ) {
                    break;
                }
                if ( gotCopy == 0 &&
                        atoi( &replStatus[replStatusLen * i] ) > 0 ) {
                    /* pick a good copy */
                    selectedInx = i;
                    gotCopy = 1;
                }
            }
        }
        if ( selectedInx < 0 ) {
            return CAT_NO_ROWS_FOUND;
        }

        collHandle->rowInx = i;
    }
    else {
        selectedInx = collHandle->rowInx;
        collHandle->rowInx++;
    }

    value = dataObjSqlResult->collName.value;
    len = dataObjSqlResult->collName.len;
    outCollEnt->collName = &value[len * selectedInx];

    value = dataObjSqlResult->dataName.value;
    len = dataObjSqlResult->dataName.len;
    outCollEnt->dataName = &value[len * selectedInx];

    value = dataObjSqlResult->dataMode.value;
    len = dataObjSqlResult->dataMode.len;
    outCollEnt->dataMode = atoi( &value[len * selectedInx] );

    value = dataObjSqlResult->dataSize.value;
    len = dataObjSqlResult->dataSize.len;
    outCollEnt->dataSize = strtoll( &value[len * selectedInx], nullptr, 0 );

    value = dataObjSqlResult->createTime.value;
    len = dataObjSqlResult->createTime.len;
    outCollEnt->createTime = &value[len * selectedInx];

    value = dataObjSqlResult->modifyTime.value;
    len = dataObjSqlResult->modifyTime.len;
    outCollEnt->modifyTime = &value[len * selectedInx];

    outCollEnt->dataId = &dataId[dataIdLen * selectedInx];

    outCollEnt->replStatus = atoi( &replStatus[replStatusLen *
                                   selectedInx] );

    value = dataObjSqlResult->replNum.value;
    len = dataObjSqlResult->replNum.len;
    outCollEnt->replNum = atoi( &value[len * selectedInx] );

    value = dataObjSqlResult->chksum.value;
    len = dataObjSqlResult->chksum.len;
    outCollEnt->chksum = &value[len * selectedInx];

    value = dataObjSqlResult->dataType.value;
    len = dataObjSqlResult->dataType.len;
    outCollEnt->dataType = &value[len * selectedInx];

    if ( rodsObjStat->specColl != nullptr ) {
        outCollEnt->specColl = *rodsObjStat->specColl;
    }
    if ( rodsObjStat->specColl != nullptr &&
            rodsObjStat->specColl->collClass != LINKED_COLL ) {
        outCollEnt->resource = rodsObjStat->specColl->resource;
        outCollEnt->ownerName = rodsObjStat->ownerName;
        outCollEnt->replStatus = NEWLY_CREATED_COPY;
        outCollEnt->resc_hier = rodsObjStat->specColl->rescHier;
    }
    else {
        value = dataObjSqlResult->resource.value;
        len = dataObjSqlResult->resource.len;
        outCollEnt->resource = &value[len * selectedInx];

        value = dataObjSqlResult->resc_hier.value;
        len = dataObjSqlResult->resc_hier.len;
        outCollEnt->resc_hier = &value[len * selectedInx];

        value = dataObjSqlResult->ownerName.value;
        len = dataObjSqlResult->ownerName.len;
        outCollEnt->ownerName = &value[len * selectedInx];
    }

    value = dataObjSqlResult->phyPath.value;
    len = dataObjSqlResult->phyPath.len;
    outCollEnt->phyPath = &value[len * selectedInx];

    return 0;
}

int
setQueryFlag( rodsArguments_t *rodsArgs ) {
    int queryFlags;

    if ( rodsArgs->veryLongOption == True ) {
        queryFlags = VERY_LONG_METADATA_FG;
    }
    else if ( rodsArgs->longOption == True ) {
        queryFlags = LONG_METADATA_FG;
    }
    else {
        queryFlags = 0;
    }

    return queryFlags;
}

int
rclInitQueryHandle( queryHandle_t *queryHandle, rcComm_t *conn ) {
    if ( queryHandle == nullptr || conn == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    queryHandle->conn = conn;
    queryHandle->connType = RC_COMM;
    queryHandle->querySpecColl = ( funcPtr ) rcQuerySpecColl;
    queryHandle->genQuery = ( funcPtr ) rcGenQuery;
    queryHandle->getHierForId = ( funcPtr ) rcGetHierFromLeafId;

    return 0;
}

int
freeCollEnt( collEnt_t *collEnt ) {
    if ( collEnt == nullptr ) {
        return 0;
    }

    clearCollEnt( collEnt );

    free( collEnt );

    return 0;
}

int
clearCollEnt( collEnt_t *collEnt ) {
    if ( collEnt == nullptr ) {
        return 0;
    }

    if ( collEnt->collName != nullptr ) {
        free( collEnt->collName );
    }
    if ( collEnt->dataName != nullptr ) {
        free( collEnt->dataName );
    }
    if ( collEnt->dataId != nullptr ) {
        free( collEnt->dataId );
    }
    if ( collEnt->createTime != nullptr ) {
        free( collEnt->createTime );
    }
    if ( collEnt->modifyTime != nullptr ) {
        free( collEnt->modifyTime );
    }
    if ( collEnt->chksum != nullptr ) {
        free( collEnt->chksum );
    }
    if ( collEnt->resource != nullptr ) {
        free( collEnt->resource );
    }
    if ( collEnt->resc_hier != nullptr ) {
        free( collEnt->resc_hier );
    }
    if ( collEnt->phyPath != nullptr ) {
        free( collEnt->phyPath );
    }
    if ( collEnt->ownerName != nullptr ) {
        free( collEnt->ownerName );
    }
    if ( collEnt->dataType != nullptr ) {
        free( collEnt->dataType );
    }
    return 0;
}


int
myChmod( char *inPath, uint dataMode ) {
    if ( dataMode < 0100 ) {
        return 0;
    }

    if ( Myumask == INIT_UMASK_VAL ) {
        Myumask = umask( 0022 );
        umask( Myumask );
    }

    int error_code = chmod( inPath, dataMode & 0777 & ~( Myumask ) );
    int errsav = errno;
    if ( error_code != 0 ) {
        rodsLog( LOG_ERROR, "chmod failed for [%s] in myChmod with error code [%s]", inPath, strerror( errsav ) );
    }

    return 0;
}

char *
getZoneHintForGenQuery( genQueryInp_t *genQueryInp ) {
    char *zoneHint;
    int i;

    if ( genQueryInp == nullptr ) {
        return nullptr;
    }

    if ( ( zoneHint = getValByKey( &genQueryInp->condInput, ZONE_KW ) ) != nullptr ) {
        return zoneHint;
    }

    for ( i = 0; i < genQueryInp->sqlCondInp.len; i++ ) {
        int inx = genQueryInp->sqlCondInp.inx[i];
        if ( inx == COL_COLL_NAME ||
                inx == COL_COLL_PARENT_NAME ||
                inx == COL_ZONE_NAME ) {
            char *tmpPtr;
            zoneHint = genQueryInp->sqlCondInp.value[i];
            if (strcmp(zoneHint, NON_ROOT_COLL_CHECK_STR)) {
                if ( ( tmpPtr = strchr( zoneHint, '/' ) ) != nullptr ) {
                    zoneHint = tmpPtr;
                }
                return zoneHint;
            }
        }
    }
    return nullptr;
}

/* getZoneType - get the ZoneType of inZoneName. icatZone is the icat
 * to query.
 */
int
getZoneType( rcComm_t *conn, char *icatZone, char *inZoneName,
             char *outZoneType ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = nullptr;
    int status;
    sqlResult_t *zoneType;
    char tmpStr[MAX_NAME_LEN];

    if ( inZoneName == nullptr || outZoneType == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( icatZone != nullptr && strlen( icatZone ) > 0 ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, icatZone );
    }
    addInxIval( &genQueryInp.selectInp, COL_ZONE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_ZONE_TYPE, 1 );

    snprintf( tmpStr, MAX_NAME_LEN, " = '%s'", inZoneName );

    addInxVal( &genQueryInp.sqlCondInp, COL_ZONE_NAME, tmpStr );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rcGenQuery( conn, &genQueryInp, &genQueryOut );

    if ( status < 0 ) {
        return status;
    }

    if ( ( zoneType = getSqlResultByInx( genQueryOut, COL_ZONE_TYPE ) ) == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "getZoneInfo: getSqlResultByInx for COL_ZONE_TYPE failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    rstrcpy( outZoneType, zoneType->value, MAX_NAME_LEN );

    freeGenQueryOut( &genQueryOut );

    return 0;
}

/* getCollSizeForProgStat - Calculate the totalNumFiles and totalFileSize and
 * put it in the operProgress struct.
 * Note that operProgress->totalNumFiles and operProgress->totalFileSize
 * needs to be initialized since it can be a recursive operation and
 * cannot be initialized in this routine.
 */
int
getCollSizeForProgStat( rcComm_t *conn, char *srcColl,
                        operProgress_t *operProgress ) {
    int status = 0;
    collHandle_t collHandle;
    collEnt_t collEnt;

    status = rclOpenCollection( conn, srcColl, RECUR_QUERY_FG,
                                &collHandle );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getCollSizeForProgStat: rclOpenCollection of %s error. status = %d",
                 srcColl, status );
        return status;
    }

    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            operProgress->totalNumFiles++;
            operProgress->totalFileSize += collEnt.dataSize;
        }
        else if ( collEnt.objType == COLL_OBJ_T ) {
            if ( collEnt.specColl.collClass != NO_SPEC_COLL ) {
                /* the child is a spec coll. need to drill down */
                status = getCollSizeForProgStat( conn, collEnt.collName,
                                                 operProgress );
                if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                    return status;
                }
            }
        }
    }
    if ( status == CAT_NO_ROWS_FOUND ) {
        return 0;
    }
    else {
        return status;
    }
}

/* getDirSizeForProgStat - Calculate the totalNumFiles and totalFileSize and
 * put it in the operProgress struct.
 * Note that operProgress->totalNumFiles and operProgress->totalFileSize
 * needs to be initialized since it can be a recursive operation and
 * cannot be initialized in this routine.
 */
int
getDirSizeForProgStat( rodsArguments_t *rodsArgs, char *srcDir,
                       operProgress_t *operProgress ) {
    using namespace boost::filesystem;
    int status = 0;
    char srcChildPath[MAX_NAME_LEN];

    if ( isPathSymlink( rodsArgs, srcDir ) > 0 ) {
        return 0;
    }

    path srcDirPath( srcDir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        rodsLog( LOG_ERROR,
                 "getDirSizeForProgStat: opendir local dir error for %s, errno = %d\n",
                 srcDir, errno );
        return USER_INPUT_PATH_ERR;
    }

    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path p = itr->path();
        snprintf( srcChildPath, MAX_NAME_LEN, "%s",
                  p.c_str() );

        if ( isPathSymlink( rodsArgs, srcChildPath ) > 0 ) {
            return 0;
        } // JMC cppcheck - resource

        if ( !exists( p ) ) {
            rodsLog( LOG_ERROR,
                     "getDirSizeForProgStat: stat error for %s, errno = %d\n",
                     srcChildPath, errno );
            return USER_INPUT_PATH_ERR;
        }
        else if ( is_regular_file( p ) ) {
            operProgress->totalNumFiles++;
            operProgress->totalFileSize += file_size( p );
        }
        else if ( is_directory( p ) ) {
            status = getDirSizeForProgStat( rodsArgs, srcChildPath,
                                            operProgress );
            if ( status < 0 ) {
                return status;
            }

        }
    }

    return status;
}

/* iCommandProgStat - the guiProgressCallback for icommand
 */
guiProgressCallback
iCommandProgStat( operProgress_t *operProgress ) {
    using namespace boost::filesystem;
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    int status;
    time_t myTime;
    struct tm *mytm;
    char timeStr[TIME_LEN];

    if ( strchr( operProgress->curFileName, '/' ) == nullptr ) {
        /* relative path */
        rstrcpy( myFile, operProgress->curFileName, MAX_NAME_LEN );
    }
    else if ( ( status = splitPathByKey( operProgress->curFileName,
                                         myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLogError( LOG_NOTICE, status,
                      "iCommandProgStat: splitPathByKey for %s error, status = %d",
                      operProgress->curFileName, status );
        return nullptr;
    }

    myTime = time( nullptr );
    mytm = localtime( &myTime );
    getLocalTimeStr( mytm, timeStr );
    if ( operProgress->flag == 0 ) {
        printf(
            "%-lld/%-lld - %5.2f%% of files done   ",
            operProgress->totalNumFilesDone, operProgress->totalNumFiles,
            VERIFY_DIV
            ( operProgress->totalNumFilesDone, operProgress->totalNumFiles ) *
            100.0 );
        printf( "%-.3f/%-.3f MB - %5.2f%% of file sizes done\n",
                ( float ) operProgress->totalFileSizeDone / 1048576.0,
                ( float ) operProgress->totalFileSize / 1048576.0,
                VERIFY_DIV
                ( operProgress->totalFileSizeDone, operProgress->totalFileSize ) *
                100.0 );
        printf( "Processing %s - %-.3f MB   %s\n", myFile,
                ( float ) operProgress->curFileSize / 1048576.0, timeStr );
    }
    else if ( operProgress->flag == 1 ) {
        printf( "%s - %-.3f/%-.3f MB - %5.2f%% done   %s\n", myFile,
                ( float ) operProgress->curFileSizeDone / 1048576.0,
                ( float ) operProgress->curFileSize / 1048576.0,
                VERIFY_DIV
                ( operProgress->curFileSizeDone, operProgress->curFileSize ) *
                100.0, timeStr );
        /* done. don't print again */
        if ( operProgress->curFileSizeDone == operProgress->curFileSize ) {
            operProgress->flag = 2;
        }
    }
    return nullptr;
}

int
getOpenedCollLen( collHandle_t *collHandle ) {
    int len;

    if ( collHandle->rodsObjStat->specColl != nullptr &&
            ( len = strlen( collHandle->linkedObjPath ) ) > 0 ) {
    }
    else {
        len = strlen( collHandle->dataObjInp.objPath );
    }
    return len;
}

int
rmSubDir( char *mydir ) {
    using namespace boost::filesystem;
    int status = 0;
    int savedStatus = 0;
    char childPath[MAX_NAME_LEN];

    path srcDirPath( mydir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        status = USER_INPUT_PATH_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "rmSubDir: opendir local dir error for %s", mydir );
        return status;
    }
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path p = itr->path();
        snprintf( childPath, MAX_NAME_LEN, "%s",
                  p.c_str() );
        if ( !exists( p ) ) {
            savedStatus = USER_INPUT_PATH_ERR - errno;
            rodsLogError( LOG_ERROR, savedStatus,
                          "rmSubDir: stat error for %s", childPath );
            continue;
        }
        if ( is_directory( p ) ) {
            status = rmSubDir( childPath );
            if ( status < 0 ) {
                savedStatus = USER_INPUT_PATH_ERR - errno;
                rodsLogError( LOG_ERROR, status,
                              "rmSubDir: rmSubDir error for %s ", childPath );
            }
            if ( rmdir( childPath ) != 0 ) {
                savedStatus = USER_INPUT_PATH_ERR - errno;
                rodsLogError( LOG_ERROR, status,
                              "rmSubDir: rmdir error for %s ", childPath );
            }
            continue;
        }
        else {
            savedStatus = USER_INPUT_PATH_ERR - errno;
            rodsLogError( LOG_ERROR, status,
                          "rmSubDir: %s is not a dir", childPath );
            continue;
        }
    }
    return savedStatus;
}

int
rmFilesInDir( char *mydir ) {
    using namespace boost::filesystem;
    int status = 0;
    int savedStatus = 0;
    char childPath[MAX_NAME_LEN];

    path srcDirPath( mydir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        status = USER_INPUT_PATH_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "rmFilesInDir: opendir local dir error for %s", mydir );
        return status;
    }
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path p = itr->path();
        snprintf( childPath, MAX_NAME_LEN, "%s",
                  p.c_str() );
        if ( !exists( p ) ) {
            savedStatus = USER_INPUT_PATH_ERR - errno;
            rodsLogError( LOG_ERROR, savedStatus,
                          "rmFilesInDir: stat error for %s", childPath );
            continue;
        }
        if ( is_regular_file( p ) ) {
            unlink( childPath );
        }
        else {
            continue;
        }
    }
    return savedStatus;
}
int
getNumFilesInDir( const char *mydir ) {
    using namespace boost::filesystem;
    int status = 0;
    int savedStatus = 0;
    char childPath[MAX_NAME_LEN];
    int count = 0;

    path srcDirPath( mydir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        status = USER_INPUT_PATH_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "getNumFilesInDir: opendir local dir error for %s", mydir );
        return status;
    }
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path p = itr->path();
        snprintf( childPath, MAX_NAME_LEN, "%s",
                  p.c_str() );
        if ( !exists( p ) ) {
            savedStatus = USER_INPUT_PATH_ERR - errno;
            rodsLogError( LOG_ERROR, savedStatus,
                          "getNumFilesInDir: stat error for %s", childPath );
            continue;
        }
        if ( is_regular_file( p ) ) {
            count++;
        }
        else {
            continue;
        }

    }
    return count;
}

pathnamePatterns_t *
readPathnamePatterns( char *buf, int buflen ) {
    if ( buf == nullptr || buflen <= 0 ) {
        return nullptr;
    }

    /* Allocate and initialize the return structure */
    pathnamePatterns_t *pp = ( pathnamePatterns_t* )malloc( sizeof( pathnamePatterns_t ) );
    if ( pp == nullptr ) {
        rodsLog( LOG_NOTICE, "readPathnamePatterns: could not allocate pp struct" );
        return nullptr;
    }

    /* copy the passed in buffer since we'll be
       manipulating it */
    pp->pattern_buf = ( char* )malloc( buflen + 1 );
    if ( pp->pattern_buf == nullptr ) {
        rodsLog( LOG_NOTICE, "readPathnamePatterns: could not allocate pattern buffer" );
        free(pp);
        return nullptr;
    }
    memcpy( pp->pattern_buf, buf, buflen );
    pp->pattern_buf[buflen] = '\n';

    /* Get the addresses of the patterns in the buffer.
       They'll be delimited by newlines. */
    std::vector<char *> patterns_vector;
    bool beginning_of_line = true;
    for ( int i = 0; i < buflen; i++ ) {
        if ( pp->pattern_buf[i] == '\n' ) {
            pp->pattern_buf[i] = '\0';
            beginning_of_line = true;
        }
        else if (beginning_of_line) {
            beginning_of_line = false;
            if ( pp->pattern_buf[i] != '#' ) {
                patterns_vector.push_back(pp->pattern_buf + i);
            }
        }
    }
    pp->num_patterns = patterns_vector.size();

    /* now allocate a string array for the patterns, and
       make each array element point to a pattern string
       in place in the buffer */
    if (pp->num_patterns > 0) {
        pp->patterns = ( char** )malloc( sizeof( char* ) * pp->num_patterns );
        if ( pp->patterns == nullptr ) {
            rodsLog( LOG_NOTICE, "readPathnamePatterns: could not allocate pattern array" );
            free(pp->pattern_buf);
            free(pp);
            return nullptr;
        }
        memcpy(pp->patterns, &patterns_vector[0], pp->num_patterns * sizeof(char*));
    }
    else {
        pp->patterns = nullptr;
    }

    return pp;
}

void
freePathnamePatterns( pathnamePatterns_t *pp ) {
    if ( pp == nullptr ) {
        return;
    }

    if ( pp->patterns ) {
        free( pp->patterns );
    }
    if ( pp->pattern_buf ) {
        free( pp->pattern_buf );
    }
    free( pp );
}

int
matchPathname( pathnamePatterns_t *pp, char *name, char *dirname ) {
    int index;
    char pathname[MAX_NAME_LEN];
    char *pattern;

    if ( pp == nullptr || name == nullptr || dirname == nullptr ) {
        return 0;
    }

    /* If the pattern starts with '/', try to match the full path.
       Otherwise, just match name. */
    for ( index = 0; index < pp->num_patterns; index++ ) {
        pattern = pp->patterns[index];
        if ( *pattern == '/' ) {
            /* a directory pattern */
            snprintf( pathname, MAX_NAME_LEN, "%s/%s", dirname, name );
            if ( fnmatch( pattern, pathname, FNM_PATHNAME ) == 0 ) {
                rodsLog( LOG_DEBUG, "matchPathname: match name %s with pattern %s",
                         pathname, pattern );
                /* we have a match */
                return 1;
            }
        }
        else {
            if ( fnmatch( pattern, name, 0 ) == 0 ) {
                rodsLog( LOG_DEBUG, "matchPathname: match name %s with pattern %s",
                         name, pattern );
                /* we have a match */
                return 1;
            }
        }
    }

    return 0;
}

/* resolveRodsTarget - based on srcPath and destPath, fill in targPath.
 * oprType -
 *      MOVE_OPR - do not create the target coll or dir because rename will
 *        take care of it.
 *      RSYNC_OPR - uses the destPath and the targPath if the src is a
 *        collection
 *      All other oprType will be treated as normal.
 */
int
resolveRodsTarget( rcComm_t *conn, rodsPathInp_t *rodsPathInp, int oprType ) {
    rodsPath_t *srcPath, *destPath;
    char srcElement[MAX_NAME_LEN], destElement[MAX_NAME_LEN];
    int status;
    int srcInx;
    rodsPath_t *targPath;

    if ( rodsPathInp == nullptr ) {
        rodsLog( LOG_ERROR,
                 "resolveRodsTarget: NULL rodsPathInp input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsPathInp->srcPath == nullptr ) {
        rodsLog( LOG_ERROR,
                 "resolveRodsTarget: NULL rodsPathInp->srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsPathInp->destPath == nullptr ) {
        rodsLog( LOG_ERROR,
                 "resolveRodsTarget: NULL rodsPathInp->destPath input" );
        return USER__NULL_INPUT_ERR;
    }

    destPath = rodsPathInp->destPath;
    if ( destPath->objState == UNKNOWN_ST ) {
        getRodsObjType( conn, destPath );
    }

    for ( srcInx = 0; srcInx < rodsPathInp->numSrc; srcInx++ ) {
        srcPath = &rodsPathInp->srcPath[srcInx];
        targPath = &rodsPathInp->targPath[srcInx];

        /* we don't do wild card yet */

        if ( srcPath->objState == UNKNOWN_ST ) {
            getRodsObjType( conn, srcPath );
            if ( srcPath->objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: srcPath %s does not exist",
                         srcPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
        }

        if ( destPath->objType >= UNKNOWN_FILE_T &&
                strcmp( destPath->outPath, STDOUT_FILE_NAME ) == 0 ) {
            /* pipe to stdout */
            if ( srcPath->objType != DATA_OBJ_T ) {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: src %s is the wrong type for dest -",
                         srcPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
            *targPath = *destPath;
            targPath->objType = LOCAL_FILE_T;
        }
        else if ( srcPath->objType == DATA_OBJ_T ||
                  srcPath->objType == LOCAL_FILE_T ) {
            /* file type source */

            if ( ( destPath->objType == COLL_OBJ_T ||
                    destPath->objType == LOCAL_DIR_T ) &&
                    destPath->objState == EXIST_ST ) {
                if ( destPath->objType <= COLL_OBJ_T ) {
                    targPath->objType = DATA_OBJ_T;
                }
                else {
                    targPath->objType = LOCAL_FILE_T;
                }

                /* collection */
                getLastPathElement( srcPath->inPath, srcElement );
                if ( strlen( srcElement ) > 0 ) {
                    snprintf( targPath->outPath, MAX_NAME_LEN, "%s/%s",
                              destPath->outPath, srcElement );
                    if ( destPath->objType <= COLL_OBJ_T ) {
                        getRodsObjType( conn, destPath );
                    }
                }
                else {
                    rstrcpy( targPath->outPath, destPath->outPath,
                             MAX_NAME_LEN );
                }
            }
            else if ( destPath->objType == DATA_OBJ_T ||
                      destPath->objType == LOCAL_FILE_T || rodsPathInp->numSrc == 1 ) {
                *targPath = *destPath;
                if ( destPath->objType <= COLL_OBJ_T ) {
                    targPath->objType = DATA_OBJ_T;
                }
                else {
                    targPath->objType = LOCAL_FILE_T;
                }
            }
            else {
                rodsLogError( LOG_ERROR, USER_FILE_DOES_NOT_EXIST,
                              "resolveRodsTarget: target %s does not exist",
                              destPath->outPath );
                return USER_FILE_DOES_NOT_EXIST;
            }
        }
        else if ( srcPath->objType == COLL_OBJ_T ||
                  srcPath->objType == LOCAL_DIR_T ) {
            /* directory type source */

            if ( destPath->objType <= COLL_OBJ_T ) {
                targPath->objType = COLL_OBJ_T;
            }
            else {
                targPath->objType = LOCAL_DIR_T;
            }

            if ( destPath->objType == DATA_OBJ_T ||
                    destPath->objType == LOCAL_FILE_T ) {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: input destPath %s is a datapath",
                         destPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
            else if ( ( destPath->objType == COLL_OBJ_T ||
                        destPath->objType == LOCAL_DIR_T ) &&
                      destPath->objState == EXIST_ST ) {
                /* the collection exist */
                getLastPathElement( srcPath->inPath, srcElement );
                if ( strlen( srcElement ) > 0 ) {
                    if ( rodsPathInp->numSrc == 1 && oprType == RSYNC_OPR ) {
                        getLastPathElement( destPath->inPath, destElement );
                        /* RSYNC_OPR. Just use the same path */
                        if ( strlen( destElement ) > 0 ) {
                            rstrcpy( targPath->outPath, destPath->outPath,
                                     MAX_NAME_LEN );
                        }
                    }
                    if ( targPath->outPath[0] == '\0' ) {
                        snprintf( targPath->outPath, MAX_NAME_LEN, "%s/%s",
                                  destPath->outPath, srcElement );
                        /* make the collection */
                        if ( destPath->objType == COLL_OBJ_T ) {
                            if ( oprType != MOVE_OPR ) {
                                /* rename does not need to mkColl */
                                if ( srcPath->objType <= COLL_OBJ_T ) {
                                    status = mkColl( conn, destPath->outPath );
                                }
                                else {
                                    status = mkColl( conn, targPath->outPath );
                                }
                            }
                            else {
                                status = 0;
                            }
                        }
                        else {
#ifdef windows_platform
                            status = iRODSNt_mkdir( targPath->outPath, 0750 );
#else
                            status = mkdir( targPath->outPath, 0750 );
#endif
                            if ( status < 0 && errno == EEXIST ) {
                                status = 0;
                            }
                        }
                        if ( status < 0 ) {
                            rodsLogError( LOG_ERROR, status,
                                          "resolveRodsTarget: mkColl/mkdir for %s,status=%d",
                                          targPath->outPath, status );
                            return status;
                        }
                    }
                }
                else {
                    rstrcpy( targPath->outPath, destPath->outPath,
                             MAX_NAME_LEN );
                }
            }
            else {
                /* dest coll does not exist */
                if ( destPath->objType <= COLL_OBJ_T ) {
                    if ( oprType != MOVE_OPR ) {
                        /* rename does not need to mkColl */
                        status = mkColl( conn, destPath->outPath );
                    }
                    else {
                        status = 0;
                    }
                }
                else {
                    /* use destPath. targPath->outPath not defined.
                     *  status = mkdir (targPath->outPath, 0750); */
#ifdef windows_platform
                    status = iRODSNt_mkdir( destPath->outPath, 0750 );
#else
                    status = mkdir( destPath->outPath, 0750 );
#endif
                }
                if ( status < 0 ) {
                    return status;
                }

                if ( rodsPathInp->numSrc == 1 ) {
                    rstrcpy( targPath->outPath, destPath->outPath,
                             MAX_NAME_LEN );
                }
                else {
                    rodsLogError( LOG_ERROR, USER_FILE_DOES_NOT_EXIST,
                                  "resolveRodsTarget: target %s does not exist",
                                  destPath->outPath );
                    return USER_FILE_DOES_NOT_EXIST;
                }
            }
            targPath->objState = EXIST_ST;
        }
        else {          /* should not be here */
            if ( srcPath->objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: source %s does not exist",
                         srcPath->outPath );
            }
            else {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: cannot handle objType %d for srcPath %s",
                         srcPath->objType, srcPath->outPath );
            }
            return USER_INPUT_PATH_ERR;
        }
    }
    return 0;
}
