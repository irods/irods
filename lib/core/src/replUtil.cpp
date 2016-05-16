/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "miscUtil.h"
#include "replUtil.h"
#include "rcGlobalExtern.h"

int
replUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
          rodsPathInp_t *rodsPathInp ) {


    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    dataObjInp_t dataObjInp;
    rodsRestart_t rodsRestart;
    initCondForRepl( myRodsEnv, myRodsArgs, &dataObjInp, &rodsRestart );

    int savedStatus = 0;
    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        if ( rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T ) {
            getRodsObjType( conn, &rodsPathInp->srcPath[i] );
            if ( rodsPathInp->srcPath[i].objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR,
                         "replUtil: srcPath %s does not exist",
                         rodsPathInp->srcPath[i].outPath );
                savedStatus = USER_INPUT_PATH_ERR;
                continue;
            }
        }
    }

    /* initialize the progress struct */
    if ( gGuiProgressCB != NULL ) {
        bzero( &conn->operProgress, sizeof( conn->operProgress ) );
        for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
            if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
                conn->operProgress.totalNumFiles++;
                if ( rodsPathInp->srcPath[i].size > 0 ) {
                    conn->operProgress.totalFileSize +=
                        rodsPathInp->srcPath[i].size;
                }
            }
            else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
                getCollSizeForProgStat( conn, rodsPathInp->srcPath[i].outPath,
                                        &conn->operProgress );
            }
        }
    }

    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        int status = 0;
        if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
            rmKeyVal( &dataObjInp.condInput, TRANSLATED_PATH_KW );
            status = replDataObjUtil( conn, rodsPathInp->srcPath[i].outPath,
                                      rodsPathInp->srcPath[i].size, myRodsArgs, &dataObjInp );
        }
        else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
            setStateForRestart( &rodsRestart, &rodsPathInp->srcPath[i],
                                myRodsArgs );
            addKeyVal( &dataObjInp.condInput, TRANSLATED_PATH_KW, "" );
            status = replCollUtil( conn, rodsPathInp->srcPath[i].outPath,
                                   myRodsEnv, myRodsArgs, &dataObjInp, &rodsRestart );
            if ( rodsRestart.fd > 0 && status < 0 ) {
                close( rodsRestart.fd );
                return status;
            }
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "replUtil: invalid repl objType %d for %s",
                     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLogError( LOG_ERROR, status,
                          "replUtil: repl error for %s, status = %d",
                          rodsPathInp->srcPath[i].outPath, status );
            savedStatus = status;
        }
    }
    return savedStatus;
}

int
replDataObjUtil( rcComm_t *conn, char *srcPath, rodsLong_t srcSize,
                 rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp ) {
    int status;
    struct timeval startTime, endTime;

    if ( srcPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "replDataObjUtil: NULL srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->verbose == True ) {
        ( void ) gettimeofday( &startTime, ( struct timezone * )0 );
    }

    if ( gGuiProgressCB != NULL ) {
        rstrcpy( conn->operProgress.curFileName, srcPath, MAX_NAME_LEN );
        conn->operProgress.curFileSize = srcSize;
        conn->operProgress.curFileSizeDone = 0;
        conn->operProgress.flag = 0;
        gGuiProgressCB( &conn->operProgress );
    }

    rstrcpy( dataObjInp->objPath, srcPath, MAX_NAME_LEN );

    status = rcDataObjRepl( conn, dataObjInp );

    if ( status >= 0 ) {
        if ( rodsArgs->verbose == True ) {
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, dataObjInp->objPath,
                         conn->transStat.bytesWritten, NULL, &startTime, &endTime );
        }
        if ( gGuiProgressCB != NULL ) {
            conn->operProgress.totalNumFilesDone++;
            conn->operProgress.totalFileSizeDone += srcSize;
        }
    }

    return status;
}

int
initCondForRepl( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                 dataObjInp_t *dataObjInp, rodsRestart_t *rodsRestart ) {
    char *myResc = NULL;
    char *tmpStr;

    if ( dataObjInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForRepl: NULL dataObjInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjInp, 0, sizeof( dataObjInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    if ( rodsArgs->replNum == True ) {
        addKeyVal( &dataObjInp->condInput, REPL_NUM_KW,
                   rodsArgs->replNumValue );
    }

    if ( rodsArgs->purgeCache == True ) { //  JMC - backport 4549
        addKeyVal( &dataObjInp->condInput, PURGE_CACHE_KW, "" );
    }


    if ( rodsArgs->all == True ) {
        addKeyVal( &dataObjInp->condInput, ALL_KW, "" );
    }

    if ( rodsArgs->admin == True ) {
        addKeyVal( &dataObjInp->condInput, ADMIN_KW, "" );
    }

    if ( rodsArgs->srcResc == True ) {
        addKeyVal( &dataObjInp->condInput, RESC_NAME_KW,
                   rodsArgs->srcRescString );
    }

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForRepl: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            myResc = rodsArgs->resourceString;
            addKeyVal( &dataObjInp->condInput, DEST_RESC_NAME_KW,
                       rodsArgs->resourceString );
        }
    }
    else if ( myRodsEnv != NULL && strlen( myRodsEnv->rodsDefResource ) > 0 ) {
        myResc = myRodsEnv->rodsDefResource;
        addKeyVal( &dataObjInp->condInput, DEF_RESC_NAME_KW,
                   myRodsEnv->rodsDefResource );
    }

    if ( myResc != NULL && rodsArgs->backupMode == True ) {
        addKeyVal( &dataObjInp->condInput, BACKUP_RESC_NAME_KW,
                   myResc );
    }

    if ( rodsArgs->unmount == True ) {
        /* use unmount for update */
        addKeyVal( &dataObjInp->condInput, UPDATE_REPL_KW, "" );
    }

    if ( rodsArgs->rbudp == True ) {
        /* use -Q for rbudp transfer */
        addKeyVal( &dataObjInp->condInput, RBUDP_TRANSFER_KW, "" );
    }

    if ( rodsArgs->veryVerbose == True ) {
        addKeyVal( &dataObjInp->condInput, VERY_VERBOSE_KW, "" );
    }

    if ( ( tmpStr = getenv( RBUDP_SEND_RATE_KW ) ) != NULL ) {
        addKeyVal( &dataObjInp->condInput, RBUDP_SEND_RATE_KW, tmpStr );
    }

    if ( ( tmpStr = getenv( RBUDP_PACK_SIZE_KW ) ) != NULL ) {
        addKeyVal( &dataObjInp->condInput, RBUDP_PACK_SIZE_KW, tmpStr );
    }

    memset( rodsRestart, 0, sizeof( rodsRestart_t ) );
    if ( rodsArgs->restart == True ) {
        int status;
        status = openRestartFile( rodsArgs->restartFileString, rodsRestart );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "initCondForRepl: openRestartFile of %s errno",
                          rodsArgs->restartFileString );
            return status;
        }
    }
    if ( rodsArgs->rlock == True ) { // JMC - backport 4609
        addKeyVal( &dataObjInp->condInput, LOCK_TYPE_KW, READ_LOCK_TYPE );
    }
    if ( rodsArgs->number == True ) {
        if ( rodsArgs->numberValue == 0 ) {
            dataObjInp->numThreads = NO_THREADING;
        }
        else {
            dataObjInp->numThreads = rodsArgs->numberValue;
        }
    }
    if ( rodsArgs->wlock == True ) { // JMC - backport 4612
        rodsLog( LOG_ERROR,
                 "initCondForPut: --wlock not supported, changing it to --rlock" );
        addKeyVal( &dataObjInp->condInput, LOCK_TYPE_KW, READ_LOCK_TYPE );
    }

    return 0;
}

int
replCollUtil( rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
              rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp,
              rodsRestart_t *rodsRestart ) {
    int status;
    int savedStatus = 0;
    collHandle_t collHandle;
    collEnt_t collEnt;
    char srcChildPath[MAX_NAME_LEN];

    if ( srcColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "replCollUtil: NULL srcColl input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "replCollUtil: -r option must be used for getting %s collection",
                 srcColl );
        return USER_INPUT_OPTION_ERR;
    }

    if ( rodsArgs->verbose == True ) {
        fprintf( stdout, "C- %s:\n", srcColl );
    }

    bzero( &collHandle, sizeof( collHandle ) );
    replKeyVal( &dataObjInp->condInput, &collHandle.dataObjInp.condInput );
    status = rclOpenCollection( conn, srcColl, INCLUDE_CONDINPUT_IN_QUERY,
                                &collHandle );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "replCollUtil: rclOpenCollection of %s error. status = %d",
                 srcColl, status );
        return status;
    }
    if ( collHandle.rodsObjStat->specColl != NULL &&
            collHandle.rodsObjStat->specColl->collClass != LINKED_COLL ) {
        /* no repl for mounted coll */
        rclCloseCollection( &collHandle );
        return 0;
    }
    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            snprintf( srcChildPath, MAX_NAME_LEN, "%s/%s",
                      collEnt.collName, collEnt.dataName );

            int status = chkStateForResume( conn, rodsRestart, srcChildPath,
                                        rodsArgs, DATA_OBJ_T, &dataObjInp->condInput, 0 );

            if ( status < 0 ) {
                /* restart failed */
                break;
            }
            else if ( status == 0 ) {
                continue;
            }

            status = replDataObjUtil( conn, srcChildPath, collEnt.dataSize,
                                      rodsArgs, dataObjInp );

            if ( status == SYS_COPY_ALREADY_IN_RESC ) {
                if ( rodsArgs->verbose == True ) {
                    printf( "copy of %s already exists. Probably OK\n",
                            srcChildPath );
                }
                status = 0;
            }

            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "replCollUtil: replDataObjUtil failed for %s. status = %d",
                              srcChildPath, status );
                savedStatus = status;
                if ( rodsRestart->fd > 0 ) {
                    break;
                }
            }
            else {
                status = procAndWriteRestartFile( rodsRestart, srcChildPath );
                if ( status < 0 ) {
                    rodsLogError( LOG_ERROR, status,
                                "replCollUtil: procAndWriteRestartFile failed for %s. status = %d",
                                srcChildPath, status );
                    savedStatus = status;
                }
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
            int status = replCollUtil( conn, collEnt.collName, myRodsEnv,
                                   rodsArgs, &childDataObjInp, rodsRestart );
            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                savedStatus = status;
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

