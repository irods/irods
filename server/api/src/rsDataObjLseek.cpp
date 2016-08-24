/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjLseek.h for a description of this API call.*/

#include "dataObjLseek.h"
#include "rodsLog.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "subStructFileLseek.h"
#include "objMetaOpr.hpp"
#include "subStructFileUnlink.h"
#include "rsDataObjLseek.hpp"
#include "rsSubStructFileLseek.hpp"
#include "rsFileLseek.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"

int
rsDataObjLseek( rsComm_t *rsComm, openedDataObjInp_t *dataObjLseekInp,
                fileLseekOut_t **dataObjLseekOut ) {
    int status;
    int l1descInx, l3descInx;
    dataObjInfo_t *dataObjInfo;

    l1descInx = dataObjLseekInp->l1descInx;

    if ( l1descInx <= 2 || l1descInx >= NUM_L1_DESC ) {
        rodsLog( LOG_NOTICE,
                 "rsDataObjLseek: l1descInx %d out of range",
                 l1descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }
    if ( L1desc[l1descInx].inuseFlag != FD_INUSE ) {
        return BAD_INPUT_DESC_INDEX;
    }
    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        /* cross zone operation */
        dataObjLseekInp->l1descInx = L1desc[l1descInx].remoteL1descInx;
        status = rcDataObjLseek( L1desc[l1descInx].remoteZoneHost->conn,
                                 dataObjLseekInp, dataObjLseekOut );
        dataObjLseekInp->l1descInx = l1descInx;
        return status;
    }

    l3descInx = L1desc[l1descInx].l3descInx;

    if ( l3descInx <= 2 ) {
        rodsLog( LOG_NOTICE,
                 "rsDataObjLseek: l3descInx %d out of range",
                 l3descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "rsDataObjLseek - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }


    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subStructFileLseekInp_t subStructFileLseekInp;
        memset( &subStructFileLseekInp, 0, sizeof( subStructFileLseekInp ) );
        subStructFileLseekInp.type = dataObjInfo->specColl->type;
        subStructFileLseekInp.fd = L1desc[l1descInx].l3descInx;
        subStructFileLseekInp.offset = dataObjLseekInp->offset;
        subStructFileLseekInp.whence = dataObjLseekInp->whence;
        rstrcpy( subStructFileLseekInp.addr.hostAddr,
                 location.c_str(),
                 NAME_LEN );
        rstrcpy( subStructFileLseekInp.resc_hier,
                 dataObjInfo->rescHier,
                 NAME_LEN );
        status = rsSubStructFileLseek( rsComm, &subStructFileLseekInp, dataObjLseekOut );
    }
    else {
        *dataObjLseekOut = ( fileLseekOut_t* )malloc( sizeof( fileLseekOut_t ) );
        memset( *dataObjLseekOut, 0, sizeof( fileLseekOut_t ) );

        ( *dataObjLseekOut )->offset = _l3Lseek( rsComm, l3descInx,
                                       dataObjLseekInp->offset, dataObjLseekInp->whence );

        if ( ( *dataObjLseekOut )->offset >= 0 ) {
            status = 0;
        }
        else {
            status = ( *dataObjLseekOut )->offset;
        }
    }

    return status;
}

rodsLong_t
_l3Lseek( rsComm_t *rsComm, int l3descInx,
          rodsLong_t offset, int whence ) {
    fileLseekInp_t fileLseekInp;
    fileLseekOut_t *fileLseekOut = NULL;
    int status;

    memset( &fileLseekInp, 0, sizeof( fileLseekInp ) );
    fileLseekInp.fileInx = l3descInx;
    fileLseekInp.offset = offset;
    fileLseekInp.whence = whence;
    status = rsFileLseek( rsComm, &fileLseekInp, &fileLseekOut );
    if ( status < 0 ) {
        return status;
    }
    else {
        rodsLong_t off = fileLseekOut->offset;
        free( fileLseekOut );
        return off;
    }
}
