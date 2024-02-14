/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "irods/rodsPath.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsLog.h"
#include "irods/miscUtil.h"
#include "irods/trimUtil.h"
#include "irods/rcGlobalExtern.h"
#include "irods/irods_at_scope_exit.hpp"

rodsLong_t TotalSizeTrimmed = 0;
int TotalTrimmed = 0;

int
trimUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
          rodsPathInp_t *rodsPathInp ) {
    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    dataObjInp_t dataObjInp;
    initCondForTrim( myRodsArgs, &dataObjInp );

    if ( myRodsArgs->dryrun == True ) {
        printf( "====== This is a DRYRUN ======\n" );
    }
    int savedStatus = 0;
    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        if ( rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T ) {
            getRodsObjType( conn, &rodsPathInp->srcPath[i] );
            if ( rodsPathInp->srcPath[i].objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR,
                         "trimUtil: srcPath %s does not exist",
                         rodsPathInp->srcPath[i].outPath );
                savedStatus = USER_INPUT_PATH_ERR;
                continue;
            }
        }
        int status = 0;
        if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
            rmKeyVal( &dataObjInp.condInput, TRANSLATED_PATH_KW );
            status = trimDataObjUtil( conn, rodsPathInp->srcPath[i].outPath,
                                      myRodsArgs, &dataObjInp );
        }
        else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
            addKeyVal( &dataObjInp.condInput, TRANSLATED_PATH_KW, "" );
            status = trimCollUtil( conn, rodsPathInp->srcPath[i].outPath,
                                   myRodsEnv, myRodsArgs, &dataObjInp );
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "trimUtil: invalid trim objType %d for %s",
                     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLogError( LOG_ERROR, status,
                          "trimUtil: trim error for %s. ",
                          rodsPathInp->srcPath[i].outPath );
            savedStatus = status;
        }
    }
    printf(
        "Total size trimmed = %-.3f MB. Number of files trimmed = %d.\n",
        ( float ) TotalSizeTrimmed / 1048576.0, TotalTrimmed );
    return savedStatus;
}

int
trimDataObjUtil( rcComm_t *conn, char *srcPath,
                 rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp ) {
    int status = 0;

    if ( srcPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "trimDataObjUtil: NULL srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    rstrcpy( dataObjInp->objPath, srcPath, MAX_NAME_LEN );

    // rcObjStat results will only be used if rcDataObjTrim is successful and actually trims at least one replica from
    // the data object. The invocation occurs before the rcDataObjTrim call because the API call will cause the rError
    // stack in the RcComm struct to be freed and set to nullptr (in successful cases), losing any rError messages
    // which might have been added in rcDataObjTrim. Specifically, a successful trim might include messages warning the
    // client that a deprecated option has been used and we want to ensure that these are returned to the client.
    rodsObjStat_t* rodsObjStatOut = nullptr;
    const auto free_obj_stat = irods::at_scope_exit{[&rodsObjStatOut] {
        if (rodsObjStatOut) {
            freeRodsObjStat(rodsObjStatOut);
        }
    }};
    const int objStatus = rcObjStat(conn, dataObjInp, &rodsObjStatOut);
    if (objStatus < 0) {
        return objStatus;
    }

    status = rcDataObjTrim( conn, dataObjInp );

    if ( status < 0 ) {
        return status;
    }
    if ( status >= 0 && rodsArgs->verbose == True ) {
        char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
        splitPathByKey( srcPath, myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' );
        if ( status > 0 ) {
            printf( "%s - a copy trimmed\n", myFile );
        }
        else {
            printf( "%s - No copy trimmed\n", myFile );
        }
    }

    if ( status > 0 ) {
        if ( objStatus == DATA_OBJ_T ) {
            TotalSizeTrimmed += rodsObjStatOut->objSize;
            TotalTrimmed++;
        } else {
            rodsLog( LOG_ERROR, "trimDataObjUtil: invalid object");
            return INVALID_OBJECT_TYPE;
        }
    }

    return status;
}

int
initCondForTrim( rodsArguments_t *rodsArgs,
                 dataObjInp_t *dataObjInp ) {
    char tmpStr[NAME_LEN];

    if ( dataObjInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForTrim: NULL dataObjInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjInp, 0, sizeof( dataObjInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    if ( rodsArgs->number == True ) {
        snprintf( tmpStr, NAME_LEN, "%d", rodsArgs->numberValue );
        addKeyVal( &dataObjInp->condInput, COPIES_KW, tmpStr );
    }

    if ( rodsArgs->admin == True ) {
        addKeyVal( &dataObjInp->condInput, ADMIN_KW, "" );
    }

    if ( rodsArgs->replNum == True ) {
        addKeyVal( &dataObjInp->condInput, REPL_NUM_KW,
                   rodsArgs->replNumValue );
    }

    if ( rodsArgs->srcResc == True ) {
        addKeyVal( &dataObjInp->condInput, RESC_NAME_KW,
                   rodsArgs->srcRescString );
    }

    if ( rodsArgs->age == True ) {
        snprintf( tmpStr, NAME_LEN, "%d", rodsArgs->agevalue );
        addKeyVal( &dataObjInp->condInput, AGE_KW, tmpStr );
    }

    if ( rodsArgs->dryrun == True ) {
        addKeyVal( &dataObjInp->condInput, DRYRUN_KW, "" );
    }

    // See rmUtil
    if (rodsArgs->unmount == True) {
        dataObjInp->oprType = UNREG_OPR;
    }

    return 0;
}

int
trimCollUtil( rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
              rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp ) {
    int status;
    int savedStatus = 0;
    char srcChildPath[MAX_NAME_LEN];
    collHandle_t collHandle;
    collEnt_t collEnt;

    if ( srcColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "trimCollUtil: NULL srcColl input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "trimCollUtil: -r option must be used for getting %s collection",
                 srcColl );
        return USER_INPUT_OPTION_ERR;
    }

    if ( rodsArgs->verbose == True ) {
        fprintf( stdout, "C- %s:\n", srcColl );
    }

    status = rclOpenCollection( conn, srcColl, 0, &collHandle );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "trimCollUtil: rclOpenCollection of %s error. status = %d",
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
            int status = trimDataObjUtil( conn, srcChildPath, rodsArgs,
                                      dataObjInp );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "trimCollUtil: trimDataObjUtil failed for %s. status = %d",
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
            int status = trimCollUtil( conn, collEnt.collName, myRodsEnv,
                                   rodsArgs, &childDataObjInp );
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

