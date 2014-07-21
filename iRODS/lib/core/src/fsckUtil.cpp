/*** Copyright (c) 2010 Data Intensive Cyberinfrastructure Foundation. All rights reserved.    ***
 *** For full copyright notice please refer to files in the COPYRIGHT directory                ***/
/* Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation */

#include "rodsPath.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"
#include "fsckUtil.hpp"
#include "miscUtil.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

int
fsckObj( rcComm_t *conn, rodsArguments_t *myRodsArgs, rodsPathInp_t *rodsPathInp, char hostname[LONG_NAME_LEN] ) {
    char inpPath[LONG_NAME_LEN] = "";
    char *inpPathO, *lastChar;
    int lenInpPath, status;

    if ( rodsPathInp->numSrc != 1 ) {
        rodsLog( LOG_ERROR, "fsckObj: gave %i input source path, should give one and only one", rodsPathInp->numSrc );
        status = USER_INPUT_PATH_ERR;
    }
    else {
        inpPathO = rodsPathInp->srcPath[0].outPath;
        path p( inpPathO );
        if ( exists( p ) ) {
            /* don't do anything if it is symlink */
            if ( is_symlink( p ) ) {
                return 0;
            }
            /* remove any trailing "/" from inpPathO */
            lenInpPath = strlen( inpPathO );
            lastChar = strrchr( inpPathO, '/' );
            if ( lastChar && strlen( lastChar ) == 1 ) { // JMC - backport 4578
                lenInpPath = lenInpPath - 1;
            }
            strncpy( inpPath, inpPathO, lenInpPath );
            if ( is_directory( p ) ) {
                status = checkIsMount( conn, inpPath );
                if ( status != 0 ) {  /* if it is part of a mounted collection, abort */
                    printf( "The directory %s or one of its subdirectories to be checked is declared as being \
                            used for a mounted collection: abort!\n", inpPath );
                    return ( status );
                }
            }
            status = fsckObjDir( conn, myRodsArgs, inpPath, hostname );
        }
        else {
            status = USER_INPUT_PATH_ERR;
            rodsLog( LOG_ERROR, "fsckObj: %s does not exist", inpPathO );
        }
    }

    return ( status );
}

int
fsckObjDir( rcComm_t *conn, rodsArguments_t *myRodsArgs, char *inpPath, char *hostname ) {
    int status = 0;
    char fullPath[LONG_NAME_LEN] = "\0";

    /* check if it is a directory */
    path srcDirPath( inpPath );
    if ( is_symlink( srcDirPath ) ) {
        /* don't do anything if it is symlink */
        return 0;
    }
    else if ( is_directory( srcDirPath ) ) {
    }
    else {
        status = chkObjConsistency( conn, myRodsArgs, inpPath, hostname );
        return ( status );
    }
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path cp = itr->path();
        snprintf( fullPath, LONG_NAME_LEN, "%s",
                  cp.c_str() );
        if ( is_symlink( cp ) ) {
            /* don't do anything if it is symlink */
            continue;
        }
        else if ( is_directory( cp ) ) {
            if ( myRodsArgs->recursive == True ) {
                status = fsckObjDir( conn, myRodsArgs, fullPath, hostname );
            }
        }
        else {
            status = chkObjConsistency( conn, myRodsArgs, fullPath, hostname );
        }
    }
    return ( status );

}

int
chkObjConsistency( rcComm_t *conn, rodsArguments_t *myRodsArgs, char *inpPath, char *hostname ) {
    int objSize, srcSize, status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char condStr[MAX_NAME_LEN], locChksum[NAME_LEN], *objChksum, *objName, *objPath;
    /* retrieve the local file size */
    path p( inpPath );
    /* don't do anything if it is symlink */
    if ( is_symlink( p ) ) {
        return 0;
    }
    srcSize = file_size( p );

    /* retrieve object size and checksum in iRODS */
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    addInxIval( &genQueryInp.selectInp, COL_DATA_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_SIZE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_DATA_CHECKSUM, 1 );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf( condStr, MAX_NAME_LEN, "='%s'", inpPath );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr );
    snprintf( condStr, MAX_NAME_LEN, "like '%s%s' || ='%s'", hostname, "%", hostname );
    addInxVal( &genQueryInp.sqlCondInp, COL_R_LOC, condStr );

    status =  rcGenQuery( conn, &genQueryInp, &genQueryOut );
    if ( status != CAT_NO_ROWS_FOUND && NULL != genQueryOut ) {
        objName = genQueryOut->sqlResult[0].value;
        objPath = genQueryOut->sqlResult[1].value;
        objSize = atoi( genQueryOut->sqlResult[2].value );
        objChksum = genQueryOut->sqlResult[3].value;
        if ( srcSize == objSize ) {
            if ( myRodsArgs->verifyChecksum == True ) {
                if ( strcmp( objChksum, "" ) != 0 ) {
                    status = verifyChksumLocFile( inpPath, objChksum, NULL );
                    if ( status == 0 ) {
                        if ( strcmp( locChksum, objChksum ) != 0 ) {
                            printf( "CORRUPTION: local file %s checksum not consistent with \
                                    iRODS object %s/%s checksum.\n", inpPath, objPath, objName );
                        }
                    }
                    else {
                        printf( "ERROR: unable to compute checksum for local file %s.\n", inpPath );
                    }
                }
                else {
                    printf( "WARNING: checksum not available for iRODS object %s/%s, no checksum comparison \
                            possible with local file %s .\n", objPath, objName, inpPath );
                }
            }
        }
        else {
            printf( "CORRUPTION: local file %s size not consistent with iRODS object %s/%s size.\n", \
                    inpPath, objPath, objName );
        }
    }

    clearGenQueryInp( &genQueryInp );
    freeGenQueryOut( &genQueryOut );

    return ( status );

}
