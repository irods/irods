/*** Copyright (c), The Regents of the University of California            ***
 *** For more infomkdiration please refer to files in the COPYRIGHT directory ***/
#include "irods/rodsPath.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsLog.h"
#include "irods/mkdirUtil.h"
#include "irods/miscUtil.h"
#include "irods/rcGlobalExtern.h"

int
mkdirUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs,
           rodsPathInp_t *rodsPathInp ) {
    int i = 0;
    int status = 0;
    int savedStatus = 0;
    collInp_t collCreateInp;


    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( &collCreateInp, 0, sizeof( collCreateInp ) );

    /* -p for parent */
    if ( myRodsArgs->physicalPath == True ) {
        addKeyVal( &collCreateInp.condInput, RECURSIVE_OPR__KW, "" );
    }
    for ( i = 0; i < rodsPathInp->numSrc; i++ ) {
        rstrcpy( collCreateInp.collName, rodsPathInp->srcPath[i].outPath,
                 MAX_NAME_LEN );
        status = rcCollCreate( conn, &collCreateInp );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "mkdirUtil: mkColl of %s error.",
                          rodsPathInp->srcPath[i].outPath );
            savedStatus = status;
        }
    }

    if ( savedStatus < 0 ) {
        return savedStatus;
    }
    else {
        return status;
    }
}

