/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"
#include "miscUtil.hpp"
#include "cpUtil.hpp"

int
cpUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
        rodsPathInp_t *rodsPathInp ) {
    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    int savedStatus = resolveRodsTarget( conn, rodsPathInp, 1 );
    if ( savedStatus < 0 ) {
        rodsLogError( LOG_ERROR, savedStatus,
                      "cpUtil: resolveRodsTarget error, status = %d", savedStatus );
        return savedStatus;
    }

    dataObjCopyInp_t dataObjCopyInp;
    rodsRestart_t rodsRestart;
    initCondForCp( myRodsEnv, myRodsArgs, &dataObjCopyInp, &rodsRestart );

    /* initialize the progress struct */
    if ( gGuiProgressCB != NULL ) {
        bzero( &conn->operProgress, sizeof( conn->operProgress ) );
        for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
            rodsPath_t * targPath = &rodsPathInp->targPath[i];
            if ( targPath->objType == DATA_OBJ_T ) {
                conn->operProgress.totalNumFiles++;
                if ( rodsPathInp->srcPath[i].size > 0 ) {
                    conn->operProgress.totalFileSize +=
                        rodsPathInp->srcPath[i].size;
                }
            }
            else {
                getCollSizeForProgStat( conn, rodsPathInp->srcPath[i].outPath,
                                        &conn->operProgress );
            }
        }
    }

    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        rodsPath_t * targPath = &rodsPathInp->targPath[i];

        if ( rodsPathInp->srcPath[i].rodsObjStat != NULL &&
                rodsPathInp->srcPath[i].rodsObjStat->specColl != NULL ) {
            dataObjCopyInp.srcDataObjInp.specColl =
                rodsPathInp->srcPath[i].rodsObjStat->specColl;
        }

        int status = 0;
        if ( targPath->objType == DATA_OBJ_T ) {
            rmKeyVal( &dataObjCopyInp.srcDataObjInp.condInput,
                      TRANSLATED_PATH_KW );
            status = cpFileUtil( conn, rodsPathInp->srcPath[i].outPath,
                                 targPath->outPath, rodsPathInp->srcPath[i].size,
                                 myRodsArgs, &dataObjCopyInp );
        }
        else if ( targPath->objType == COLL_OBJ_T ) {
            setStateForRestart( &rodsRestart, targPath, myRodsArgs );
            addKeyVal( &dataObjCopyInp.srcDataObjInp.condInput,
                       TRANSLATED_PATH_KW, "" );
            status = cpCollUtil( conn, rodsPathInp->srcPath[i].outPath,
                                 targPath->outPath, myRodsEnv, myRodsArgs, &dataObjCopyInp,
                                 &rodsRestart );
            if ( rodsRestart.fd > 0 && status < 0 ) {
                close( rodsRestart.fd );
                return status;
            }
            if ( dataObjCopyInp.srcDataObjInp.specColl != NULL &&
                    dataObjCopyInp.srcDataObjInp.specColl->collClass == STRUCT_FILE_COLL ) {
                dataObjCopyInp.srcDataObjInp.specColl = NULL;
                status = cpCollUtil( conn, rodsPathInp->srcPath[i].outPath,
                                     targPath->outPath, myRodsEnv, myRodsArgs, &dataObjCopyInp,
                                     &rodsRestart );
            }
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "cpUtil: invalid cp dest objType %d for %s",
                     targPath->objType, targPath->outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND &&
                status != SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
            rodsLogError( LOG_ERROR, status,
                          "cpUtil: cp error for %s, status = %d",
                          targPath->outPath, status );
            savedStatus = status;
        }
    }

    if ( rodsRestart.fd > 0 ) {
        close( rodsRestart.fd );
    }
    return savedStatus;
}

int
cpFileUtil( rcComm_t *conn, char *srcPath, char *targPath, rodsLong_t srcSize,
            rodsArguments_t *rodsArgs, dataObjCopyInp_t *dataObjCopyInp ) {
    int status;
    struct timeval startTime, endTime;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "cpFileUtil: NULL srcPath or targPath in cp" );
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

    rstrcpy( dataObjCopyInp->destDataObjInp.objPath, targPath, MAX_NAME_LEN );
    rstrcpy( dataObjCopyInp->srcDataObjInp.objPath, srcPath, MAX_NAME_LEN );
    dataObjCopyInp->srcDataObjInp.dataSize = -1;

    status = rcDataObjCopy( conn, dataObjCopyInp );

    if ( status >= 0 ) {
        if ( rodsArgs->verbose == True ) {
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, dataObjCopyInp->destDataObjInp.objPath,
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
initCondForCp( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
               dataObjCopyInp_t *dataObjCopyInp, rodsRestart_t *rodsRestart ) {
    char *tmpStr;

    if ( dataObjCopyInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForCp: NULL dataObjCopyInp in cp" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjCopyInp, 0, sizeof( dataObjCopyInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    if ( rodsArgs->dataType == True ) {
        if ( rodsArgs->dataTypeString != NULL ) {
            addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, DATA_TYPE_KW,
                       rodsArgs->dataTypeString );
        }
    }

    if ( rodsArgs->force == True ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, FORCE_FLAG_KW, "" );
    }

    if ( rodsArgs->checksum == True ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, REG_CHKSUM_KW, "" );
    }

    if ( rodsArgs->verifyChecksum == True ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                   VERIFY_CHKSUM_KW, "" );
    }

    if ( rodsArgs->number == True ) {
        if ( rodsArgs->numberValue == 0 ) {
            dataObjCopyInp->destDataObjInp.numThreads = NO_THREADING;
        }
        else {
            dataObjCopyInp->destDataObjInp.numThreads = rodsArgs->numberValue;
        }
    }

    if ( rodsArgs->physicalPath == True ) {
        if ( rodsArgs->physicalPathString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForCp: NULL physicalPathString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, FILE_PATH_KW,
                       rodsArgs->physicalPathString );
        }
    }

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForCp: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                       DEST_RESC_NAME_KW, rodsArgs->resourceString );
        }
    }
    else if ( myRodsEnv != NULL && strlen( myRodsEnv->rodsDefResource ) > 0 ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, DEST_RESC_NAME_KW,
                   myRodsEnv->rodsDefResource );
    }

    if ( rodsArgs->rbudp == True ) {
        /* use -Q for rbudp transfer */
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                   RBUDP_TRANSFER_KW, "" );
        addKeyVal( &dataObjCopyInp->srcDataObjInp.condInput,
                   RBUDP_TRANSFER_KW, "" );
    }

    if ( rodsArgs->veryVerbose == True ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, VERY_VERBOSE_KW, "" );
        addKeyVal( &dataObjCopyInp->srcDataObjInp.condInput, VERY_VERBOSE_KW, "" );
    }

    if ( ( tmpStr = getenv( RBUDP_SEND_RATE_KW ) ) != NULL ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                   RBUDP_SEND_RATE_KW, tmpStr );
        addKeyVal( &dataObjCopyInp->srcDataObjInp.condInput,
                   RBUDP_SEND_RATE_KW, tmpStr );
    }

    if ( ( tmpStr = getenv( RBUDP_PACK_SIZE_KW ) ) != NULL ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                   RBUDP_PACK_SIZE_KW, tmpStr );
        addKeyVal( &dataObjCopyInp->srcDataObjInp.condInput,
                   RBUDP_PACK_SIZE_KW, tmpStr );
    }

    memset( rodsRestart, 0, sizeof( rodsRestart_t ) );
    if ( rodsArgs->restart == True ) {
        int status;
        status = openRestartFile( rodsArgs->restartFileString, rodsRestart );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "initCondForCp: openRestartFile of %s errno",
                          rodsArgs->restartFileString );
            return status;
        }
    }

    //dataObjCopyInp->destDataObjInp.createMode = 0600;		// seems unused, and caused https://github.com/irods/irods/issues/2085
    dataObjCopyInp->destDataObjInp.openFlags = O_WRONLY;

    return 0;
}

int
cpCollUtil( rcComm_t *conn, char *srcColl, char *targColl,
            rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
            dataObjCopyInp_t *dataObjCopyInp, rodsRestart_t *rodsRestart ) {
    int status = 0;
    int savedStatus = 0;
    char parPath[MAX_NAME_LEN], childPath[MAX_NAME_LEN];
    collHandle_t collHandle;
    collEnt_t collEnt;
    char srcChildPath[MAX_NAME_LEN], targChildPath[MAX_NAME_LEN];
    dataObjCopyInp_t childDataObjCopyInp;

    if ( srcColl == NULL || targColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "cpCollUtil: NULL srcColl or targColl in cp" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "cpCollUtil: -r option must be used for copying %s directory",
                 srcColl );
        return USER_INPUT_OPTION_ERR;
    }

    status = rclOpenCollection( conn, srcColl, 0, &collHandle );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "cpCollUtil: rclOpenCollection of %s error. status = %d",
                 srcColl, status );
        return status;
    }
    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            snprintf( srcChildPath, MAX_NAME_LEN, "%s/%s",
                      collEnt.collName, collEnt.dataName );
            snprintf( targChildPath, MAX_NAME_LEN, "%s/%s",
                      targColl, collEnt.dataName );

            status = chkStateForResume( conn, rodsRestart, targChildPath,
                                        rodsArgs, DATA_OBJ_T, &dataObjCopyInp->destDataObjInp.condInput,
                                        1 );

            if ( status < 0 ) {
                /* restart failed */
                break;
            }
            else if ( status == 0 ) {
                continue;
            }

            status = cpFileUtil( conn, srcChildPath, targChildPath,
                                 collEnt.dataSize, rodsArgs, dataObjCopyInp );

            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "getCollUtil: getDataObjUtil failed for %s. status = %d",
                              srcChildPath, status );
                if ( rodsRestart->fd > 0 ) {
                    break;
                }
                else {
                    savedStatus = status;
                }
            }
            else {
                status = procAndWrriteRestartFile( rodsRestart, targChildPath );
            }
        }
        else if ( collEnt.objType == COLL_OBJ_T ) {
            if ( ( status = splitPathByKey(
                                collEnt.collName, parPath, MAX_NAME_LEN, childPath, MAX_NAME_LEN, '/' ) ) < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "cpCollUtil:: splitPathByKey for %s error, status = %d",
                              collEnt.collName, status );
                return status;
            }

            snprintf( targChildPath, MAX_NAME_LEN, "%s/%s",
                      targColl, childPath );
            mkCollR( conn, targColl, targChildPath );

            if ( rodsArgs->verbose == True ) {
                fprintf( stdout, "C- %s:\n", targChildPath );
            }

            /* the child is a spec coll. need to drill down */
            childDataObjCopyInp = *dataObjCopyInp;
            if ( collEnt.specColl.collClass != NO_SPEC_COLL )
                childDataObjCopyInp.srcDataObjInp.specColl =
                    &collEnt.specColl;
            status = cpCollUtil( conn, collEnt.collName,
                                 targChildPath, myRodsEnv, rodsArgs, &childDataObjCopyInp,
                                 rodsRestart );

            if ( status < 0 && status != CAT_NO_ROWS_FOUND &&
                    status != SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
                savedStatus = status;
            }
        }
    }
    rclCloseCollection( &collHandle );

    if ( savedStatus < 0 ) {
        return savedStatus;
    }
    else if ( status == CAT_NO_ROWS_FOUND ||
              status == SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
        return 0;
    }
    else {
        return status;
    }
}

