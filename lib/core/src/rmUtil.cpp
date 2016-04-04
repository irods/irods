/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "miscUtil.h"
#include "rodsLog.h"
#include "rmUtil.h"
#include "rcGlobalExtern.h"

int
rmUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs,
        rodsPathInp_t *rodsPathInp ) {
    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    collInp_t collInp;
    dataObjInp_t dataObjInp;
    initCondForRm( myRodsArgs, &dataObjInp, &collInp );

    int savedStatus = 0;
    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        if ( rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T ) {
            getRodsObjType( conn, &rodsPathInp->srcPath[i] );
            if ( rodsPathInp->srcPath[i].objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR,
                         "rmUtil: srcPath %s does not exist",
                         rodsPathInp->srcPath[i].outPath );
                savedStatus = USER_INPUT_PATH_ERR;
                continue;
            }
        }

        int status = 0;
        if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
            status = rmDataObjUtil( conn, rodsPathInp->srcPath[i].outPath,
                                    myRodsArgs, &dataObjInp );
        }
        else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
            status = rmCollUtil( conn, rodsPathInp->srcPath[i].outPath,
                                 myRodsArgs, &collInp );
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "rmUtil: invalid rm objType %d for %s",
                     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLogError( LOG_ERROR, status,
                          "rmUtil: rm error for %s, status = %d",
                          rodsPathInp->srcPath[i].outPath, status );
            savedStatus = status;
        }
    }
    return savedStatus;
}

int
rmDataObjUtil( rcComm_t *conn, char *srcPath,
               rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp ) {
    int status;
    struct timeval startTime, endTime;

    if ( srcPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rmDataObjUtil: NULL srcPath input" );
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
initCondForRm( rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp,
               collInp_t *collInp ) {

#ifdef _WIN32
    struct _timeb timebuffer;
#endif

    if ( dataObjInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForRm: NULL dataObjInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjInp, 0, sizeof( dataObjInp_t ) );
    memset( collInp, 0, sizeof( collInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    if ( rodsArgs->unmount == True ) {
        dataObjInp->oprType = collInp->oprType = UNREG_OPR;
    }

    if ( rodsArgs->force == True ) {
        addKeyVal( &dataObjInp->condInput, FORCE_FLAG_KW, "" );
        addKeyVal( &collInp->condInput, FORCE_FLAG_KW, "" );
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4552
    if ( rodsArgs->empty == True ) {
        addKeyVal( &dataObjInp->condInput, EMPTY_BUNDLE_ONLY_KW, "" );
        addKeyVal( &collInp->condInput, EMPTY_BUNDLE_ONLY_KW, "" );
    }
    // =-=-=-=-=-=-=-
    if ( rodsArgs->replNum == True ) {
        addKeyVal( &dataObjInp->condInput, REPL_NUM_KW,
                   rodsArgs->replNumValue );
        if ( rodsArgs->recursive == True ) {
            rodsLog( LOG_NOTICE,
                     "initCondForRm: -n option is only for dataObj removal. It will be ignored for collection removal" );
        }
    }

    if ( rodsArgs->recursive == True ) {
        addKeyVal( &dataObjInp->condInput, RECURSIVE_OPR__KW, "" );
        addKeyVal( &collInp->condInput, RECURSIVE_OPR__KW, "" );
    }


    /* XXXXX need to add -u register cond */

    dataObjInp->openFlags = O_RDONLY;

    seedRandom();

    return 0;
}

int
rmCollUtil( rcComm_t *conn, char *srcColl,
            rodsArguments_t *rodsArgs, collInp_t *collInp ) {
    int status;

    if ( srcColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "rmCollUtil: NULL srcColl input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->recursive != True ) {
        rodsLog( LOG_ERROR,
                 "rmCollUtil: -r option must be used for collection %s",
                 srcColl );
        return USER_INPUT_OPTION_ERR;
    }

    rstrcpy( collInp->collName, srcColl, MAX_NAME_LEN );
    status = rcRmColl( conn, collInp, rodsArgs->verbose );

    return status;
}
