#include "reGlobalsExtern.hpp"
#include "dataObjChksum.hpp"
#include "dataObjRsync.hpp"
#include "objMetaOpr.hpp"
#include "specColl.hpp"
#include "dataObjOpr.hpp"
#include "rsApiHandler.hpp"
#include "modDataObjMeta.hpp"
#include "getRemoteZoneResc.hpp"

#include "irods_resource_redirect.hpp"

int
rsDataObjRsync( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                msParamArray_t **outParamArray ) {
    int status;
    char *rsyncMode;
    char *remoteZoneOpr;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;

    *outParamArray = NULL;
    if ( dataObjInp == NULL ) {
        rodsLog( LOG_ERROR, "rsDataObjRsync error. NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsyncMode = getValByKey( &dataObjInp->condInput, RSYNC_MODE_KW );
    if ( rsyncMode == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsDataObjRsync: RSYNC_MODE_KW input is missing" );
        return USER_RSYNC_NO_MODE_INPUT_ERR;
    }

    if ( strcmp( rsyncMode, LOCAL_TO_IRODS ) == 0 ) {
        remoteZoneOpr = REMOTE_CREATE;
    }
    else {
        remoteZoneOpr = REMOTE_OPEN;
    }

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    if ( strcmp( rsyncMode, IRODS_TO_IRODS ) == 0 ) {
        if ( isLocalZone( dataObjInp->objPath ) == 0 ) {
            dataObjInp_t myDataObjInp;
            char *destObjPath;
            /* source in a remote zone. try dest */
            destObjPath = getValByKey( &dataObjInp->condInput,
                                       RSYNC_DEST_PATH_KW );
            if ( destObjPath == NULL ) {
                rodsLog( LOG_ERROR,
                         "rsDataObjRsync: RSYNC_DEST_PATH_KW input is missing for %s",
                         dataObjInp->objPath );
                return USER_RSYNC_NO_MODE_INPUT_ERR;
            }
            myDataObjInp = *dataObjInp;
            remoteZoneOpr = REMOTE_CREATE;
            rstrcpy( myDataObjInp.objPath, destObjPath, MAX_NAME_LEN );
            remoteFlag = getAndConnRemoteZone( rsComm, &myDataObjInp,
                                               &rodsServerHost, remoteZoneOpr );
        }
        else {
            remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp,
                                               &rodsServerHost, remoteZoneOpr );
        }
    }
    else {
        remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                           remoteZoneOpr );
    }

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {

        status = _rcDataObjRsync( rodsServerHost->conn, dataObjInp,
                                  outParamArray );
        return status;
    }

    if ( strcmp( rsyncMode, IRODS_TO_LOCAL ) == 0 ) {
        status = rsRsyncFileToData( rsComm, dataObjInp );
    }
    else if ( strcmp( rsyncMode, LOCAL_TO_IRODS ) == 0 ) {
        status = rsRsyncDataToFile( rsComm, dataObjInp );
    }
    else if ( strcmp( rsyncMode, IRODS_TO_IRODS ) == 0 ) {
        status = rsRsyncDataToData( rsComm, dataObjInp );
    }
    else {
        rodsLog( LOG_ERROR,
                 "rsDataObjRsync: rsyncMode %s  not supported" );
        return USER_RSYNC_NO_MODE_INPUT_ERR;
    }

    return status;
}

int
rsRsyncDataToFile( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status;
    char *fileChksumStr = NULL;
    char *dataObjChksumStr = NULL;
    dataObjInfo_t *dataObjInfoHead = NULL;

    fileChksumStr = getValByKey( &dataObjInp->condInput, RSYNC_CHKSUM_KW );

    if ( fileChksumStr == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsRsyncDataToFile: RSYNC_CHKSUM_KW input is missing for %s",
                 dataObjInp->objPath );
        return CHKSUM_EMPTY_IN_STRUCT_ERR;
    }

    status = _rsDataObjChksum( rsComm, dataObjInp, &dataObjChksumStr,
                               &dataObjInfoHead );

    if ( status < 0 && status != CAT_NO_ACCESS_PERMISSION &&
            status != CAT_NO_ROWS_FOUND ) {
        /* XXXXX CAT_NO_ACCESS_PERMISSION mean the chksum was calculated but
         * cannot be registered. But the chksum value is OK.
         */
        rodsLog( LOG_ERROR,
                 "rsRsyncDataToFile: _rsDataObjChksum of %s error. status = %d",
                 dataObjInp->objPath, status );
        return status;
    }

    freeAllDataObjInfo( dataObjInfoHead );

    status = dataObjChksumStr && strcmp( dataObjChksumStr, fileChksumStr ) == 0 ?
             0 : SYS_SVR_TO_CLI_GET_ACTION;

    free( dataObjChksumStr );

    return status;
}

int
rsRsyncFileToData( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {

    char * fileChksumStr = getValByKey( &dataObjInp->condInput, RSYNC_CHKSUM_KW );

    if ( fileChksumStr == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsRsyncFileToData: RSYNC_CHKSUM_KW input is missing" );
        return CHKSUM_EMPTY_IN_STRUCT_ERR;
    }

    // =-=-=-=-=-=-=-
    // determine the resource hierarchy if one is not provided
    if ( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
        std::string       hier;
        irods::error ret = irods::resolve_resource_hierarchy( irods::OPEN_OPERATION,
                           rsComm, dataObjInp, hier );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " :: failed in irods::resolve_resource_hierarchy for [";
            msg << dataObjInp->objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword

    char *dataObjChksumStr = NULL;
    dataObjInfo_t *dataObjInfoHead = NULL;
    int status = _rsDataObjChksum( rsComm, dataObjInp, &dataObjChksumStr,
                                   &dataObjInfoHead );

    if ( status < 0 && status != CAT_NO_ACCESS_PERMISSION &&
            status != CAT_NO_ROWS_FOUND ) {
        /* XXXXX CAT_NO_ACCESS_PERMISSION mean the chksum was calculated but
         * cannot be registered. But the chksum value is OK.
         */
        rodsLog( LOG_ERROR,
                 "rsRsyncFileToData: _rsDataObjChksum of %s error. status = %d",
                 dataObjInp->objPath, status );
    }

    freeAllDataObjInfo( dataObjInfoHead );

    if ( dataObjChksumStr != NULL &&
            strcmp( dataObjChksumStr, fileChksumStr ) == 0 ) {
        free( dataObjChksumStr );
        return 0;
    }
    free( dataObjChksumStr );
    return SYS_SVR_TO_CLI_PUT_ACTION;
}

int
rsRsyncDataToData( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status;
    char *srcChksumStr = NULL;
    char *destChksumStr = NULL;
    dataObjCopyInp_t dataObjCopyInp;
    char *destObjPath;
    transferStat_t *transStat = NULL;

    /* always have the FORCE flag on */
    addKeyVal( &dataObjInp->condInput, FORCE_FLAG_KW, "" );

    destObjPath = getValByKey( &dataObjInp->condInput, RSYNC_DEST_PATH_KW );
    if ( destObjPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "rsRsyncDataToData: RSYNC_DEST_PATH_KW input is missing for %s",
                 dataObjInp->objPath );
        return USER_RSYNC_NO_MODE_INPUT_ERR;
    }

    memset( &dataObjCopyInp, 0, sizeof( dataObjCopyInp ) );
    rstrcpy( dataObjCopyInp.srcDataObjInp.objPath, dataObjInp->objPath,
             MAX_NAME_LEN );
    dataObjCopyInp.srcDataObjInp.dataSize = dataObjInp->dataSize;
    replDataObjInp( dataObjInp, &dataObjCopyInp.destDataObjInp );
    rstrcpy( dataObjCopyInp.destDataObjInp.objPath, destObjPath,
             MAX_NAME_LEN );

    /* use rsDataObjChksum because the path could in in remote zone */
    status = rsDataObjChksum( rsComm, &dataObjCopyInp.srcDataObjInp,
                              &srcChksumStr );

    if ( status < 0 &&
            ( status != CAT_NO_ACCESS_PERMISSION || srcChksumStr == NULL ) ) {
        /* XXXXX CAT_NO_ACCESS_PERMISSION mean the chksum was calculated but
         * cannot be registered. But the chksum value is OK.
         */
        rodsLog( LOG_ERROR,
                 "rsRsyncDataToData: _rsDataObjChksum error for %s, status = %d",
                 dataObjCopyInp.srcDataObjInp.objPath, status );
        clearKeyVal( &dataObjCopyInp.srcDataObjInp.condInput );
        return status;
    }

    /* use rsDataObjChksum because the path could in in remote zone */
    status = rsDataObjChksum( rsComm, &dataObjCopyInp.destDataObjInp,
                              &destChksumStr );

    if ( status < 0 && status != CAT_NO_ACCESS_PERMISSION &&
            status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_ERROR,
                 "rsRsyncDataToData: _rsDataObjChksum error for %s, status = %d",
                 dataObjCopyInp.destDataObjInp.objPath, status );
        free( srcChksumStr );
        free( destChksumStr );
        clearKeyVal( &dataObjCopyInp.srcDataObjInp.condInput );
        clearKeyVal( &dataObjCopyInp.destDataObjInp.condInput );
        return status;
    }

    if ( destChksumStr != NULL && strcmp( srcChksumStr, destChksumStr ) == 0 ) {
        free( srcChksumStr );
        free( destChksumStr );
        clearKeyVal( &dataObjCopyInp.destDataObjInp.condInput );
        clearKeyVal( &dataObjCopyInp.srcDataObjInp.condInput );
        return 0;
    }

    addKeyVal( &dataObjCopyInp.destDataObjInp.condInput, REG_CHKSUM_KW,
               srcChksumStr );
    status = rsDataObjCopy( rsComm, &dataObjCopyInp, &transStat );
    free( transStat );
    free( srcChksumStr );
    free( destChksumStr );
    clearKeyVal( &dataObjCopyInp.destDataObjInp.condInput );
    clearKeyVal( &dataObjCopyInp.srcDataObjInp.condInput );

    if ( status >= 0 ) {
        return SYS_RSYNC_TARGET_MODIFIED;
    }
    else {
        return status;
    }
}

