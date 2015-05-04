/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "miscUtil.h"
#include "rodsLog.h"
#include "rmtrashUtil.h"
#include "rcGlobalExtern.h"

int
rmtrashUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs,
             rodsPathInp_t *rodsPathInp ) {
    int i = 0;
    int status = 0;
    int savedStatus = 0;
    collInp_t collInp;
    dataObjInp_t dataObjInp;


    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    initCondForRmtrash( myRodsArgs, &dataObjInp, &collInp );

    if ( rodsPathInp->numSrc <= 0 ) {
        char trashPath[MAX_NAME_LEN];
        char *myZoneName;
        char myZoneType[MAX_NAME_LEN];

        if ( myRodsArgs->zoneName != NULL ) {
            myZoneName = myRodsArgs->zoneName;
        }
        else {
            myZoneName = conn->clientUser.rodsZone;
        }
        if ( myRodsArgs->admin == True ) {
            if ( myRodsArgs->user == True ) {
                snprintf( trashPath, MAX_NAME_LEN, "/%s/trash/home/%s",
                          myZoneName, myRodsArgs->userString );
            }
            else if ( myRodsArgs->orphan  == True ) {
                snprintf( trashPath, MAX_NAME_LEN, "/%s/trash/orphan",
                          myZoneName );
            }
            else {
                snprintf( trashPath, MAX_NAME_LEN, "/%s/trash/home",
                          myZoneName );
            }
        }
        else {
            int remoteFlag = 0;
            status = getZoneType( conn, myZoneName, conn->clientUser.rodsZone,
                                  myZoneType );
            if ( status >= 0 ) {
                if ( strcmp( myZoneType, "remote" ) == 0 ) {
                    remoteFlag = 1;
                }
            }
            if ( remoteFlag == 0 ) {
                snprintf( trashPath, MAX_NAME_LEN, "/%s/trash/home/%s",
                          myZoneName, conn->clientUser.userName );
            }
            else {
                snprintf( trashPath, MAX_NAME_LEN, "/%s/trash/home/%s#%s",
                          myZoneName, conn->clientUser.userName,
                          conn->clientUser.rodsZone );
            }
        }
        addSrcInPath( rodsPathInp, trashPath );
        rstrcpy( rodsPathInp->srcPath[0].outPath, trashPath, MAX_NAME_LEN );
        rodsPathInp->srcPath[0].objType = COLL_OBJ_T;
    }

    for ( i = 0; i < rodsPathInp->numSrc; i++ ) {
        if ( rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T ) {
            getRodsObjType( conn, &rodsPathInp->srcPath[i] );
            if ( rodsPathInp->srcPath[i].objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR,
                         "rmtrashUtil: srcPath %s does not exist",
                         rodsPathInp->srcPath[i].outPath );
                savedStatus = USER_INPUT_PATH_ERR;
                continue;
            }
        }

        if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
            status = rmtrashDataObjUtil( conn, rodsPathInp->srcPath[i].outPath,
                                         myRodsArgs, &dataObjInp );
        }
        else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
            status = rmtrashCollUtil( conn, rodsPathInp->srcPath[i].outPath,
                                      myRodsArgs, &collInp );
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "rmtrashUtil: invalid rmtrash objType %d for %s",
                     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "rmtrashUtil: rmtrash error for %s, status = %d",
                          rodsPathInp->srcPath[i].outPath, status );
            savedStatus = status;
        }
    }
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

int
rmtrashDataObjUtil( rcComm_t *conn, char *srcPath,
                    rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp ) {
    int status = 0;
    struct timeval startTime, endTime;

    if ( srcPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rmtrashDataObjUtil: NULL srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->verbose == True ) {
        ( void ) gettimeofday( &startTime, ( struct timezone * )0 );
    }

    rstrcpy( dataObjInp->objPath, srcPath, MAX_NAME_LEN );

    status = rcDataObjUnlink( conn, dataObjInp );

    if ( status >= 0 && rodsArgs->verbose == True ) {
        ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
        printTiming( conn, dataObjInp->objPath, -1, NULL,
                     &startTime, &endTime );
    }

    return status;
}

int
initCondForRmtrash( rodsArguments_t *rodsArgs,
                    dataObjInp_t *dataObjInp, collInp_t *collInp ) {
    char tmpStr[NAME_LEN];

    if ( dataObjInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForRmtrash: NULL dataObjInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjInp, 0, sizeof( dataObjInp_t ) );
    memset( collInp, 0, sizeof( collInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    addKeyVal( &dataObjInp->condInput, FORCE_FLAG_KW, "" );
    addKeyVal( &collInp->condInput, FORCE_FLAG_KW, "" );

    addKeyVal( &dataObjInp->condInput, RECURSIVE_OPR__KW, "" );
    addKeyVal( &collInp->condInput, RECURSIVE_OPR__KW, "" );

    if ( rodsArgs->admin == True ) {
        addKeyVal( &dataObjInp->condInput, ADMIN_RMTRASH_KW, "" );
        addKeyVal( &collInp->condInput, ADMIN_RMTRASH_KW, "" );
    }
    else {
        addKeyVal( &dataObjInp->condInput, RMTRASH_KW, "" );
        addKeyVal( &collInp->condInput, RMTRASH_KW, "" );
    }

    if ( rodsArgs->zone == True ) {
        addKeyVal( &collInp->condInput, ZONE_KW, rodsArgs->zoneName );
        addKeyVal( &dataObjInp->condInput, ZONE_KW, rodsArgs->zoneName );
    }

    if ( rodsArgs->age == True ) {
        snprintf( tmpStr, NAME_LEN, "%d", rodsArgs->agevalue );
        addKeyVal( &collInp->condInput, AGE_KW, tmpStr );
        addKeyVal( &dataObjInp->condInput, AGE_KW, tmpStr );
    }

    seedRandom();

    return 0;
}

int
rmtrashCollUtil( rcComm_t *conn, char *srcColl,
                 rodsArguments_t *rodsArgs, collInp_t *collInp ) {
    int status;

    if ( srcColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "rmtrashCollUtil: NULL srcColl input" );
        return USER__NULL_INPUT_ERR;
    }

    rstrcpy( collInp->collName, srcColl, MAX_NAME_LEN );
    status = rcRmColl( conn, collInp, rodsArgs->verbose );

    return status;
}

