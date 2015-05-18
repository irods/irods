/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "phybunUtil.h"
#include "miscUtil.h"
#include "rcGlobalExtern.h"

int
phybunUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs,
            rodsPathInp_t *rodsPathInp ) {
    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    rodsPath_t *collPath;
    structFileExtAndRegInp_t phyBundleCollInp;
    int savedStatus = initCondForPhybunOpr( myRodsArgs, &phyBundleCollInp );
    if ( savedStatus < 0 ) {
        return savedStatus;
    }

    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        collPath = &rodsPathInp->srcPath[i];	/* iRODS Collection */

        int status = getRodsObjType( conn, collPath );

        if ( collPath->objType !=  COLL_OBJ_T ) {
            rodsLogError( LOG_ERROR, status,
                          "phybunUtil: input path %s is not a collection",
                          collPath->outPath );
            return USER_INPUT_PATH_ERR;
        }

        rstrcpy( phyBundleCollInp.collection, collPath->outPath, MAX_NAME_LEN );

        status = rcPhyBundleColl( conn, &phyBundleCollInp );
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLogError( LOG_ERROR, status,
                          "phybunUtil: opr error for %s, status = %d",
                          collPath->outPath, status );
            savedStatus = status;
        }
    }
    return savedStatus;
}

int
initCondForPhybunOpr( rodsArguments_t *rodsArgs,
                      structFileExtAndRegInp_t *phyBundleCollInp ) {
    char tmpStr[NAME_LEN]; // JMC - backport 4771

    if ( phyBundleCollInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForPhybunOpr: NULL phyBundleCollInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( phyBundleCollInp, 0, sizeof( structFileExtAndRegInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForPhybunOpr: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &phyBundleCollInp->condInput,
                       DEST_RESC_NAME_KW, rodsArgs->resourceString );
        }
    }
    else {
        rodsLog( LOG_ERROR,
                 "initCondForPhybunOpr: A -Rresource must be input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsArgs->srcResc == True ) {
        addKeyVal( &phyBundleCollInp->condInput, RESC_NAME_KW,
                   rodsArgs->srcRescString );
    }

    if ( rodsArgs->verifyChecksum == True ) { // JMC - backport 4528
        addKeyVal( &phyBundleCollInp->condInput, VERIFY_CHKSUM_KW, "" );
    }
    // =-=-=-=-=-=-=
    // JMC - backport 4658
    if ( rodsArgs->dataTypeString != NULL ) {
        if ( strcmp( rodsArgs->dataTypeString, "t" ) == 0 ||
                strcmp( rodsArgs->dataTypeString, TAR_DT_STR ) == 0 ||
                strcmp( rodsArgs->dataTypeString, "tar" ) == 0 ) {
            addKeyVal( &phyBundleCollInp->condInput, DATA_TYPE_KW,
                       TAR_BUNDLE_DT_STR );
        }
        else if ( strcmp( rodsArgs->dataTypeString, "g" ) == 0 ||
                  strcmp( rodsArgs->dataTypeString, GZIP_TAR_DT_STR ) == 0 ||
                  strcmp( rodsArgs->dataTypeString, "gzip" ) == 0 ) {
            addKeyVal( &phyBundleCollInp->condInput, DATA_TYPE_KW,
                       GZIP_TAR_BUNDLE_DT_STR );
        }
        else if ( strcmp( rodsArgs->dataTypeString, "b" ) == 0 ||
                  strcmp( rodsArgs->dataTypeString, BZIP2_TAR_DT_STR ) == 0 ||
                  strcmp( rodsArgs->dataTypeString, "bzip" ) == 0 ) {
            addKeyVal( &phyBundleCollInp->condInput, DATA_TYPE_KW,
                       BZIP2_TAR_BUNDLE_DT_STR );
        }
        else if ( strcmp( rodsArgs->dataTypeString, "z" ) == 0 ||
                  strcmp( rodsArgs->dataTypeString, ZIP_DT_STR ) == 0 ||
                  strcmp( rodsArgs->dataTypeString, "zip" ) == 0 ) {
            addKeyVal( &phyBundleCollInp->condInput, DATA_TYPE_KW,
                       ZIP_BUNDLE_DT_STR );
        }
        else {
            addKeyVal( &phyBundleCollInp->condInput, DATA_TYPE_KW,
                       rodsArgs->dataTypeString );
        }
    }
    // =-=-=-=-=-=-=
    if ( rodsArgs->number == True ) { // JMC - backport 4771
        snprintf( tmpStr, NAME_LEN, "%d", rodsArgs->numberValue );
        addKeyVal( &phyBundleCollInp->condInput, MAX_SUB_FILE_KW, tmpStr );
    }

    if ( rodsArgs->sizeFlag == True ) {
        char tmpStr1[NAME_LEN];
        snprintf( tmpStr1, sizeof( tmpStr1 ), "%ji", ( intmax_t )rodsArgs->size );
        addKeyVal( &phyBundleCollInp->condInput, MAX_BUNDLE_SIZE_KW, tmpStr1 );
    }

    return 0;
}
