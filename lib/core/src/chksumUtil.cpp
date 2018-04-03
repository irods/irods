/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "miscUtil.h"
#include "rodsLog.h"
#include "chksumUtil.h"
#include "rcGlobalExtern.h"

static int ChksumCnt = 0;
static int FailedChksumCnt = 0;
int
chksumUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
            rodsPathInp_t *rodsPathInp ) {
    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    collInp_t collInp;
    dataObjInp_t dataObjInp;
    int savedStatus = initCondForChksum( myRodsArgs, &dataObjInp, &collInp );

    if ( savedStatus < 0 ) {
        return savedStatus;
    }

    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        if ( rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T ) {
            getRodsObjType( conn, &rodsPathInp->srcPath[i] );
            if ( rodsPathInp->srcPath[i].objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR, "chksumUtil: srcPath %s does not exist", rodsPathInp->srcPath[i].outPath );
                savedStatus = USER_INPUT_PATH_ERR;
                continue;
            }
        }

        int status = 0;
        if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
            rmKeyVal( &dataObjInp.condInput, TRANSLATED_PATH_KW );
            status = chksumDataObjUtil( conn, rodsPathInp->srcPath[i].outPath, myRodsArgs, &dataObjInp );
        }

        else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
            addKeyVal( &dataObjInp.condInput, TRANSLATED_PATH_KW, "" );
            status = chksumCollUtil( conn, rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs, &dataObjInp, &collInp );
        }

        else {
            /* should not be here */
            rodsLog( LOG_ERROR, "chksumUtil: invalid chksum objType %d for %s",
                     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLogError( LOG_ERROR, status, "chksumUtil: chksum error for %s, status = %d",
                          rodsPathInp->srcPath[i].outPath, status );
            savedStatus = status;
        }
    }

    printf( "Total checksum performed = %d, Failed checksum = %d\n",
            ChksumCnt, FailedChksumCnt );

    return savedStatus;
}

int
chksumDataObjUtil( rcComm_t *conn, char *srcPath,
                   rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp ) {
    int status;
    struct timeval startTime, endTime;
    char *chksumStr = NULL;
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];

    if ( srcPath == NULL ) {

        rodsLog( LOG_ERROR,
                 "chksumDataObjUtil: NULL srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->verbose == True ) {
        ( void ) gettimeofday( &startTime, ( struct timezone * )0 );
    }

    rstrcpy( dataObjInp->objPath, srcPath, MAX_NAME_LEN );

    status = rcDataObjChksum( conn, dataObjInp, &chksumStr );

    ChksumCnt++;

    if ( status < 0 ) {
        FailedChksumCnt++;
        rodsLogError( LOG_ERROR, status,
                      "chksumDataObjUtil: rcDataObjChksum error for %s",
                      dataObjInp->objPath );
        printErrorStack(conn->rError);
        freeRError(conn->rError);
        conn->rError = NULL;
        return status;
    }

    status = splitPathByKey( dataObjInp->objPath, myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' );

    if ( 0 == status && rodsArgs->silent == False ) {
        printf( "    %s    %s\n", myFile, chksumStr );
        free( chksumStr );
        if ( rodsArgs->verbose == True ) {
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, dataObjInp->objPath, -1, NULL,
                         &startTime, &endTime );
        }
    }

    return status;
}

int
initCondForChksum( rodsArguments_t *rodsArgs,
                   dataObjInp_t *dataObjInp, collInp_t *collInp ) {
    if ( dataObjInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForChksum: NULL dataObjInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjInp, 0, sizeof( dataObjInp_t ) );
    memset( collInp, 0, sizeof( collInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    if ( rodsArgs->force == True && rodsArgs->verifyChecksum ) {
        rodsLog( LOG_ERROR,
                 "initCondForChksum: the 'K' and 'f' option cannot be used together" );
        return USER_OPTION_INPUT_ERR;
    }

    if ( rodsArgs->all == True && rodsArgs->replNum == True ) {
        rodsLog( LOG_ERROR,
                 "initCondForChksum: the 'N' and 'a' option cannot be used together" );
        return USER_OPTION_INPUT_ERR;
    }

    if ( rodsArgs->admin == True ) {
        addKeyVal( &dataObjInp->condInput, ADMIN_KW, "" );
    }

    if ( rodsArgs->force == True ) {
        addKeyVal( &dataObjInp->condInput, FORCE_CHKSUM_KW, "" );
        addKeyVal( &collInp->condInput, FORCE_CHKSUM_KW, "" );
    }

    if ( rodsArgs->all == True ) {
        addKeyVal( &dataObjInp->condInput, CHKSUM_ALL_KW, "" );
        addKeyVal( &collInp->condInput, CHKSUM_ALL_KW, "" );
    }

    if ( rodsArgs->verifyChecksum == True ) {
        addKeyVal( &dataObjInp->condInput, VERIFY_CHKSUM_KW, "" );
        addKeyVal( &collInp->condInput, VERIFY_CHKSUM_KW, "" );
    }

    if ( rodsArgs->replNum == True ) {
        addKeyVal( &dataObjInp->condInput, REPL_NUM_KW,
                   rodsArgs->replNumValue );
    }

    if ( rodsArgs->resource == True ) {
        addKeyVal( &dataObjInp->condInput, RESC_NAME_KW,
                   rodsArgs->resourceString );
    }

    if (rodsArgs->verify == True) {
        if (rodsArgs->verifyChecksum == True) {
            addKeyVal( &dataObjInp->condInput, VERIFY_VAULT_SIZE_EQUALS_DATABASE_SIZE_KW, "" );
        } else {
            rodsLog(LOG_ERROR, "initCondForChksum: if --verify is used, -K must also be used");
            return USER_OPTION_INPUT_ERR;
        }
    }

    /* XXXXX need to add -u register cond */

    dataObjInp->openFlags = O_RDONLY;

    return 0;
}

int
chksumCollUtil( rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
                rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp, collInp_t *collInp ) {
    int savedStatus = 0;
    char srcChildPath[MAX_NAME_LEN];
    collHandle_t collHandle;
    collEnt_t collEnt;
    int queryFlags;

    if ( srcColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "chksumCollUtil: NULL srcColl input" );
        return USER__NULL_INPUT_ERR;
    }

    fprintf( stdout, "C- %s:\n", srcColl );

    if ( rodsArgs->resource == True ) {
        queryFlags = INCLUDE_CONDINPUT_IN_QUERY;
        bzero( &collHandle, sizeof( collHandle ) );
        replKeyVal( &dataObjInp->condInput, &collHandle.dataObjInp.condInput );
    }
    else {
        queryFlags = 0;
    }
    int status = rclOpenCollection( conn, srcColl, queryFlags, &collHandle );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "chksumCollUtil: rclOpenCollection of %s error. status = %d",
                 srcColl, status );
        return status;
    }
    if ( collHandle.rodsObjStat->specColl != NULL &&
            collHandle.rodsObjStat->specColl->collClass != LINKED_COLL ) {
        /* no trim for mounted coll */
        rclCloseCollection( &collHandle );
        return 0;
    }
    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            snprintf( srcChildPath, MAX_NAME_LEN, "%s/%s",
                      collEnt.collName, collEnt.dataName );
            /* screen unnecessary call to chksumDataObjUtil if user input a
             * resource. */
            int status = chksumDataObjUtil( conn, srcChildPath,
                                        rodsArgs, dataObjInp );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "chksumCollUtil:chksumDataObjU failed for %s.stat = %d",
                              srcChildPath, status );
                /* need to set global error here */
                savedStatus = status;
            }
        }
        else if ( collEnt.objType == COLL_OBJ_T ) {
            dataObjInp_t childDataObjInp;
            childDataObjInp = *dataObjInp;
            if ( collEnt.specColl.collClass != NO_SPEC_COLL ) {
                childDataObjInp.specColl = &collEnt.specColl;
            }
            else {
                childDataObjInp.specColl = NULL;
            }
            int status = chksumCollUtil( conn, collEnt.collName, myRodsEnv,
                                     rodsArgs, &childDataObjInp, collInp );
            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                return status;
            }
        }
    }
    rclCloseCollection( &collHandle );
    if ( savedStatus < 0 ) {
        return savedStatus;
    }
    else if ( status == CAT_NO_ROWS_FOUND ) {
        return 0;
    }
    else {
        return status;
    }
}
