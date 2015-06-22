/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "miscUtil.h"
#include "mvUtil.h"
#include "rcGlobalExtern.h"

int
mvUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs, rodsPathInp_t *rodsPathInp ) {
    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    dataObjCopyInp_t dataObjRenameInp;
    initCondForMv( &dataObjRenameInp );

    int savedStatus = resolveRodsTarget( conn, rodsPathInp, MOVE_OPR );
    if ( savedStatus < 0 ) {
        rodsLogError( LOG_ERROR, savedStatus,
                      "mvUtil: resolveRodsTarget error, status = %d", savedStatus );
        return savedStatus;
    }

    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        rodsPath_t * targPath = &rodsPathInp->targPath[i];

        int status = mvObjUtil( conn, rodsPathInp->srcPath[i].outPath,
                                targPath->outPath, targPath->objType, myRodsArgs,
                                &dataObjRenameInp );

        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLogError( LOG_ERROR, status,
                          "mvUtil: mv error for %s, status = %d",
                          targPath->outPath, status );
            savedStatus = status;
        }
    }
    return savedStatus;
}

int
mvObjUtil( rcComm_t *conn, char *srcPath, char *targPath, objType_t objType,
           rodsArguments_t *rodsArgs, dataObjCopyInp_t *dataObjRenameInp ) {
    int status;
    struct timeval startTime, endTime;

    if ( srcPath == NULL || targPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "mvFileUtil: NULL srcPath or targPath input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( objType == DATA_OBJ_T ) {
        dataObjRenameInp->srcDataObjInp.oprType =
            dataObjRenameInp->destDataObjInp.oprType = RENAME_DATA_OBJ;
    }
    else if ( objType == COLL_OBJ_T ) {
        dataObjRenameInp->srcDataObjInp.oprType =
            dataObjRenameInp->destDataObjInp.oprType = RENAME_COLL;
    }
    else {
        /* should not be here */
        rodsLog( LOG_ERROR,
                 "mvObjUtil: invalid cp dest objType %d for %s",
                 objType, targPath );
        return USER_INPUT_PATH_ERR;
    }


    if ( rodsArgs->verbose == True ) {
        ( void ) gettimeofday( &startTime, ( struct timezone * )0 );
    }

    rstrcpy( dataObjRenameInp->destDataObjInp.objPath, targPath, MAX_NAME_LEN );
    rstrcpy( dataObjRenameInp->srcDataObjInp.objPath, srcPath, MAX_NAME_LEN );

    status = rcDataObjRename( conn, dataObjRenameInp );

    if ( status >= 0 && rodsArgs->verbose == True ) {
        ( void ) gettimeofday( &endTime, ( struct timezone * )0 );
        printTiming( conn, dataObjRenameInp->destDataObjInp.objPath,
                     0, NULL, &startTime, &endTime );
    }

    return status;
}

int
initCondForMv( dataObjCopyInp_t *dataObjRenameInp ) {
    if ( dataObjRenameInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForMv: NULL dataObjRenameInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjRenameInp, 0, sizeof( dataObjCopyInp_t ) );

    return 0;
}

