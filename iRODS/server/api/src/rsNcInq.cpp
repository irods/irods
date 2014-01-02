/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See ncInq.h for a description of this API call.*/
#include "ncInq.hpp"
#include "rodsLog.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "rsApiHandler.hpp"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "getRemoteZoneResc.hpp"


int
rsNcInq( rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut ) {
    int l1descInx;
    int status = 0;

    if ( getValByKey( &ncInqInp->condInput, NATIVE_NETCDF_CALL_KW ) !=
            NULL ) {
        /* just do nc_inq */
        status = _rsNcInq( rsComm, ncInqInp, ncInqOut );
        return status;
    }
    l1descInx = ncInqInp->ncid;
    if ( l1descInx < 2 || l1descInx >= NUM_L1_DESC ) {
        rodsLog( LOG_ERROR,
                 "rsNcInq: l1descInx %d out of range",
                 l1descInx );
        return ( SYS_FILE_DESC_OUT_OF_RANGE );
    }
    if ( L1desc[l1descInx].inuseFlag != FD_INUSE ) {
        return BAD_INPUT_DESC_INDEX;
    }
    if ( L1desc[l1descInx].openedAggInfo.ncAggInfo != NULL ) {
        status = rsNcInqColl( rsComm, ncInqInp, ncInqOut );
    }
    else {
        status = rsNcInqDataObj( rsComm, ncInqInp, ncInqOut );
    }
    return status;
}

int
rsNcInqColl( rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut ) {
    int status;
    int l1descInx;
    ncOpenInp_t ncOpenInp;
    ncInqInp_t myNcInqInp;
    int i;

    l1descInx = ncInqInp->ncid;
    /* always use element 0 file aggr collection */
    if ( L1desc[l1descInx].openedAggInfo.objNcid0 == -1 ) {
        return NETCDF_AGG_ELE_FILE_NOT_OPENED;
    }
    myNcInqInp = *ncInqInp;
    myNcInqInp.ncid = L1desc[l1descInx].openedAggInfo.objNcid0;
    bzero( &myNcInqInp.condInput, sizeof( keyValPair_t ) );
    status = rsNcInqDataObj( rsComm, &myNcInqInp, ncInqOut );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "rsNcInqColl: rsNcInqDataObj error for %s", ncOpenInp.objPath );
        return status;
    }
    /* make correction to 'time' dimension */
    for ( i = 0; i < ( *ncInqOut )->ndims; i++ ) {
        if ( strcasecmp( ( *ncInqOut )->dim[i].name, "time" ) == 0 ) {
            ( *ncInqOut )->dim[i].arrayLen = sumAggElementArraylen(
                                                 L1desc[l1descInx].openedAggInfo.ncAggInfo,
                                                 L1desc[l1descInx].openedAggInfo.ncAggInfo->numFiles );
            if ( ( *ncInqOut )->dim[i].arrayLen < 0 ) {
                status = ( *ncInqOut )->dim[i].arrayLen;
                freeNcInqOut( ncInqOut );
            }
            break;
        }
    }
    return status;
}

int
rsNcInqDataObj( rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut ) {
    int remoteFlag;
    rodsServerHost_t *rodsServerHost = NULL;
    int l1descInx;
    ncInqInp_t myNcInqInp;
    int status = 0;

    l1descInx = ncInqInp->ncid;
    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        bzero( &myNcInqInp, sizeof( myNcInqInp ) );
        myNcInqInp.ncid = L1desc[l1descInx].remoteL1descInx;

        /* cross zone operation */
        status = rcNcInq( L1desc[l1descInx].remoteZoneHost->conn,
                          &myNcInqInp, ncInqOut );
    }
    else {
        remoteFlag = resoAndConnHostByDataObjInfo( rsComm,
                     L1desc[l1descInx].dataObjInfo, &rodsServerHost );
        if ( remoteFlag < 0 ) {
            return ( remoteFlag );
        }
        else if ( remoteFlag == LOCAL_HOST ) {
            myNcInqInp = *ncInqInp;
            myNcInqInp.ncid = L1desc[l1descInx].l3descInx;
            bzero( &myNcInqInp.condInput, sizeof( myNcInqInp.condInput ) );
            status = _rsNcInq( rsComm, &myNcInqInp, ncInqOut );
            if ( status < 0 ) {
                return status;
            }
        }
        else {
            /* execute it remotely */
            myNcInqInp = *ncInqInp;
            myNcInqInp.ncid = L1desc[l1descInx].l3descInx;
            bzero( &myNcInqInp.condInput, sizeof( myNcInqInp.condInput ) );
            addKeyVal( &myNcInqInp.condInput, NATIVE_NETCDF_CALL_KW, "" );
            status = rcNcInq( rodsServerHost->conn, &myNcInqInp,
                              ncInqOut );
            clearKeyVal( &myNcInqInp.condInput );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "rsNcInq: rcsNcInq %d for %s error, status = %d",
                         L1desc[l1descInx].l3descInx,
                         L1desc[l1descInx].dataObjInfo->objPath, status );
                return ( status );
            }
        }
    }
    return status;
}

int
_rsNcInq( rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut ) {
    int status = ncInq( ncInqInp, ncInqOut );
    return status;
}
