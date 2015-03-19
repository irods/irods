/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"
#include "putUtil.hpp"
#include "miscUtil.hpp"
#include "rcPortalOpr.hpp"
#include <string>
#include <boost/filesystem.hpp>
#include "irods_server_properties.hpp"
#include "readServerConfig.hpp"

#include "sockComm.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

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
putUtil( rcComm_t **myConn, rodsEnv *myRodsEnv,
         rodsArguments_t *myRodsArgs, rodsPathInp_t *rodsPathInp ) {
    int i;
    int status;
    int savedStatus = 0;
    rodsPath_t *targPath = 0;
    dataObjInp_t dataObjOprInp;
    bulkOprInp_t bulkOprInp;
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

    status = initCondForPut( conn, myRodsEnv, myRodsArgs, &dataObjOprInp,
                             &bulkOprInp, &rodsRestart );

    if ( status < 0 ) {
        return status;
    }

    if ( rodsPathInp->resolved == False ) {
        status = resolveRodsTarget( conn, rodsPathInp, 1 );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "putUtil: resolveRodsTarget error, status = %d", status );
            return status;
        }
        rodsPathInp->resolved = True;
    }

    /* initialize the progress struct */
    if ( gGuiProgressCB != NULL ) {
        bzero( &conn->operProgress, sizeof( conn->operProgress ) );
        for ( i = 0; i < rodsPathInp->numSrc; i++ ) {
            targPath = &rodsPathInp->targPath[i];
            if ( targPath->objType == DATA_OBJ_T ) {
                conn->operProgress.totalNumFiles++;
                if ( rodsPathInp->srcPath[i].size > 0 ) {
                    conn->operProgress.totalFileSize +=
                        rodsPathInp->srcPath[i].size;
                }
            }
            else {
                getDirSizeForProgStat( myRodsArgs,
                                       rodsPathInp->srcPath[i].outPath, &conn->operProgress );
            }
        }
    }

    if ( conn->fileRestart.flags == FILE_RESTART_ON ) {
        fileRestartInfo_t *info;
        status = readLfRestartFile( conn->fileRestart.infoFile, &info );
        if ( status >= 0 ) {
            status = lfRestartPutWithInfo( conn, info );
            if ( status >= 0 ) {
                /* save info so we know what got restarted */
                rstrcpy( conn->fileRestart.info.objPath, info->objPath,
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

        if ( targPath->objType == DATA_OBJ_T ) {
            if ( isPathSymlink( myRodsArgs, rodsPathInp->srcPath[i].outPath ) > 0 ) {
                continue;
            }
            dataObjOprInp.createMode = rodsPathInp->srcPath[i].objMode;
            status = putFileUtil( conn, rodsPathInp->srcPath[i].outPath,
                                  targPath->outPath, rodsPathInp->srcPath[i].size,
                                  myRodsArgs, &dataObjOprInp );
        }
        else if ( targPath->objType == COLL_OBJ_T ) {
            setStateForRestart( &rodsRestart, targPath, myRodsArgs );
            if ( myRodsArgs->bulk == True ) {
                status = bulkPutDirUtil( myConn,
                                         rodsPathInp->srcPath[i].outPath, targPath->outPath,
                                         myRodsEnv, myRodsArgs, &dataObjOprInp, &bulkOprInp,
                                         &rodsRestart );
            }
            else {
                status = putDirUtil( myConn, rodsPathInp->srcPath[i].outPath,
                                     targPath->outPath, myRodsEnv, myRodsArgs, &dataObjOprInp,
                                     &bulkOprInp, &rodsRestart, NULL );
            }
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "putUtil: invalid put dest objType %d for %s",
                     targPath->objType, targPath->outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "putUtil: put error for %s, status = %d",
                          targPath->outPath, status );
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
                              "putUtil: rcReconnect error for %s", targPath->outPath );
                return status;
            }
            status = putUtil( myConn,  myRodsEnv, myRodsArgs, rodsPathInp );
            if ( status >= 0 ) {
                printf( "Retry put successful\n" );
                break;
            }
            else {
                rodsLogError( LOG_ERROR, status,
                              "putUtil: retry putUtil error" );
            }
            myRodsArgs->retriesValue--;
        }
    }
    return status;
}

int
putFileUtil( rcComm_t *conn, char *srcPath, char *targPath, rodsLong_t srcSize,
             rodsArguments_t *rodsArgs, dataObjInp_t *dataObjOprInp ) {
    int status;
    struct timeval startTime, endTime;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "putFileUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( conn->fileRestart.info.status == FILE_RESTARTED &&
            strcmp( conn->fileRestart.info.objPath, targPath ) == 0 ) {
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

    /* have to take care of checksum here since it needs to be recalculated */
    if ( rodsArgs->checksum == True ) {
        // set the expected flag to indicate that we want a server-side
        // checksum computed and stored in the catalog
        addKeyVal( &dataObjOprInp->condInput, REG_CHKSUM_KW, "" );

    }
    else if ( rodsArgs->verifyChecksum == True ) {
        rodsEnv env;
        int ret = getRodsEnv( &env );
        if ( ret < 0 ) {
            rodsLog(
                LOG_ERROR,
                "putFileUtil - failed to capture rods env %d",
                ret );
            return ret;
        }

        status = rcChksumLocFile( srcPath,
                                  VERIFY_CHKSUM_KW,
                                  &dataObjOprInp->condInput,
                                  env.rodsDefaultHashScheme );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "putFileUtil: rcChksumLocFile error for %s, status = %d",
                          srcPath, status );
            return status;
        }
    }
    if ( strlen( targPath ) >= MAX_PATH_ALLOWED - 1 ) {
        return USER_PATH_EXCEEDS_MAX;
    }
    rstrcpy( dataObjOprInp->objPath, targPath, MAX_NAME_LEN );
    dataObjOprInp->dataSize = srcSize;
    status = rcDataObjPut( conn, dataObjOprInp, srcPath );

    if ( status >= 0 ) {
        if ( rodsArgs->verbose == True ) {
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, dataObjOprInp->objPath, srcSize, srcPath,
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
initCondForPut( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                dataObjInp_t *dataObjOprInp, bulkOprInp_t *bulkOprInp,
                rodsRestart_t *rodsRestart ) {
    char *tmpStr;

    if ( rodsArgs == NULL ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR, "initCondForPut :: NULL rodsArgs" );
        return -1;
    }

    if ( dataObjOprInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForPut: NULL dataObjOprInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjOprInp, 0, sizeof( dataObjInp_t ) );

    if ( rodsArgs->bulk == True ) {
        if ( bulkOprInp == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForPut: NULL bulkOprInp input" );
            return USER__NULL_INPUT_ERR;
        }
        bzero( bulkOprInp, sizeof( bulkOprInp_t ) );
        if ( rodsArgs->checksum == True ) {
            addKeyVal( &bulkOprInp->condInput, REG_CHKSUM_KW, "" );
        }
        else if ( rodsArgs->verifyChecksum == True ) {
            addKeyVal( &bulkOprInp->condInput, VERIFY_CHKSUM_KW, "" );
        }
        initAttriArrayOfBulkOprInp( bulkOprInp );
    }

    dataObjOprInp->oprType = PUT_OPR;

    if ( rodsArgs->all == True ) {
        addKeyVal( &dataObjOprInp->condInput, ALL_KW, "" );
    }

    if ( rodsArgs->dataType == True ) {
        if ( rodsArgs->dataTypeString == NULL ) {
            addKeyVal( &dataObjOprInp->condInput, DATA_TYPE_KW, "generic" );
        }
        else {
            if ( strcmp( rodsArgs->dataTypeString, "t" ) == 0 ||
                    strcmp( rodsArgs->dataTypeString, "tar" ) == 0 ) {
                addKeyVal( &dataObjOprInp->condInput, DATA_TYPE_KW,
                           "tar file" );
            }
            else {
                addKeyVal( &dataObjOprInp->condInput, DATA_TYPE_KW,
                           rodsArgs->dataTypeString );
            }
        }
    }
    else {
        addKeyVal( &dataObjOprInp->condInput, DATA_TYPE_KW, "generic" );
    }

    if ( rodsArgs->force == True ) {
        addKeyVal( &dataObjOprInp->condInput, FORCE_FLAG_KW, "" );
        if ( rodsArgs->bulk == True ) {
            addKeyVal( &bulkOprInp->condInput, FORCE_FLAG_KW, "" );
        }
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

    if ( rodsArgs->physicalPath == True ) {
        if ( rodsArgs->physicalPathString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForPut: NULL physicalPathString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            boost::filesystem::path slash( "/" );
            std::string preferred_slash = slash.make_preferred().native();
            if ( preferred_slash[0] != rodsArgs->physicalPathString[0] ) {
                rodsLog(
                    LOG_ERROR,
                    "initCondForPut: physical path [%s] must be absolute, not relative",
                    rodsArgs->physicalPathString );
                return USER_INPUT_PATH_ERR;
            }

            addKeyVal( &dataObjOprInp->condInput, FILE_PATH_KW,
                       rodsArgs->physicalPathString );
        }
    }

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForPut: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &dataObjOprInp->condInput, DEST_RESC_NAME_KW,
                       rodsArgs->resourceString );
            if ( rodsArgs->bulk == True ) {
                addKeyVal( &bulkOprInp->condInput, DEST_RESC_NAME_KW,
                           rodsArgs->resourceString );
            }

        }
    }
    else if ( myRodsEnv != NULL && strlen( myRodsEnv->rodsDefResource ) > 0 ) {
        /* use rodsDefResource but set the DEF_RESC_NAME_KW instead.
         * Used by dataObjCreate. Will only make a new replica only if
         * DEST_RESC_NAME_KW is set */
        addKeyVal( &dataObjOprInp->condInput, DEF_RESC_NAME_KW,
                   myRodsEnv->rodsDefResource );
        if ( rodsArgs->bulk == True ) {
            addKeyVal( &bulkOprInp->condInput, DEF_RESC_NAME_KW,
                       myRodsEnv->rodsDefResource );
        }
    }

    if ( rodsArgs->replNum == True ) {
        addKeyVal( &dataObjOprInp->condInput, REPL_NUM_KW,
                   rodsArgs->replNumValue );
    }

    if ( rodsArgs->purgeCache == True ) { // JMC - backport 4537
        addKeyVal( &dataObjOprInp->condInput, PURGE_CACHE_KW, "" );
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

    memset( rodsRestart, 0, sizeof( rodsRestart_t ) );
    if ( rodsArgs->restart == True ) {
        int status;
        status = openRestartFile( rodsArgs->restartFileString, rodsRestart );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "initCondForPut: openRestartFile of %s error",
                          rodsArgs->restartFileString );
            return status;
        }
        if ( rodsRestart->doneCnt == 0 ) {
            /* brand new */
            if ( rodsArgs->bulk == True ) {
                rstrcpy( rodsRestart->oprType, BULK_OPR_KW, NAME_LEN );
            }
            else {
                rstrcpy( rodsRestart->oprType, NON_BULK_OPR_KW, NAME_LEN );
            }
        }
        else {
            if ( rodsArgs->bulk == True ) {
                if ( strcmp( rodsRestart->oprType, BULK_OPR_KW ) != 0 ) {
                    rodsLog( LOG_ERROR,
                             "initCondForPut: -b cannot be used to restart this iput" );
                    return BULK_OPR_MISMATCH_FOR_RESTART;
                }
            }
            else {
                if ( strcmp( rodsRestart->oprType, NON_BULK_OPR_KW ) != 0 ) {
                    rodsLog( LOG_ERROR,
                             "initCondForPut: -b must be used to restart this iput" );
                    return BULK_OPR_MISMATCH_FOR_RESTART;
                }
            }
        }
    }
    if ( rodsArgs->retries == True && rodsArgs->restart == False &&
            rodsArgs->lfrestart == False ) {

        rodsLog( LOG_ERROR,
                 "initCondForPut: --retries must be used with -X or --lfrestart option" );

        return USER_INPUT_OPTION_ERR;
    }

    /* Not needed - dataObjOprInp->createMode = 0700; */
    /* mmap in rbudp needs O_RDWR */
    dataObjOprInp->openFlags = O_RDWR;

    if ( rodsArgs->lfrestart == True ) {
        if ( rodsArgs->bulk == True ) {
            rodsLog( LOG_NOTICE,
                     "initCondForPut: --lfrestart cannot be used with -b option" );
        }
        else if ( rodsArgs->rbudp == True ) {
            rodsLog( LOG_NOTICE,
                     "initCondForPut: --lfrestart cannot be used with -Q option" );
        }
        else {
            conn->fileRestart.flags = FILE_RESTART_ON;
            rstrcpy( conn->fileRestart.infoFile, rodsArgs->lfrestartFileString,
                     MAX_NAME_LEN );
        }
    }

    if ( rodsArgs->wlock == True ) { // JMC - backport 4604
        addKeyVal( &dataObjOprInp->condInput, LOCK_TYPE_KW, WRITE_LOCK_TYPE );
    }
    if ( rodsArgs->rlock == True ) { // JMC - backport 4612
        rodsLog( LOG_ERROR, "initCondForPut: --rlock not supported, changing it to --wlock" );
        addKeyVal( &dataObjOprInp->condInput, LOCK_TYPE_KW, WRITE_LOCK_TYPE );
    }

    return 0;
}

int
putDirUtil( rcComm_t **myConn, char *srcDir, char *targColl,
            rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, dataObjInp_t *dataObjOprInp,
            bulkOprInp_t *bulkOprInp, rodsRestart_t *rodsRestart,
            bulkOprInfo_t *bulkOprInfo ) {
    char srcChildPath[MAX_NAME_LEN], targChildPath[MAX_NAME_LEN];

    if ( srcDir == NULL || targColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "putDirUtil: NULL srcDir or targColl input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( isPathSymlink( rodsArgs, srcDir ) > 0 ) {
        return 0;
    }

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "putDirUtil: -r option must be used for putting %s directory",
                 srcDir );
        return USER_INPUT_OPTION_ERR;
    }

    if ( rodsArgs->redirectConn == True && rodsArgs->force != True ) {
        int reconnFlag;
        if ( rodsArgs->reconnect == True ) {
            reconnFlag = RECONN_TIMEOUT;
        }
        else {
            reconnFlag = NO_RECONN;
        }
        /* reconnect to the resource server */
        rstrcpy( dataObjOprInp->objPath, targColl, MAX_NAME_LEN );
        redirectConnToRescSvr( myConn, dataObjOprInp, myRodsEnv, reconnFlag );
        rodsArgs->redirectConn = 0;    /* only do it once */
    }

    rcComm_t *conn = *myConn;
    boost::filesystem::path srcDirPath( srcDir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        rodsLog( LOG_ERROR,
                 "putDirUtil: opendir local dir error for %s, errno = %d\n",
                 srcDir, errno );
        return USER_INPUT_PATH_ERR;
    }

    if ( rodsArgs->verbose == True ) {
        fprintf( stdout, "C- %s:\n", targColl );
    }

    int bulkFlag = NON_BULK_OPR;
    if ( bulkOprInfo != NULL ) {
        bulkFlag = bulkOprInfo->flags;
    }

    int savedStatus = 0;
    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
    for ( boost::filesystem::directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        boost::filesystem::path p = itr->path();
        snprintf( srcChildPath, MAX_NAME_LEN, "%s",
                  p.c_str() );

        if ( isPathSymlink( rodsArgs, srcChildPath ) > 0 ) {
            continue;
        }
        if ( !exists( p ) ) {
            rodsLog( LOG_ERROR,
                     "putDirUtil: stat error for %s, errno = %d\n",
                     srcChildPath, errno );
            return USER_INPUT_PATH_ERR;
        }

        if ( is_symlink( p ) ) {
            boost::filesystem::path cp = read_symlink( p );
            snprintf( srcChildPath, MAX_NAME_LEN, "%s/%s",
                      srcDir, cp.c_str() );
            p = boost::filesystem::path( srcChildPath );
        }
        rodsLong_t dataSize = 0;
        dataObjOprInp->createMode = getPathStMode( p.c_str() );
        objType_t childObjType;
        if ( is_regular_file( p ) ) {
            childObjType = DATA_OBJ_T;
            dataSize = file_size( p );
        }
        else if ( is_directory( p ) ) {
            childObjType = COLL_OBJ_T;
        }
        else {
            rodsLog( LOG_ERROR,
                     "putDirUtil: unknown local path type for %s",
                     srcChildPath );
            savedStatus = USER_INPUT_PATH_ERR;
            continue;
        }
        boost::filesystem::path childPath = p.filename();
        snprintf( targChildPath, MAX_NAME_LEN, "%s/%s",
                  targColl, childPath.c_str() );
        if ( childObjType == DATA_OBJ_T ) {
            if ( bulkFlag == BULK_OPR_SMALL_FILES &&
                    file_size( p ) > MAX_BULK_OPR_FILE_SIZE ) {
                continue;
            }
            else if ( bulkFlag == BULK_OPR_LARGE_FILES &&
                      file_size( p ) <= MAX_BULK_OPR_FILE_SIZE ) {
                continue;
            }
        }

        int status = chkStateForResume( conn, rodsRestart, targChildPath,
                                        rodsArgs, childObjType, &dataObjOprInp->condInput, 1 );

        if ( status < 0 ) {
            /* restart failed */
            return status;
        }
        else if ( status == 0 ) {
            if ( bulkFlag == BULK_OPR_SMALL_FILES &&
                    ( rodsRestart->restartState & LAST_PATH_MATCHED ) != 0 ) {
                /* enable foreFlag one time */
                setForceFlagForRestart( bulkOprInp, bulkOprInfo );
            }
            continue;
        }

        if ( childObjType == DATA_OBJ_T ) {   /* a file */
            if ( bulkFlag == BULK_OPR_SMALL_FILES ) {
                status = bulkPutFileUtil( conn, srcChildPath, targChildPath,
                                          dataSize,  dataObjOprInp->createMode, rodsArgs,
                                          bulkOprInp, bulkOprInfo );
            }
            else {
                /* normal put */
                status = putFileUtil( conn, srcChildPath, targChildPath,
                                      dataSize, rodsArgs, dataObjOprInp );
            }
            if ( rodsRestart->fd > 0 ) {
                if ( status >= 0 ) {
                    if ( bulkFlag == BULK_OPR_SMALL_FILES ) {
                        if ( status > 0 ) {
                            /* status is the number of files bulk loaded */
                            rodsRestart->curCnt += status;
                            status = writeRestartFile( rodsRestart,
                                                       targChildPath );
                        }
                    }
                    else {
                        /* write the restart file */
                        rodsRestart->curCnt ++;
                        status = writeRestartFile( rodsRestart, targChildPath );
                    }
                }
            }
        }
        else {        /* a directory */
            status = mkColl( conn, targChildPath );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "putDirUtil: mkColl error for %s", targChildPath );
            }
            status = putDirUtil( myConn, srcChildPath, targChildPath,
                                 myRodsEnv, rodsArgs, dataObjOprInp, bulkOprInp,
                                 rodsRestart, bulkOprInfo );

        }

        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLogError( LOG_ERROR, status,
                          "putDirUtil: put %s failed. status = %d",
                          srcChildPath, status );
            savedStatus = status;
            if ( rodsRestart->fd > 0 ) {
                break;
            }
        }
    }
    return savedStatus;
}

int
bulkPutDirUtil( rcComm_t **myConn, char *srcDir, char *targColl,
                rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, dataObjInp_t *dataObjOprInp,
                bulkOprInp_t *bulkOprInp, rodsRestart_t *rodsRestart ) {
    int status;
    bulkOprInfo_t bulkOprInfo;

    /* do large files first */
    bzero( &bulkOprInfo, sizeof( bulkOprInfo ) );
    bulkOprInfo.flags = BULK_OPR_LARGE_FILES;

    status = putDirUtil( myConn, srcDir, targColl, myRodsEnv, rodsArgs,
                         dataObjOprInp, bulkOprInp, rodsRestart, &bulkOprInfo );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "bulkPutDirUtil: Large files bulkPut error for %s", srcDir );
        return status;
    }

    /* now bulk put the small files */
    bzero( &bulkOprInfo, sizeof( bulkOprInfo ) );
    bulkOprInfo.flags = BULK_OPR_SMALL_FILES;
    rstrcpy( dataObjOprInp->objPath, targColl, MAX_NAME_LEN );
    rstrcpy( bulkOprInp->objPath, targColl, MAX_NAME_LEN );
    bulkOprInfo.bytesBuf.len = 0;
    bulkOprInfo.bytesBuf.buf = malloc( BULK_OPR_BUF_SIZE );

    status = putDirUtil( myConn, srcDir, targColl, myRodsEnv, rodsArgs,
                         dataObjOprInp, bulkOprInp, rodsRestart, &bulkOprInfo );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "bulkPutDirUtil: Small files bulkPut error for %s", srcDir );
        return status;
    }

    if ( bulkOprInfo.count > 0 ) {
        status = sendBulkPut( *myConn, bulkOprInp, &bulkOprInfo, rodsArgs );
        if ( status >= 0 ) {
            if ( rodsRestart->fd > 0 ) {
                rodsRestart->curCnt += bulkOprInfo.count;
                /* last entry. */
                writeRestartFile( rodsRestart, bulkOprInfo.cachedTargPath );
            }
        }
        else {
            rodsLogError( LOG_ERROR, status,
                          "bulkPutDirUtil: tarAndBulkPut error for %s",
                          bulkOprInfo.phyBunDir );
        }
        clearBulkOprInfo( &bulkOprInfo );
        if ( bulkOprInfo.bytesBuf.buf != NULL ) {
            free( bulkOprInfo.bytesBuf.buf );
            bulkOprInfo.bytesBuf.buf = NULL;
        }

    }
    return status;
}

int
getPhyBunDir( char *phyBunRootDir, char *userName, char *outPhyBunDir ) {
    while ( 1 ) {
        snprintf( outPhyBunDir, MAX_NAME_LEN, "%s/%s.phybun.%d", phyBunRootDir,
                  userName, ( int ) random() );
        boost::filesystem::path p( outPhyBunDir );
        if ( !exists( p ) ) {
            break;
        }
    }
    return 0;
}

int
bulkPutFileUtil( rcComm_t *conn, char *srcPath, char *targPath,
                 rodsLong_t srcSize, int createMode,
                 rodsArguments_t *rodsArgs, bulkOprInp_t *bulkOprInp,
                 bulkOprInfo_t *bulkOprInfo ) {
    int status;
    int in_fd;
    int bytesRead = 0;
    /*#ifndef windows_platform
      char *bufPtr = bulkOprInfo->bytesBuf.buf + bulkOprInfo->size;
    #else*/  /* make change for Windows only */
    char *bufPtr = ( char * )( bulkOprInfo->bytesBuf.buf ) + bulkOprInfo->size;
    /*#endif*/

#ifdef windows_platform
    in_fd = iRODSNt_bopen( srcPath, O_RDONLY, 0 );
#else
    in_fd = open( srcPath, O_RDONLY, 0 );
#endif
    if ( in_fd < 0 ) { /* error */
        status = USER_FILE_DOES_NOT_EXIST - errno;
        rodsLog( LOG_ERROR,
                 "bulkPutFileUtilcannot open file %s, status = %d", srcPath, status );
        return status;
    }

    status = myRead( in_fd, bufPtr, srcSize, &bytesRead, NULL );
    if ( status != srcSize ) {
        if ( status >= 0 ) {
            status = SYS_COPY_LEN_ERR - errno;
            rodsLogError( LOG_ERROR, status, "bulkPutFileUtil: bytesRead %ju does not match srcSize %ju for %s",
                          ( uintmax_t )bytesRead, ( uintmax_t )srcSize, srcPath );
        }
        else {
            status = USER_INPUT_PATH_ERR - errno;
        }
        return status;
    }
    close( in_fd );
    rstrcpy( bulkOprInfo->cachedTargPath, targPath, MAX_NAME_LEN );
    bulkOprInfo->count++;
    bulkOprInfo->size += srcSize;
    bulkOprInfo->bytesBuf.len = bulkOprInfo->size;

    if ( getValByKey( &bulkOprInp->condInput, REG_CHKSUM_KW ) != NULL ||
            getValByKey( &bulkOprInp->condInput, VERIFY_CHKSUM_KW ) != NULL ) {
        char chksumStr[NAME_LEN];

        rodsEnv env;
        int ret = getRodsEnv( &env );
        if ( ret < 0 ) {
            rodsLog(
                LOG_ERROR,
                "bulkPutFileUtil: error getting rods env %d",
                ret );
            return ret;
        }
        status = chksumLocFile( srcPath, chksumStr, env.rodsDefaultHashScheme );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "bulkPutFileUtil: chksumLocFile error for %s ", srcPath );
            return status;
        }
        status = fillAttriArrayOfBulkOprInp( targPath, createMode, chksumStr,
                                             bulkOprInfo->size, bulkOprInp );
    }
    else {
        status = fillAttriArrayOfBulkOprInp( targPath, createMode, NULL,
                                             bulkOprInfo->size, bulkOprInp );
    }
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "bulkPutFileUtil: fillAttriArrayOfBulkOprInp error for %s",
                      srcPath );
        return status;
    }
    if ( bulkOprInfo->count >= MAX_NUM_BULK_OPR_FILES ||
            bulkOprInfo->size >= BULK_OPR_BUF_SIZE -
            MAX_BULK_OPR_FILE_SIZE ) {
        /* tar send it */
        status = sendBulkPut( conn, bulkOprInp, bulkOprInfo, rodsArgs );
        if ( status >= 0 ) {
            /* return the count */
            status = bulkOprInfo->count;
        }
        else {
            rodsLogError( LOG_ERROR, status,
                          "bulkPutFileUtil: tarAndBulkPut error for %s", srcPath );
        }
        clearBulkOprInfo( bulkOprInfo );
    }
    return status;
}

int
sendBulkPut( rcComm_t *conn, bulkOprInp_t *bulkOprInp,
             bulkOprInfo_t *bulkOprInfo, rodsArguments_t *rodsArgs ) {
    struct timeval startTime, endTime;
    int status = 0;

    if ( bulkOprInfo == NULL || bulkOprInfo->count <= 0 ) {
        return 0;
    }

    if ( rodsArgs->verbose == True ) {
        ( void ) gettimeofday( &startTime, ( struct timezone * )0 );
    }

    /* send it */
    if ( bulkOprInfo->bytesBuf.buf != NULL ) {
        status = rcBulkDataObjPut( conn, bulkOprInp, &bulkOprInfo->bytesBuf );
    }
    /* reset the row count */
    bulkOprInp->attriArray.rowCnt = 0;
    if ( bulkOprInfo->forceFlagAdded == 1 ) {
        rmKeyVal( &bulkOprInp->condInput, FORCE_FLAG_KW );
        bulkOprInfo->forceFlagAdded = 0;
    }
    if ( status >= 0 ) {
        if ( rodsArgs->verbose == True ) {
            printf( "Bulk upload %d files.\n", bulkOprInfo->count );
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, bulkOprInfo->cachedTargPath,
                         bulkOprInfo->size, bulkOprInfo->cachedTargPath,
                         &startTime, &endTime );
        }
        if ( gGuiProgressCB != NULL ) {
            rstrcpy( conn->operProgress.curFileName,
                     bulkOprInfo->cachedTargPath, MAX_NAME_LEN );
            conn->operProgress.totalNumFilesDone +=  bulkOprInfo->count;
            conn->operProgress.totalFileSizeDone += bulkOprInfo->size;
            gGuiProgressCB( &conn->operProgress );
        }
    }

    return status;
}

int
clearBulkOprInfo( bulkOprInfo_t *bulkOprInfo ) {
    if ( bulkOprInfo == NULL || bulkOprInfo->count <= 0 ) {
        return 0;
    }
    bulkOprInfo->bytesBuf.len = 0;
    bulkOprInfo->count = bulkOprInfo->size = 0;
    *bulkOprInfo->cachedTargPath = '\0';

    return 0;
}

