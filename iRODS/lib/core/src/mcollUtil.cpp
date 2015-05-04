/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "mcollUtil.h"
#include "miscUtil.h"
#include "rcGlobalExtern.h"

/* redefine of some input option */
#define syncCacheDir	sizeFlag
#define purgeCacheDir   physicalPath
int
mcollUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
           rodsPathInp_t *rodsPathInp ) {
    rodsPath_t *destPath, *srcPath;
    dataObjInp_t dataObjOprInp;

    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    int savedStatus = initCondForMcoll( myRodsEnv, myRodsArgs, &dataObjOprInp );
    if ( savedStatus < 0 ) {
        return savedStatus;
    }

    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        int status;
        if ( myRodsArgs->mountCollection == True ) {
            destPath = &rodsPathInp->destPath[i];	/* iRODS path */
            srcPath = &rodsPathInp->srcPath[i];	/* file Path */

            getRodsObjType( conn, destPath );

            addKeyVal( &dataObjOprInp.condInput, FILE_PATH_KW,
                       srcPath->outPath );
            rstrcpy( dataObjOprInp.objPath, destPath->outPath, MAX_NAME_LEN );
            status = rcPhyPathReg( conn, &dataObjOprInp );
        }
        else if ( myRodsArgs->unmount == True ) {	/* unmount */
            srcPath = &rodsPathInp->srcPath[i]; /* file Path */

            getRodsObjType( conn, srcPath );

            rstrcpy( dataObjOprInp.objPath, srcPath->outPath, MAX_NAME_LEN );
            status = rcPhyPathReg( conn, &dataObjOprInp );
        }
        else if ( myRodsArgs->syncCacheDir == True ||
                  myRodsArgs->purgeCacheDir == True ) {  /* sync or purge */
            srcPath = &rodsPathInp->srcPath[i]; /* file Path */

            getRodsObjType( conn, srcPath );

            rstrcpy( dataObjOprInp.objPath, srcPath->outPath, MAX_NAME_LEN );
            status = rcSyncMountedColl( conn, &dataObjOprInp );
        }
        else {
            status = USER_INPUT_OPTION_ERR;
        }

        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLogError( LOG_ERROR, status,
                          "imcoll error" );
            savedStatus = status;
        }
    }
    return savedStatus;
}

int
initCondForMcoll( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                  dataObjInp_t *dataObjOprInp ) {
    if ( dataObjOprInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForReg: NULL dataObjOprInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjOprInp, 0, sizeof( dataObjInp_t ) );

    if ( rodsArgs == NULL ) {
        return USER_INPUT_OPTION_ERR;
    }

    if ( rodsArgs->collection == True ) {
        addKeyVal( &dataObjOprInp->condInput, COLLECTION_KW, "" );
    }

    if ( rodsArgs->purgeCacheDir == True ) {
        dataObjOprInp->oprType =
            dataObjOprInp->oprType | PURGE_STRUCT_FILE_CACHE;
    }

    if ( rodsArgs->unmount == True ) {
        addKeyVal( &dataObjOprInp->condInput,
                   COLLECTION_TYPE_KW, UNMOUNT_STR );
    }
    else if ( rodsArgs->mountCollection == True ) {
        char *mountType = rodsArgs->mountType;
        if ( strcmp( mountType, "h" ) == 0 ||
                strcmp( mountType, "haaw" ) == 0 ) {
            addKeyVal( &dataObjOprInp->condInput,
                       COLLECTION_TYPE_KW, HAAW_STRUCT_FILE_STR );
        }
        else if ( strcmp( mountType, "m" ) == 0 ||
                  strcmp( mountType, "msso" ) == 0 ) {
            addKeyVal( &dataObjOprInp->condInput,
                       COLLECTION_TYPE_KW, MSSO_STRUCT_FILE_STR );
            /* XXXXX need to add "msso structFile" token */
            addKeyVal( &dataObjOprInp->condInput, DATA_TYPE_KW, MSSO_DT_STR );
        }
        else if ( strcmp( mountType, "t" ) == 0 ||
                  strcmp( mountType, "tar" ) == 0 ) {
            addKeyVal( &dataObjOprInp->condInput,
                       COLLECTION_TYPE_KW, TAR_STRUCT_FILE_STR );
            /* XXXXX need to add "tar structFile" token */
            addKeyVal( &dataObjOprInp->condInput, DATA_TYPE_KW, TAR_DT_STR );
        }
        else if ( strcmp( mountType, "f" ) == 0 ||
                  strcmp( mountType, "filesystem" ) == 0 ) {
            /* the collection is a filesystem mount point */
            addKeyVal( &dataObjOprInp->condInput,
                       COLLECTION_TYPE_KW, MOUNT_POINT_STR );
        }
        else if ( strcmp( mountType, "l" ) == 0 ||
                  strcmp( mountType, "link" ) == 0 ) {
            /* the collection is a softlink point */
            addKeyVal( &dataObjOprInp->condInput,
                       COLLECTION_TYPE_KW, LINK_POINT_STR );
        }
    }
    else if ( rodsArgs->purgeCacheDir == True ) {
        dataObjOprInp->oprType =
            dataObjOprInp->oprType | PURGE_STRUCT_FILE_CACHE;
    }
    else if ( rodsArgs->syncCacheDir != True ) {
        /* should not be here */
        rodsLog( LOG_ERROR, "initCondForMcoll: -U or -m must be used. " );
        return USER_INPUT_OPTION_ERR;
    }

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForReg: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &dataObjOprInp->condInput, DEST_RESC_NAME_KW,
                       rodsArgs->resourceString );
        }
    }
    else if ( myRodsEnv != NULL && strlen( myRodsEnv->rodsDefResource ) > 0 ) {
        addKeyVal( &dataObjOprInp->condInput, DEST_RESC_NAME_KW,
                   myRodsEnv->rodsDefResource );
    }

    return 0;
}

