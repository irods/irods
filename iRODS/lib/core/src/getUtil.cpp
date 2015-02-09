/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"
#include "lsUtil.hpp"
#include "getUtil.hpp"
#include "miscUtil.hpp"
#include "rcPortalOpr.hpp"
#include "sockComm.hpp"

int
setSessionTicket( rcComm_t *myConn, char *ticket ) {
    ticketAdminInp_t ticketAdminInp;
    int status;

    ticketAdminInp.arg1 = "session";
    ticketAdminInp.arg2 = ticket;
    ticketAdminInp.arg3 = "";
    ticketAdminInp.arg4 = "";
    ticketAdminInp.arg5 = "";
    ticketAdminInp.arg6 = "";
    status = rcTicketAdmin( myConn, &ticketAdminInp );
    if ( status != 0 ) {
        printf( "set ticket error %d \n", status );
    }
    return status;
}

int
getUtil( rcComm_t **myConn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
         rodsPathInp_t *rodsPathInp ) {
    int i = 0;
    int status = 0;
    int savedStatus = 0;
    rodsPath_t *targPath = 0;
    dataObjInp_t dataObjOprInp;
    rodsRestart_t rodsRestart;
    rcComm_t *conn = *myConn;

    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( myRodsArgs->ticket == True ) {
        if ( myRodsArgs->ticketString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForPut: NULL ticketString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            setSessionTicket( conn, myRodsArgs->ticketString );
        }
    }

    initCondForGet( conn, myRodsArgs, &dataObjOprInp, &rodsRestart );

    if ( rodsPathInp->resolved == False ) {
        status = resolveRodsTarget( conn, rodsPathInp, 1 );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "getUtil: resolveRodsTarget" );
            return status;
        }
        rodsPathInp->resolved = True;
    }

    /* initialize the progress struct */
    if ( gGuiProgressCB != NULL ) {
        bzero( &conn->operProgress, sizeof( conn->operProgress ) );
        for ( i = 0; i < rodsPathInp->numSrc; i++ ) {
            targPath = &rodsPathInp->targPath[i];
            if ( targPath->objType == LOCAL_FILE_T ) {
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

    if ( conn->fileRestart.flags == FILE_RESTART_ON ) {
        fileRestartInfo_t *info;
        status = readLfRestartFile( conn->fileRestart.infoFile, &info );
        if ( status >= 0 ) {
            status = lfRestartGetWithInfo( conn, info );
            if ( status >= 0 ) {
                /* save info so we know what got restarted and what not to
                 * delete in setStateForResume */
                rstrcpy( conn->fileRestart.info.objPath, info->objPath,
                         MAX_NAME_LEN );
                rstrcpy( conn->fileRestart.info.fileName, info->fileName,
                         MAX_NAME_LEN );
                conn->fileRestart.info.status = FILE_RESTARTED;
                printf( "%s was restarted successfully\n",
                        conn->fileRestart.info.objPath );
                unlink( conn->fileRestart.infoFile );
            }
            free( info );
        }
    }

    for ( i = 0; i < rodsPathInp->numSrc; i++ ) {
        targPath = &rodsPathInp->targPath[i];

        if ( rodsPathInp->srcPath[i].rodsObjStat != NULL &&
                rodsPathInp->srcPath[i].rodsObjStat->specColl != NULL ) {
            dataObjOprInp.specColl =
                rodsPathInp->srcPath[i].rodsObjStat->specColl;
        }
        else {
            dataObjOprInp.specColl = NULL;
        }
        if ( targPath->objType == LOCAL_FILE_T ) {
            rmKeyVal( &dataObjOprInp.condInput, TRANSLATED_PATH_KW );
            status = getDataObjUtil( conn, rodsPathInp->srcPath[i].outPath,
                                     targPath->outPath, rodsPathInp->srcPath[i].size,
                                     rodsPathInp->srcPath[i].objMode,
                                     myRodsArgs, &dataObjOprInp );
        }
        else if ( targPath->objType ==  LOCAL_DIR_T ) {
            setStateForRestart( &rodsRestart, targPath, myRodsArgs );
            addKeyVal( &dataObjOprInp.condInput, TRANSLATED_PATH_KW, "" );
            status = getCollUtil( myConn, rodsPathInp->srcPath[i].outPath,
                                  targPath->outPath, myRodsEnv, myRodsArgs, &dataObjOprInp,
                                  &rodsRestart );
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "getUtil: invalid get dest objType %d for %s",
                     targPath->objType, targPath->outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "getUtil: get error for %s",
                          targPath->outPath );
            savedStatus = status;
            break;
        }
    }

    if ( rodsRestart.fd > 0 ) {
        close( rodsRestart.fd );
    }
    if ( savedStatus < 0 ) {
        status = savedStatus;
    }
    else if ( status == CAT_NO_ROWS_FOUND ) {
        status = 0;
    }

    if ( status < 0 && myRodsArgs->retries == True ) {
        int reconnFlag;
        /* this is recursive. Only do it the first time */
        myRodsArgs->retries = False;
        if ( myRodsArgs->reconnect == True ) {
            reconnFlag = RECONN_TIMEOUT;
        }
        else {
            reconnFlag = NO_RECONN;
        }
        while ( myRodsArgs->retriesValue > 0 ) {
            rErrMsg_t errMsg;
            bzero( &errMsg, sizeof( errMsg ) );
            status = rcReconnect( myConn, myRodsEnv->rodsHost, myRodsEnv,
                                  reconnFlag );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "getUtil: rcReconnect error for %s", targPath->outPath );
                return status;
            }
            status = getUtil( myConn,  myRodsEnv, myRodsArgs, rodsPathInp );
            if ( status >= 0 ) {
                printf( "Retry get successful\n" );
                break;
            }
            else {
                rodsLogError( LOG_ERROR, status,
                              "getUtil: retry getUtil error" );
            }
            myRodsArgs->retriesValue--;
        }
    }
    return status;
}

int
getDataObjUtil( rcComm_t *conn, char *srcPath, char *targPath,
                rodsLong_t srcSize, uint dataMode,
                rodsArguments_t *rodsArgs, dataObjInp_t *dataObjOprInp ) {
    int status;
    struct timeval startTime, endTime;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "getDataObjUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( conn->fileRestart.info.status == FILE_RESTARTED &&
            strcmp( conn->fileRestart.info.objPath, srcPath ) == 0 ) {
        /* it was restarted */
        conn->fileRestart.info.status = FILE_NOT_RESTART;
        return 0;
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

    rstrcpy( dataObjOprInp->objPath, srcPath, MAX_NAME_LEN );
    /* rcDataObjGet verifies dataSize if given */
    if ( rodsArgs->replNum == True || rodsArgs->resource == True ) {
        /* don't verify because it may be an old copy and hence the size
         * could be wrong */
        dataObjOprInp->dataSize = 0;
    }
    else {
        dataObjOprInp->dataSize = srcSize;
    }

    status = rcDataObjGet( conn, dataObjOprInp, targPath );

    if ( status >= 0 ) {
        /* old objState use numCopies in place of dataMode.
         * Just a sanity check */
        myChmod( targPath, dataMode );
        if ( rodsArgs->verbose == True ) {
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, dataObjOprInp->objPath, srcSize, targPath,
                         &startTime, &endTime );
        }
        if ( gGuiProgressCB != NULL ) {
            conn->operProgress.totalNumFilesDone++;
            conn->operProgress.totalFileSizeDone += srcSize;
        }
    }

    return status;
}

int
initCondForGet( rcComm_t *conn, rodsArguments_t *rodsArgs,
                dataObjInp_t *dataObjOprInp, rodsRestart_t *rodsRestart ) {
    char *tmpStr;

    if ( dataObjOprInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForGet: NULL dataObjOprInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjOprInp, 0, sizeof( dataObjInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    dataObjOprInp->oprType = GET_OPR;

    if ( rodsArgs->force == True ) {
        addKeyVal( &dataObjOprInp->condInput, FORCE_FLAG_KW, "" );
    }

    if ( rodsArgs->verifyChecksum == True ) {
        addKeyVal( &dataObjOprInp->condInput, VERIFY_CHKSUM_KW, "" );
    }

#ifdef windows_platform
    dataObjOprInp->numThreads = NO_THREADING;
#else
    if ( rodsArgs->number == True ) {
        if ( rodsArgs->numberValue == 0 ) {
            dataObjOprInp->numThreads = NO_THREADING;
        }
        else {
            dataObjOprInp->numThreads = rodsArgs->numberValue;
        }
    }
#endif

    if ( rodsArgs->replNum == True ) {
        addKeyVal( &dataObjOprInp->condInput, REPL_NUM_KW,
                   rodsArgs->replNumValue );
    }

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForPut: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &dataObjOprInp->condInput, RESC_NAME_KW,
                       rodsArgs->resourceString );
        }
    }

    if ( rodsArgs->ticket == True ) {
        if ( rodsArgs->ticketString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForPut: NULL ticketString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &dataObjOprInp->condInput, TICKET_KW,
                       rodsArgs->ticketString );
        }
    }


    if ( rodsArgs->rbudp == True ) {
        /* use -Q for rbudp transfer */
        addKeyVal( &dataObjOprInp->condInput, RBUDP_TRANSFER_KW, "" );
    }

    if ( rodsArgs->veryVerbose == True ) {
        addKeyVal( &dataObjOprInp->condInput, VERY_VERBOSE_KW, "" );
    }

    if ( ( tmpStr = getenv( RBUDP_SEND_RATE_KW ) ) != NULL ) {
        addKeyVal( &dataObjOprInp->condInput, RBUDP_SEND_RATE_KW, tmpStr );
    }

    if ( ( tmpStr = getenv( RBUDP_PACK_SIZE_KW ) ) != NULL ) {
        addKeyVal( &dataObjOprInp->condInput, RBUDP_PACK_SIZE_KW, tmpStr );
    }

    if ( rodsArgs->purgeCache == True ) { // JMC - backport 4537
        addKeyVal( &dataObjOprInp->condInput, PURGE_CACHE_KW, "" );
    }

    memset( rodsRestart, 0, sizeof( rodsRestart_t ) );
    if ( rodsArgs->restart == True ) {
        int status;
        status = openRestartFile( rodsArgs->restartFileString, rodsRestart );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "initCondForPut: openRestartFile of %s errno",
                          rodsArgs->restartFileString );
            return status;
        }
    }

    if ( rodsArgs->retries == True && rodsArgs->restart == False &&
            rodsArgs->lfrestart == False ) {
        rodsLog( LOG_ERROR,
                 "initCondForGet: --retries must be used with -X option" );
        return USER_INPUT_OPTION_ERR;
    }

    if ( rodsArgs->lfrestart == True ) {
        if ( rodsArgs->rbudp == True ) {
            rodsLog( LOG_NOTICE,
                     "initCondForPut: --lfrestart cannot be used with -Q option" );
        }
        else {
            conn->fileRestart.flags = FILE_RESTART_ON;
            rstrcpy( conn->fileRestart.infoFile, rodsArgs->lfrestartFileString,
                     MAX_NAME_LEN );
        }
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4604
    if ( rodsArgs->rlock == True ) {
        addKeyVal( &dataObjOprInp->condInput, LOCK_TYPE_KW, READ_LOCK_TYPE );
    }
    // =-=-=-=-=-=-=-
    // =-=-=-=-=-=-=-
    // JMC - backport 4612
    if ( rodsArgs->wlock == True ) {
        rodsLog( LOG_ERROR, "initCondForPut: --wlock not supported, changing it to --rlock" );
        addKeyVal( &dataObjOprInp->condInput, LOCK_TYPE_KW, READ_LOCK_TYPE );
    }
    // =-=-=-=-=-=-=-
    dataObjOprInp->openFlags = O_RDONLY;

    return 0;
}

int
getCollUtil( rcComm_t **myConn, char *srcColl, char *targDir,
             rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, dataObjInp_t *dataObjOprInp,
             rodsRestart_t *rodsRestart ) {
    int status = 0;
    int savedStatus = 0;
    char srcChildPath[MAX_NAME_LEN], targChildPath[MAX_NAME_LEN];
    char parPath[MAX_NAME_LEN], childPath[MAX_NAME_LEN];
    collHandle_t collHandle;
    collEnt_t collEnt;
    dataObjInp_t childDataObjInp;
    rcComm_t *conn;

    if ( srcColl == NULL || targDir == NULL ) {
        rodsLog( LOG_ERROR,
                 "getCollUtil: NULL srcColl or targDir input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "getCollUtil: -r option must be used for getting %s collection",
                 targDir );
        return USER_INPUT_OPTION_ERR;
    }

    if ( rodsArgs->redirectConn == True ) {
        int reconnFlag;
        if ( rodsArgs->reconnect == True ) {
            reconnFlag = RECONN_TIMEOUT;
        }
        else {
            reconnFlag = NO_RECONN;
        }
        /* reconnect to the resource server */
        rstrcpy( dataObjOprInp->objPath, srcColl, MAX_NAME_LEN );
        redirectConnToRescSvr( myConn, dataObjOprInp, myRodsEnv, reconnFlag );
        rodsArgs->redirectConn = 0;    /* only do it once */
    }
    conn = *myConn;

    printCollOrDir( targDir, LOCAL_DIR_T, rodsArgs, dataObjOprInp->specColl );
    status = rclOpenCollection( conn, srcColl, 0, &collHandle );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getCollUtil: rclOpenCollection of %s error. status = %d",
                 srcColl, status );
        return status;
    }
    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            rodsLong_t mySize;

            mySize = collEnt.dataSize;    /* have to save it. May be freed */

            snprintf( targChildPath, MAX_NAME_LEN, "%s/%s",
                      targDir, collEnt.dataName );
            snprintf( srcChildPath, MAX_NAME_LEN, "%s/%s",
                      collEnt.collName, collEnt.dataName );

            status = chkStateForResume( conn, rodsRestart, targChildPath,
                                        rodsArgs, LOCAL_FILE_T, &dataObjOprInp->condInput, 1 );

            if ( status < 0 ) {
                /* restart failed */
                break;
            }
            else if ( status == 0 ) {
                continue;
            }

            status = getDataObjUtil( conn, srcChildPath, targChildPath, mySize,
                                     collEnt.dataMode, rodsArgs, dataObjOprInp );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "getCollUtil: getDataObjUtil failed for %s. status = %d",
                              srcChildPath, status );
                savedStatus = status;
                if ( rodsRestart->fd > 0 ) {
                    break;
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
                              "getCollUtil:: splitPathByKey for %s error, status = %d",
                              collEnt.collName, status );
                return status;
            }
            snprintf( targChildPath, MAX_NAME_LEN, "%s/%s",
                      targDir, childPath );

            mkdirR( targDir, targChildPath, 0750 );

            /* the child is a spec coll. need to drill down */
            childDataObjInp = *dataObjOprInp;
            if ( collEnt.specColl.collClass != NO_SPEC_COLL ) {
                childDataObjInp.specColl = &collEnt.specColl;
            }
            else {
                childDataObjInp.specColl = NULL;
            }
            status = getCollUtil( myConn, collEnt.collName, targChildPath,
                                  myRodsEnv, rodsArgs, &childDataObjInp, rodsRestart );
            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                rodsLogError( LOG_ERROR, status,
                              "getCollUtil: getCollUtil failed for %s. status = %d",
                              collEnt.collName, status );
                savedStatus = status;
                if ( rodsRestart->fd > 0 ) {
                    break;
                }
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

