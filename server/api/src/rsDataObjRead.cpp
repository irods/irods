/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjRead.h for a description of this API call.*/

#include "dataObjRead.h"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "subStructFileRead.h"  /* XXXXX can be taken out when structFile api done */
#include "rsDataObjRead.hpp"
#include "rsSubStructFileRead.hpp"
#include "rsFileRead.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_hierarchy_parser.hpp"

int
applyRuleForPostProcForRead( rsComm_t *rsComm, bytesBuf_t *dataObjReadOutBBuf, char *objPath ) {
    if ( ReadWriteRuleState != ON_STATE ) {
        return 0;
    }

    ruleExecInfo_t rei2;
    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    msParamArray_t msParamArray;
    memset( ( char* )&msParamArray, 0, sizeof( msParamArray_t ) );

    if ( rsComm != NULL ) {
        rei2.rsComm = rsComm;
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
    }
    rei2.doi = ( dataObjInfo_t* )malloc( sizeof( dataObjInfo_t ) );
    memset(rei2.doi,0,sizeof(dataObjInfo_t));

    snprintf( rei2.doi->objPath, sizeof( rei2.doi->objPath ), "%s", objPath );

    memset( &msParamArray, 0, sizeof( msParamArray ) );
    int * myInOutStruct = ( int* )malloc( sizeof( int ) );
    *myInOutStruct = dataObjReadOutBBuf->len;
    addMsParamToArray( &msParamArray, "*ReadBuf", BUF_LEN_MS_T, myInOutStruct,
                       dataObjReadOutBBuf, 0 );
    int status = applyRule( "acPostProcForDataObjRead(*ReadBuf)", &msParamArray, &rei2,
                            NO_SAVE_REI );
    free( rei2.doi );
    if ( status < 0 ) {
        if ( rei2.status < 0 ) {
            status = rei2.status;
        }
        rodsLog( LOG_ERROR,
                 "rsDataObjRead: acPostProcForDataObjRead error=%d", status );
        clearMsParamArray( &msParamArray, 0 );
        return status;
    }
    clearMsParamArray( &msParamArray, 0 );

    return 0;

}

int
rsDataObjRead( rsComm_t *rsComm, openedDataObjInp_t *dataObjReadInp,
               bytesBuf_t *dataObjReadOutBBuf ) {
    int bytesRead;
    int l1descInx = dataObjReadInp->l1descInx;

    if ( l1descInx < 2 || l1descInx >= NUM_L1_DESC ) {
        rodsLog( LOG_NOTICE,
                 "rsDataObjRead: l1descInx %d out of range",
                 l1descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }
    if ( L1desc[l1descInx].inuseFlag != FD_INUSE ) {
        return BAD_INPUT_DESC_INDEX;
    }

    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        /* cross zone operation */
        dataObjReadInp->l1descInx = L1desc[l1descInx].remoteL1descInx;
        bytesRead = rcDataObjRead( L1desc[l1descInx].remoteZoneHost->conn,
                                   dataObjReadInp, dataObjReadOutBBuf );
        dataObjReadInp->l1descInx = l1descInx;
    }
    else {
        int i;
        bytesRead = l3Read( rsComm, l1descInx, dataObjReadInp->len,
                            dataObjReadOutBBuf );
        i = applyRuleForPostProcForRead( rsComm, dataObjReadOutBBuf,
                                         L1desc[l1descInx].dataObjInfo->objPath );
        if ( i < 0 ) {
            return i;
        }
    }

    return bytesRead;
}

int
l3Read( rsComm_t *rsComm, int l1descInx, int len,
        bytesBuf_t *dataObjReadOutBBuf ) {
    int bytesRead;

    dataObjInfo_t *dataObjInfo;
    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Read - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }


    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subStructFileFdOprInp_t subStructFileReadInp;
        memset( &subStructFileReadInp, 0, sizeof( subStructFileReadInp ) );
        subStructFileReadInp.type = dataObjInfo->specColl->type;
        subStructFileReadInp.fd = L1desc[l1descInx].l3descInx;
        subStructFileReadInp.len = len;
        rstrcpy( subStructFileReadInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( subStructFileReadInp.resc_hier, dataObjInfo->rescHier, MAX_NAME_LEN );
        bytesRead = rsSubStructFileRead( rsComm, &subStructFileReadInp, dataObjReadOutBBuf );
    }
    else {
        fileReadInp_t fileReadInp;
        int category = FILE_CAT;  // do not support DB type
        switch ( category ) {
        case FILE_CAT:
            memset( &fileReadInp, 0, sizeof( fileReadInp ) );
            fileReadInp.fileInx = L1desc[l1descInx].l3descInx;
            fileReadInp.len = len;
            bytesRead = rsFileRead( rsComm, &fileReadInp, dataObjReadOutBBuf );
            break;

        default:
            rodsLog( LOG_NOTICE,
                     "l3Read: rescCat type %d is not recognized", category );
            bytesRead = SYS_INVALID_RESC_TYPE;
            break;
        }
    }
    return bytesRead;
}

int
_l3Read( rsComm_t *rsComm, int l3descInx, void *buf, int len ) {
    fileReadInp_t fileReadInp;
    bytesBuf_t dataObjReadInpBBuf;
    int bytesRead;

    dataObjReadInpBBuf.len = len;
    dataObjReadInpBBuf.buf = buf;


    memset( &fileReadInp, 0, sizeof( fileReadInp ) );
    fileReadInp.fileInx = l3descInx;
    fileReadInp.len = len;
    bytesRead = rsFileRead( rsComm, &fileReadInp, &dataObjReadInpBBuf );

    return bytesRead;
}
