/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"
#include "rsyncUtil.hpp"
#include "miscUtil.hpp"

#include <sstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

#include "irods_log.hpp"


static int CurrentTime = 0;
int
ageExceeded( int ageLimit, int myTime, char *objPath,
             rodsLong_t fileSize );

int
rsyncUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
           rodsPathInp_t *rodsPathInp ) {
    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    int savedStatus = resolveRodsTarget( conn, rodsPathInp, RSYNC_OPR );
    if ( savedStatus < 0 ) {
        rodsLogError( LOG_ERROR, savedStatus,
                      "rsyncUtil: resolveRodsTarget" );
        return savedStatus;
    }

    dataObjInp_t dataObjOprInp;
    dataObjCopyInp_t dataObjCopyInp;
    if ( rodsPathInp->srcPath[0].objType <= COLL_OBJ_T &&
            rodsPathInp->targPath[0].objType <= COLL_OBJ_T ) {
        initCondForIrodsToIrodsRsync( myRodsEnv, myRodsArgs, &dataObjCopyInp );
    }
    else {
        initCondForRsync( myRodsEnv, myRodsArgs, &dataObjOprInp );
    }

    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {

        rodsPath_t * targPath = &rodsPathInp->targPath[i];
        rodsPath_t * srcPath = &rodsPathInp->srcPath[i];
        int srcType = srcPath->objType;
        int targType = targPath->objType;

        if ( srcPath->objState != EXIST_ST ) {
            rodsLog( LOG_ERROR,
                     "rsyncUtil: Source path %s does not exist" );
            return USER_INPUT_PATH_ERR;
        }

        if ( srcPath->objType <= COLL_OBJ_T && srcPath->rodsObjStat != NULL &&
                rodsPathInp->srcPath[i].rodsObjStat->specColl != NULL ) {
            dataObjOprInp.specColl = dataObjCopyInp.srcDataObjInp.specColl =
                                         srcPath->rodsObjStat->specColl;
        }
        else {
            dataObjOprInp.specColl =
                dataObjCopyInp.srcDataObjInp.specColl = NULL;
        }

        int status = 0;
        if ( srcType == DATA_OBJ_T && targType == LOCAL_FILE_T ) {
            rmKeyVal( &dataObjOprInp.condInput, TRANSLATED_PATH_KW );
            status = rsyncDataToFileUtil( conn, srcPath, targPath,
                                          myRodsArgs, &dataObjOprInp );
        }
        else if ( srcType == LOCAL_FILE_T && targType == DATA_OBJ_T ) {
            if ( isPathSymlink( myRodsArgs, srcPath->outPath ) > 0 ) {
                continue;
            }
            dataObjOprInp.createMode = rodsPathInp->srcPath[i].objMode;
            status = rsyncFileToDataUtil( conn, srcPath, targPath,
                                          myRodsArgs, &dataObjOprInp );
        }
        else if ( srcType == DATA_OBJ_T && targType == DATA_OBJ_T ) {
            rmKeyVal( &dataObjCopyInp.srcDataObjInp.condInput,
                      TRANSLATED_PATH_KW );
            addKeyVal( &dataObjCopyInp.destDataObjInp.condInput,
                       REG_CHKSUM_KW, "" );
            status = rsyncDataToDataUtil( conn, srcPath, targPath,
                                          myRodsArgs, &dataObjCopyInp );
        }
        else if ( srcType == COLL_OBJ_T && targType == LOCAL_DIR_T ) {
            addKeyVal( &dataObjOprInp.condInput, TRANSLATED_PATH_KW, "" );
            status = rsyncCollToDirUtil( conn, srcPath, targPath,
                                         myRodsEnv, myRodsArgs, &dataObjOprInp );
            if ( status >= 0 && dataObjOprInp.specColl != NULL &&
                    dataObjOprInp.specColl->collClass == STRUCT_FILE_COLL ) {
                dataObjOprInp.specColl = NULL;
                status = rsyncCollToDirUtil( conn, srcPath, targPath,
                                             myRodsEnv, myRodsArgs, &dataObjOprInp );
            }
        }
        else if ( srcType == LOCAL_DIR_T && targType == COLL_OBJ_T ) {
            status = rsyncDirToCollUtil( conn, srcPath, targPath,
                                         myRodsEnv, myRodsArgs, &dataObjOprInp );
        }
        else if ( srcType == COLL_OBJ_T && targType == COLL_OBJ_T ) {
            addKeyVal( &dataObjCopyInp.srcDataObjInp.condInput,
                       TRANSLATED_PATH_KW, "" );
            addKeyVal( &dataObjCopyInp.destDataObjInp.condInput,
                       REG_CHKSUM_KW, "" );
            status = rsyncCollToCollUtil( conn, srcPath, targPath,
                                          myRodsEnv, myRodsArgs, &dataObjCopyInp );
            if ( status >= 0 && dataObjOprInp.specColl != NULL &&
                    dataObjCopyInp.srcDataObjInp.specColl->collClass == STRUCT_FILE_COLL ) {
                dataObjCopyInp.srcDataObjInp.specColl = NULL;
                status = rsyncCollToCollUtil( conn, srcPath, targPath,
                                              myRodsEnv, myRodsArgs, &dataObjCopyInp );
            }
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "rsyncUtil: invalid srcType %d and targType %d combination for %s",
                     srcType, targType, targPath->outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND &&
                status != SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
            rodsLogError( LOG_ERROR, status,
                          "rsyncUtil: rsync error for %s",
                          targPath->outPath );
            savedStatus = status;
        }
    }
    return savedStatus;
}

int
rsyncDataToFileUtil( rcComm_t *conn, rodsPath_t *srcPath,
                     rodsPath_t *targPath, rodsArguments_t *myRodsArgs,
                     dataObjInp_t *dataObjOprInp ) {
    int status;
    struct timeval startTime, endTime;
    int getFlag = 0;
    int syncFlag = 0;
    char *chksum;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsyncDataToFileUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }
    /* check the age */
    if ( myRodsArgs->age == True ) {
        if ( srcPath->rodsObjStat != NULL ) {
            if ( ageExceeded( myRodsArgs->agevalue,
                              atoi( srcPath->rodsObjStat->modifyTime ),
                              srcPath->outPath, srcPath->size ) ) {
                return 0;
            }
        }
    }

    if ( myRodsArgs->verbose == True ) {
        ( void ) gettimeofday( &startTime, ( struct timezone * )0 );
        bzero( &conn->transStat, sizeof( transStat_t ) );
    }

    rodsEnv env;
    int ret = getRodsEnv( &env );
    if ( ret < 0 ) {
        rodsLogError(
            LOG_ERROR,
            ret,
            "rsyncDataToFileUtil: getRodsEnv failed" );
        return ret;
    }

    if ( targPath->objState == NOT_EXIST_ST ) {
        getFlag = 1;
    }
    else if ( myRodsArgs->sizeFlag == True ) {
        /* sync by size */
        if ( targPath->size != srcPath->size ) {
            getFlag = 1;
        }
    }
    else if ( strlen( srcPath->chksum ) > 0 ) {
        /* src has a checksum value */
        status = rcChksumLocFile( targPath->outPath,
                                  RSYNC_CHKSUM_KW,
                                  &dataObjOprInp->condInput,
                                  env.rodsDefaultHashScheme );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "rsyncDataToFileUtil: rcChksumLocFile error for %s, status = %d",
                          targPath->outPath, status );
            return status;
        }
        else {
            chksum = getValByKey( &dataObjOprInp->condInput, RSYNC_CHKSUM_KW );
            if ( chksum == NULL || strcmp( chksum, srcPath->chksum ) != 0 ) {
                getFlag = 1;
            }
        }
    }
    else {
        /* exist but no chksum */
        status = rcChksumLocFile( targPath->outPath, RSYNC_CHKSUM_KW,
                                  &dataObjOprInp->condInput,
                                  env.rodsDefaultHashScheme );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "rsyncDataToFileUtil: rcChksumLocFile error for %s, status = %d",
                          targPath->outPath, status );
            return status;
        }
        else {
            syncFlag = 1;
        }
    }

    if ( getFlag + syncFlag > 0 ) {
        if ( myRodsArgs->verifyChecksum == True ) {
            addKeyVal( &dataObjOprInp->condInput, VERIFY_CHKSUM_KW, "" );
        }
        rstrcpy( dataObjOprInp->objPath, srcPath->outPath, MAX_NAME_LEN );
        dataObjOprInp->dataSize = srcPath->size;
        dataObjOprInp->openFlags = O_RDONLY;
    }

    if ( getFlag == 1 ) {
        /* only do the sync if no -l option specified */
        if ( myRodsArgs->longOption != True ) {
            status = rcDataObjGet( conn, dataObjOprInp, targPath->outPath );
        }
        else {
            status = 0;
            printf( "%s   %lld   N\n", srcPath->outPath, srcPath->size );
        }
        rmKeyVal( &dataObjOprInp->condInput, RSYNC_CHKSUM_KW );
        if ( status >= 0 && myRodsArgs->longOption != True ) {
            myChmod( targPath->outPath, srcPath->objMode );
        }
    }
    else if ( syncFlag == 1 ) {
        addKeyVal( &dataObjOprInp->condInput, RSYNC_DEST_PATH_KW,
                   targPath->outPath );
        addKeyVal( &dataObjOprInp->condInput, RSYNC_MODE_KW, LOCAL_TO_IRODS );
        status = rcDataObjRsync( conn, dataObjOprInp );
        rmKeyVal( &dataObjOprInp->condInput, RSYNC_DEST_PATH_KW );
        rmKeyVal( &dataObjOprInp->condInput, RSYNC_MODE_KW );
    }
    else {
        status = 0;
    }

    if ( status >= 0 && myRodsArgs->verbose == True ) {
        if ( getFlag > 0 ||
                ( syncFlag > 0 && status == SYS_RSYNC_TARGET_MODIFIED ) ) {
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, srcPath->outPath, srcPath->size,
                         targPath->outPath, &startTime, &endTime );
        }
        else {
            printNoSync( srcPath->outPath, srcPath->size, "a match" );
        }
    }

    return status;
}

int
rsyncFileToDataUtil( rcComm_t *conn, rodsPath_t *srcPath,
                     rodsPath_t *targPath, rodsArguments_t *myRodsArgs,
                     dataObjInp_t *dataObjOprInp ) {
    int status;
    struct timeval startTime, endTime;
    int putFlag = 0;
    int syncFlag = 0;
    char *chksum;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsyncFileToDataUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }
    /* check the age */
    if ( myRodsArgs->age == True ) {
#ifndef windows_platform
        struct stat statbuf;
        status = stat( srcPath->outPath, &statbuf );
#else
        struct irodsntstat statbuf;
        status = iRODSNt_stat( srcPath->outPath, &statbuf );
#endif
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "rsyncFileToDataUtil: stat error for %s, errno = %d\n",
                     srcPath->outPath, errno );
            return USER_INPUT_PATH_ERR;
        }
        if ( ageExceeded( myRodsArgs->agevalue, statbuf.st_mtime,
                          srcPath->outPath, srcPath->size ) ) {
            return 0;
        }
    }

    rodsEnv env;
    int ret = getRodsEnv( &env );
    if ( ret < 0 ) {
        rodsLogError(
            LOG_ERROR,
            ret,
            "rsyncDataToFileUtil: getRodsEnv failed" );
        return ret;
    }

    if ( myRodsArgs->verbose == True ) {
        ( void ) gettimeofday( &startTime, ( struct timezone * )0 );
        bzero( &conn->transStat, sizeof( transStat_t ) );
    }

    if ( targPath->objState == NOT_EXIST_ST ) {
        putFlag = 1;
    }
    else if ( myRodsArgs->sizeFlag == True ) {
        /* sync by size */
        if ( targPath->size != srcPath->size ) {
            putFlag = 1;
        }
    }
    else if ( strlen( targPath->chksum ) > 0 ) {
        /* src has a checksum value */
        status = rcChksumLocFile( srcPath->outPath, RSYNC_CHKSUM_KW,
                                  &dataObjOprInp->condInput,
                                  env.rodsDefaultHashScheme );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "rsyncFileToDataUtil: rcChksumLocFile error for %s, status = %d",
                          srcPath->outPath, status );
            return status;
        }
        else {
            chksum = getValByKey( &dataObjOprInp->condInput, RSYNC_CHKSUM_KW );
            if ( chksum == NULL || strcmp( chksum, targPath->chksum ) != 0 ) {
                if ( chksum != NULL && myRodsArgs->verifyChecksum == True ) {
                    addKeyVal( &dataObjOprInp->condInput, VERIFY_CHKSUM_KW,
                               chksum );
                }

                putFlag = 1;
            }
        }
    }
    else {
        /* exist but no chksum */
        status = rcChksumLocFile( srcPath->outPath, RSYNC_CHKSUM_KW,
                                  &dataObjOprInp->condInput,
                                  env.rodsDefaultHashScheme );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "rsyncFileToDataUtil: rcChksumLocFile error for %s, status = %d",
                          srcPath->outPath, status );
            return status;
        }
        else {
            chksum = getValByKey( &dataObjOprInp->condInput, RSYNC_CHKSUM_KW );
            if ( chksum != NULL && myRodsArgs->verifyChecksum == True ) {
                addKeyVal( &dataObjOprInp->condInput, VERIFY_CHKSUM_KW, chksum );
            }
            syncFlag = 1;
        }
    }

    if ( putFlag + syncFlag > 0 ) {
        rstrcpy( dataObjOprInp->objPath, targPath->outPath, MAX_NAME_LEN );
        dataObjOprInp->dataSize = srcPath->size;
        dataObjOprInp->openFlags = O_WRONLY;
    }

    if ( putFlag == 1 ) {
        /* only do the sync if no -l option specified */
        if ( myRodsArgs->longOption != True ) {
            status = rcDataObjPut( conn, dataObjOprInp, srcPath->outPath );
        }
        else {
            status = 0;
            printf( "%s   %lld   N\n", srcPath->outPath, srcPath->size );
        }
        rmKeyVal( &dataObjOprInp->condInput, RSYNC_CHKSUM_KW );
    }
    else if ( syncFlag == 1 ) {
        addKeyVal( &dataObjOprInp->condInput, RSYNC_DEST_PATH_KW,
                   srcPath->outPath );
        addKeyVal( &dataObjOprInp->condInput, RSYNC_MODE_KW, IRODS_TO_LOCAL );
        /* only do the sync if no -l option specified */
        if ( myRodsArgs->longOption != True ) {
            status = rcDataObjRsync( conn, dataObjOprInp );
        }
        else {
            status = 0;
        }
        rmKeyVal( &dataObjOprInp->condInput, RSYNC_DEST_PATH_KW );
        rmKeyVal( &dataObjOprInp->condInput, RSYNC_MODE_KW );
    }
    else {
        status = 0;
    }

    if ( status >= 0 && myRodsArgs->verbose == True ) {
        if ( putFlag > 0 ||
                ( syncFlag > 0 && status == SYS_RSYNC_TARGET_MODIFIED ) ) {
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, srcPath->outPath, srcPath->size,
                         targPath->outPath, &startTime, &endTime );
        }
        else {
            printNoSync( srcPath->outPath, srcPath->size, "a match" );
        }
    }

    return status;
}

int
rsyncDataToDataUtil( rcComm_t *conn, rodsPath_t *srcPath,
                     rodsPath_t *targPath, rodsArguments_t *myRodsArgs,
                     dataObjCopyInp_t *dataObjCopyInp ) {
    int status;
    struct timeval startTime, endTime;
    int syncFlag = 0;
    int cpFlag = 0;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsyncDataToDataUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }
    /* check the age */
    if ( myRodsArgs->age == True ) {
        if ( srcPath->rodsObjStat != NULL ) {
            if ( ageExceeded( myRodsArgs->agevalue,
                              atoi( srcPath->rodsObjStat->modifyTime ),
                              srcPath->outPath, srcPath->size ) ) {
                return 0;
            }
        }
    }

    if ( myRodsArgs->verbose == True ) {
        ( void ) gettimeofday( &startTime, ( struct timezone * )0 );
        bzero( &conn->transStat, sizeof( transStat_t ) );
    }

    if ( targPath->objState == NOT_EXIST_ST ) {
        cpFlag = 1;
    }
    else if ( myRodsArgs->sizeFlag == True ) {
        /* sync by size */
        if ( targPath->size != srcPath->size ) {
            cpFlag = 1;
        }
    }
    else if ( strlen( srcPath->chksum ) > 0 && strlen( targPath->chksum ) > 0 ) {
        /* src and trg has a checksum value */
        if ( strcmp( targPath->chksum, srcPath->chksum ) != 0 ) {
            cpFlag = 1;
        }
    }
    else {
        syncFlag = 1;
    }

    if ( cpFlag == 1 ) {
        rstrcpy( dataObjCopyInp->srcDataObjInp.objPath, srcPath->outPath,
                 MAX_NAME_LEN );
        dataObjCopyInp->srcDataObjInp.dataSize = srcPath->size;
        rstrcpy( dataObjCopyInp->destDataObjInp.objPath, targPath->outPath,
                 MAX_NAME_LEN );
        /* only do the sync if no -l option specified */
        if ( myRodsArgs->longOption != True ) {
            status = rcDataObjCopy( conn, dataObjCopyInp );

            if ( status < 0 ) {
                char* sys_error;
                char* rods_error = rodsErrorName( status, &sys_error );
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to copy the object \"";
                msg << srcPath->outPath;
                msg << "\" to \"";
                msg << targPath->outPath;
                msg << "\" ";
                msg << rods_error << " " << sys_error;
                irods::log( LOG_ERROR, msg.str() );
            }

        }
        else {
            status = 0;
            printf( "%s   %lld   N\n", srcPath->outPath, srcPath->size );
        }
    }
    else if ( syncFlag == 1 ) {
        dataObjInp_t *dataObjOprInp = &dataObjCopyInp->destDataObjInp;
        rstrcpy( dataObjOprInp->objPath, srcPath->outPath,
                 MAX_NAME_LEN );
        dataObjOprInp->dataSize = srcPath->size;
        addKeyVal( &dataObjOprInp->condInput, RSYNC_DEST_PATH_KW,
                   targPath->outPath );
        addKeyVal( &dataObjOprInp->condInput, RSYNC_MODE_KW, IRODS_TO_IRODS );
        /* only do the sync if no -l option specified */
        if ( myRodsArgs->longOption != True ) {
            status = rcDataObjRsync( conn, dataObjOprInp );
        }
        else {
            status = 0;
            printf( "%s   %lld   N\n", srcPath->outPath, srcPath->size );
        }
        rmKeyVal( &dataObjOprInp->condInput, RSYNC_MODE_KW );
        rmKeyVal( &dataObjOprInp->condInput, RSYNC_DEST_PATH_KW );
    }
    else {
        status = 0;
    }

    if ( status >= 0 && myRodsArgs->verbose == True ) {
        if ( cpFlag > 0 ||
                ( syncFlag > 0 && status == SYS_RSYNC_TARGET_MODIFIED ) ) {
            ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
            printTiming( conn, srcPath->outPath, srcPath->size,
                         targPath->outPath, &startTime, &endTime );
        }
        else {
            printNoSync( srcPath->outPath, srcPath->size, "a match" );
        }
    }

    return status;
}

int
rsyncCollToDirUtil( rcComm_t *conn, rodsPath_t *srcPath,
                    rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                    dataObjInp_t *dataObjOprInp ) {
    int status = 0;
    int savedStatus = 0;
    char *srcColl, *targDir;
    char targChildPath[MAX_NAME_LEN];
    char parPath[MAX_NAME_LEN], childPath[MAX_NAME_LEN];
    rodsPath_t mySrcPath, myTargPath;
    collHandle_t collHandle;
    collEnt_t collEnt;
    dataObjInp_t childDataObjInp;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsyncCollToDirUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }

    srcColl = srcPath->outPath;
    targDir = targPath->outPath;

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "rsyncCollToDirUtil: -r option must be used for collection %s",
                 targDir );
        return USER_INPUT_OPTION_ERR;
    }

    status = rclOpenCollection( conn, srcColl,
                                VERY_LONG_METADATA_FG, &collHandle );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsyncCollToDirUtil: rclOpenCollection of %s error. status = %d",
                 srcColl, status );
        return status;
    }

    memset( &mySrcPath, 0, sizeof( mySrcPath ) );
    memset( &myTargPath, 0, sizeof( myTargPath ) );
    myTargPath.objType = LOCAL_FILE_T;
    mySrcPath.objType = DATA_OBJ_T;

    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            if ( rodsArgs->age == True ) {
                if ( ageExceeded( rodsArgs->agevalue,
                                  atoi( collEnt.modifyTime ),
                                  collEnt.dataName, collEnt.dataSize ) ) {
                    continue;
                }
            }
            snprintf( myTargPath.outPath, MAX_NAME_LEN, "%s/%s",
                      targDir, collEnt.dataName );
            snprintf( mySrcPath.outPath, MAX_NAME_LEN, "%s/%s",
                      collEnt.collName, collEnt.dataName );
            /* fill in some info for mySrcPath */
            if ( strlen( mySrcPath.dataId ) == 0 ) {
                rstrcpy( mySrcPath.dataId, collEnt.dataId, NAME_LEN );
            }
            mySrcPath.size = collEnt.dataSize;
            mySrcPath.objMode = collEnt.dataMode;
            rstrcpy( mySrcPath.chksum, collEnt.chksum, NAME_LEN );
            mySrcPath.objState = EXIST_ST;

            getFileType( &myTargPath );

            status = rsyncDataToFileUtil( conn, &mySrcPath, &myTargPath,
                                          rodsArgs, dataObjOprInp );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "rsyncCollUtil: rsyncDataObjUtil failed for %s. status = %d",
                              mySrcPath.outPath, status );
                /* need to set global error here */
                savedStatus = status;
                status = 0;
            }
        }
        else if ( collEnt.objType == COLL_OBJ_T ) {
            if ( ( status = splitPathByKey(
                                collEnt.collName, parPath, MAX_NAME_LEN, childPath, MAX_NAME_LEN, '/' ) ) < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "rsyncCollToDirUtil:: splitPathByKey for %s error, stat=%d",
                              collEnt.collName, status );
                return status;
            }
            snprintf( targChildPath, MAX_NAME_LEN, "%s/%s",
                      targDir, childPath );

            /* only do the sync if no -l option specified */
            if ( rodsArgs->longOption != True ) {
                mkdirR( targDir, targChildPath, 0750 );
            }

            /* the child is a spec coll. need to drill down */
            childDataObjInp = *dataObjOprInp;
            if ( collEnt.specColl.collClass != NO_SPEC_COLL ) {
                childDataObjInp.specColl = &collEnt.specColl;
            }
            else {
                childDataObjInp.specColl = NULL;
            }
            rstrcpy( myTargPath.outPath, targChildPath, MAX_NAME_LEN );
            rstrcpy( mySrcPath.outPath, collEnt.collName, MAX_NAME_LEN );

            status = rsyncCollToDirUtil( conn, &mySrcPath,
                                         &myTargPath, myRodsEnv, rodsArgs, &childDataObjInp );

            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                return status;
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

int
rsyncDirToCollUtil( rcComm_t *conn, rodsPath_t *srcPath,
                    rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                    dataObjInp_t *dataObjOprInp ) {
    char *srcDir, *targColl;
    rodsPath_t mySrcPath, myTargPath;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsyncDirToCollUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }

    srcDir = srcPath->outPath;
    targColl = targPath->outPath;

    if ( isPathSymlink( rodsArgs, srcDir ) > 0 ) {
        return 0;
    }

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "rsyncDirToCollUtil: -r option must be used for putting %s directory",
                 srcDir );
        return USER_INPUT_OPTION_ERR;
    }

    path srcDirPath( srcDir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        rodsLog( LOG_ERROR,
                 "rsyncDirToCollUtil: opendir local dir error for %s, errno = %d\n",
                 srcDir, errno );
        return USER_INPUT_PATH_ERR;
    }

    if ( rodsArgs->verbose == True ) {
        fprintf( stdout, "C- %s:\n", targColl );
    }

    memset( &mySrcPath,  0, sizeof( mySrcPath ) );
    memset( &myTargPath, 0, sizeof( myTargPath ) );
    myTargPath.objType = DATA_OBJ_T;
    mySrcPath.objType  = LOCAL_FILE_T;

    directory_iterator end_itr; // default construction yields past-the-end
    int savedStatus = 0;
    for ( directory_iterator itr( srcDirPath );
            itr != end_itr;
            ++itr ) {
        path p = itr->path();
        snprintf( mySrcPath.outPath, MAX_NAME_LEN, "%s", p.c_str() );

        if ( isPathSymlink( rodsArgs, mySrcPath.outPath ) > 0 ) {
            continue;
        }

        if ( !exists( p ) ) {
            rodsLog( LOG_ERROR,
                     "rsyncDirToCollUtil: stat error for %s, errno = %d\n",
                     mySrcPath.outPath, errno );
            return USER_INPUT_PATH_ERR;
        }

        if ( ( is_regular_file( p ) && rodsArgs->age == True ) &&
                ageExceeded(
                    rodsArgs->agevalue,
                    last_write_time( p ),
                    mySrcPath.outPath,
                    file_size( p ) ) ) {
            continue;
        }

        bzero( &myTargPath, sizeof( myTargPath ) );
        path childPath = p.filename();
        snprintf( myTargPath.outPath, MAX_NAME_LEN, "%s/%s",
                  targColl, childPath.c_str() );
        if ( is_symlink( p ) ) {
            path cp = read_symlink( p );
            snprintf( mySrcPath.outPath, MAX_NAME_LEN, "%s/%s",
                      srcDir, cp.c_str() );
            p = path( mySrcPath.outPath );
        }
        dataObjOprInp->createMode = getPathStMode( p.c_str() );
        int status = 0;
        if ( is_regular_file( p ) ) {
            myTargPath.objType = DATA_OBJ_T;
            mySrcPath.objType = LOCAL_FILE_T;
            mySrcPath.objState = EXIST_ST;

            mySrcPath.size = file_size( p );
            getRodsObjType( conn, &myTargPath );
            status = rsyncFileToDataUtil( conn, &mySrcPath, &myTargPath,
                                          rodsArgs, dataObjOprInp );
            /* fix a big mem leak */
            if ( myTargPath.rodsObjStat != NULL ) {
                freeRodsObjStat( myTargPath.rodsObjStat );
                myTargPath.rodsObjStat = NULL;
            }
        }
        else if ( is_directory( p ) ) {
            /* only do the sync if no -l option specified */
            if ( rodsArgs->longOption != True ) {
                status = mkColl( conn, targColl );
            }
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "rsyncDirToCollUtil: mkColl error for %s",
                              myTargPath.outPath );
            }
            else {
                myTargPath.objType = COLL_OBJ_T;
                mySrcPath.objType = LOCAL_DIR_T;
                mySrcPath.objState = myTargPath.objState = EXIST_ST;
                getRodsObjType( conn, &myTargPath );
                status = rsyncDirToCollUtil( conn, &mySrcPath, &myTargPath,
                                             myRodsEnv, rodsArgs, dataObjOprInp );
                /* fix a big mem leak */
                if ( myTargPath.rodsObjStat != NULL ) {
                    freeRodsObjStat( myTargPath.rodsObjStat );
                    myTargPath.rodsObjStat = NULL;
                }
            }
        }
        else {
            rodsLog( LOG_ERROR,
                     "rsyncDirToCollUtil: unknown local path %s",
                     mySrcPath.outPath );
            status = USER_INPUT_PATH_ERR;
        }

        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND &&
                status != SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
            savedStatus = status;
            rodsLogError( LOG_ERROR, status,
                          "rsyncDirToCollUtil: put %s failed. status = %d",
                          mySrcPath.outPath, status );
        }
    }

    return savedStatus;

}

int
rsyncCollToCollUtil( rcComm_t *conn, rodsPath_t *srcPath,
                     rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                     dataObjCopyInp_t *dataObjCopyInp ) {
    int status = 0;
    int savedStatus = 0;
    char *srcColl, *targColl;
    char targChildPath[MAX_NAME_LEN];
    char parPath[MAX_NAME_LEN], childPath[MAX_NAME_LEN];

    rodsPath_t mySrcPath, myTargPath;
    collHandle_t collHandle;
    collEnt_t collEnt;
    dataObjCopyInp_t childDataObjCopyInp;

    dataObjInp_t *dataObjOprInp = &collHandle.dataObjInp;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsyncCollToCollUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }

    srcColl = srcPath->outPath;
    targColl = targPath->outPath;

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "rsyncCollToCollUtil: -r option must be used for collection %s",
                 targColl );
        return USER_INPUT_OPTION_ERR;
    }

    status = rclOpenCollection( conn, srcColl,
                                VERY_LONG_METADATA_FG, &collHandle );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getCollUtil: rclOpenCollection of %s error. status = %d",
                 srcColl, status );
        return status;
    }

    if ( dataObjOprInp->specColl != NULL ) {
        if ( rodsArgs->verbose == True ) {
            if ( rodsArgs->verbose == True ) {
                char objType[NAME_LEN];
                status = getSpecCollTypeStr( dataObjOprInp->specColl, objType );
                if ( status < 0 ) {
                    fprintf( stdout, "C- %s    :\n", targColl );
                }
                else {
                    fprintf( stdout, "C- %s    %-5.5s :\n", targColl, objType );
                }
            }
        }
    }

    memset( &mySrcPath, 0, sizeof( mySrcPath ) );
    memset( &myTargPath, 0, sizeof( myTargPath ) );
    myTargPath.objType = DATA_OBJ_T;
    mySrcPath.objType = DATA_OBJ_T;

    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            if ( rodsArgs->age == True ) {
                if ( ageExceeded( rodsArgs->agevalue,
                                  atoi( collEnt.modifyTime ),
                                  collEnt.dataName, collEnt.dataSize ) ) {
                    continue;
                }
            }

            snprintf( myTargPath.outPath, MAX_NAME_LEN, "%s/%s",
                      targColl, collEnt.dataName );

            snprintf( mySrcPath.outPath, MAX_NAME_LEN, "%s/%s",
                      collEnt.collName, collEnt.dataName );
            /* fill in some info for mySrcPath */
            if ( strlen( mySrcPath.dataId ) == 0 ) {
                rstrcpy( mySrcPath.dataId, collEnt.dataId, NAME_LEN );
            }
            mySrcPath.size = collEnt.dataSize;
            rstrcpy( mySrcPath.chksum, collEnt.chksum, NAME_LEN );
            mySrcPath.objState = EXIST_ST;

            getRodsObjType( conn, &myTargPath );

            status = rsyncDataToDataUtil( conn, &mySrcPath, &myTargPath,
                                          rodsArgs, dataObjCopyInp );
            if ( myTargPath.rodsObjStat != NULL ) {
                freeRodsObjStat( myTargPath.rodsObjStat );
                myTargPath.rodsObjStat = NULL;
            }
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "rsyncCollToCollUtil: rsyncDataToDataUtil failed for %s.stat=%d",
                              myTargPath.outPath, status );
                /* need to set global error here */
                savedStatus = status;
                status = 0;
            }
        }
        else if ( collEnt.objType == COLL_OBJ_T ) {
            if ( ( status = splitPathByKey(
                                collEnt.collName, parPath, MAX_NAME_LEN, childPath, MAX_NAME_LEN, '/' ) ) < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "rsyncCollToCollUtil:: splitPathByKey for %s error, status = %d",
                              collEnt.collName, status );
                return status;
            }
            snprintf( targChildPath, MAX_NAME_LEN, "%s/%s",
                      targColl, childPath );

            if ( rodsArgs->longOption != True ) {   /* only do the sync if no -l option specified */
                mkColl( conn, targChildPath );
            }

            /* the child is a spec coll. need to drill down */
            childDataObjCopyInp = *dataObjCopyInp;
            if ( collEnt.specColl.collClass != NO_SPEC_COLL )
                childDataObjCopyInp.srcDataObjInp.specColl =
                    &collEnt.specColl;
            rstrcpy( myTargPath.outPath, targChildPath, MAX_NAME_LEN );
            rstrcpy( mySrcPath.outPath, collEnt.collName, MAX_NAME_LEN );
            status = rsyncCollToCollUtil( conn, &mySrcPath,
                                          &myTargPath, myRodsEnv, rodsArgs, &childDataObjCopyInp );


            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                return status;
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

int
initCondForRsync( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                  dataObjInp_t *dataObjInp ) {
    char tmpStr[NAME_LEN];


    if ( dataObjInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForRsync: NULL dataObjOprInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjInp, 0, sizeof( dataObjInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    /* always turn on the force flag */
    addKeyVal( &dataObjInp->condInput, FORCE_FLAG_KW, "" );

    if ( rodsArgs->sizeFlag == True ) {
        addKeyVal( &dataObjInp->condInput, VERIFY_BY_SIZE_KW, "" );
    }

    if ( rodsArgs->all == True ) {
        addKeyVal( &dataObjInp->condInput, ALL_KW, "" );
    }

#ifdef windows_platform
    dataObjInp->numThreads = NO_THREADING;
#else
    if ( rodsArgs->number == True ) {
        if ( rodsArgs->numberValue == 0 ) {
            dataObjInp->numThreads = NO_THREADING;
        }
        else {
            dataObjInp->numThreads = rodsArgs->numberValue;
        }
    }
#endif

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForRepl: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &dataObjInp->condInput, DEST_RESC_NAME_KW,
                       rodsArgs->resourceString );
        }
    }
    else if ( myRodsEnv != NULL && strlen( myRodsEnv->rodsDefResource ) > 0 ) {
        addKeyVal( &dataObjInp->condInput, DEST_RESC_NAME_KW,
                   myRodsEnv->rodsDefResource );
    }
    if ( rodsArgs->age == True ) {
        snprintf( tmpStr, NAME_LEN, "%d", rodsArgs->agevalue );
        addKeyVal( &dataObjInp->condInput, AGE_KW, tmpStr );
    }

    return 0;
}

int
initCondForIrodsToIrodsRsync( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                              dataObjCopyInp_t *dataObjCopyInp ) {
    char tmpStr[NAME_LEN];

    if ( dataObjCopyInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForCp: NULL dataObjCopyInp incp" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjCopyInp, 0, sizeof( dataObjCopyInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    /* always turn on the force flag */
    addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, FORCE_FLAG_KW, "" );

    if ( rodsArgs->sizeFlag == True ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                   VERIFY_BY_SIZE_KW, "" );
    }

    if ( rodsArgs->all == True ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, ALL_KW, "" );
    }

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForRepl: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                       DEST_RESC_NAME_KW, rodsArgs->resourceString );
        }
    }
    else if ( myRodsEnv != NULL && strlen( myRodsEnv->rodsDefResource ) > 0 ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                   DEST_RESC_NAME_KW, myRodsEnv->rodsDefResource );
    }
// =-=-=-=-=-=-=-
// JMC - backport 4865
#ifdef windows_platform
    dataObjCopyInp->destDataObjInp.numThreads = NO_THREADING;
#else
    if ( rodsArgs->number == True ) {
        if ( rodsArgs->numberValue == 0 ) {
            dataObjCopyInp->destDataObjInp.numThreads = NO_THREADING;
        }
        else {
            dataObjCopyInp->destDataObjInp.numThreads = rodsArgs->numberValue;
        }
    }
#endif
// =-=-=-=-=-=-=-
    if ( rodsArgs->age == True ) {
        snprintf( tmpStr, NAME_LEN, "%d", rodsArgs->agevalue );
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput, AGE_KW, tmpStr );
    }

    if ( rodsArgs->verifyChecksum == True ) {
        addKeyVal( &dataObjCopyInp->destDataObjInp.condInput,
                   VERIFY_CHKSUM_KW, "" );
    }

    return 0;
}

int
ageExceeded( int ageLimit, int myTime, char *objPath,
             rodsLong_t fileSize ) {
    int age;

    if ( CurrentTime == 0 ) {
        CurrentTime = time( 0 );
    }
    age = CurrentTime - myTime;
    if ( age > ageLimit * 60 ) {
        printNoSync( objPath, fileSize, "age" );
        return 1;
    }
    else {
        return 0;
    }
}

