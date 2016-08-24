/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjWrite.h for a description of this API call.*/

#include "dataObjWrite.h"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "subStructFileWrite.h"
#include "fileWrite.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "subStructFileRead.h"  /* XXXXX can be taken out when structFile api done */
#include "rsDataObjWrite.hpp"
#include "rsSubStructFileWrite.hpp"
#include "rsFileWrite.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_file_object.hpp"
#include "irods_resource_redirect.hpp"


int
applyRuleForPostProcForWrite( rsComm_t *rsComm, bytesBuf_t *dataObjWriteInpBBuf, char *objPath ) {
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
    *myInOutStruct = dataObjWriteInpBBuf->len;
    addMsParamToArray( &msParamArray, "*WriteBuf", BUF_LEN_MS_T, myInOutStruct,
                       dataObjWriteInpBBuf, 0 );
    int status =  applyRule( "acPostProcForDataObjWrite(*WriteBuf)", &msParamArray, &rei2, NO_SAVE_REI );
    free( rei2.doi );
    if ( status < 0 ) {
        if ( rei2.status < 0 ) {
            status = rei2.status;
        }
        rodsLog( LOG_ERROR,
                 "rsDataObjWrite: acPostProcForDataObjWrite error=%d", status );
        clearMsParamArray( &msParamArray, 0 );
        return status;
    }
    clearMsParamArray( &msParamArray, 0 );

    return 0;

}

int rsDataObjWrite(
    rsComm_t*           rsComm,
    openedDataObjInp_t* dataObjWriteInp,
    bytesBuf_t*         dataObjWriteInpBBuf ) {
    int bytesWritten = 0;
    int l1descInx    = dataObjWriteInp->l1descInx;

    if ( l1descInx < 2 || l1descInx >= NUM_L1_DESC ) {
        rodsLog(
            LOG_NOTICE,
            "rsDataObjWrite: l1descInx %d out of range",
            l1descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    if ( L1desc[l1descInx].inuseFlag != FD_INUSE ) {
        return BAD_INPUT_DESC_INDEX;
    }

    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        // =-=-=-=-=-=-=-
        // cross zone operation
        dataObjWriteInp->l1descInx = L1desc[l1descInx].remoteL1descInx;
        bytesWritten = rcDataObjWrite(
                           L1desc[l1descInx].remoteZoneHost->conn,
                           dataObjWriteInp,
                           dataObjWriteInpBBuf );
        dataObjWriteInp->l1descInx = l1descInx;
    }
    else {
        int i = applyRuleForPostProcForWrite(
                    rsComm,
                    dataObjWriteInpBBuf,
                    L1desc[l1descInx].dataObjInfo->objPath );
        if ( i < 0 ) {
            return i;
        }

        // =-=-=-=-=-=-=-
        // notify the resource hierarchy that something is afoot
        irods::file_object_ptr file_obj(
            new irods::file_object(
                rsComm,
                L1desc[l1descInx].dataObjInfo ) );
        char* pdmo_kw = getValByKey( &dataObjWriteInp->condInput, IN_PDMO_KW );
        if ( pdmo_kw != NULL ) {
            file_obj->in_pdmo( pdmo_kw );
        }
        irods::error ret = fileNotify(
                               rsComm,
                               file_obj,
                               irods::WRITE_OPERATION );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "Failed to signal the resource that the data object \"";
            msg << L1desc[l1descInx].dataObjInfo->objPath;
            msg << "\" was modified.";
            ret = PASSMSG( msg.str(), ret );
            irods::log( ret );
            return ret.code();
        }

        dataObjWriteInp->len = dataObjWriteInpBBuf->len;
        bytesWritten = l3Write(
                           rsComm,
                           l1descInx,
                           dataObjWriteInp->len,
                           dataObjWriteInpBBuf );
    }

    return bytesWritten;
}

int
l3Write( rsComm_t *rsComm, int l1descInx, int len,
         bytesBuf_t *dataObjWriteInpBBuf ) {
    fileWriteInp_t fileWriteInp;
    int bytesWritten;

    dataObjInfo_t *dataObjInfo;
    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Write - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subStructFileFdOprInp_t subStructFileWriteInp;
        memset( &subStructFileWriteInp, 0, sizeof( subStructFileWriteInp ) );
        subStructFileWriteInp.type = dataObjInfo->specColl->type;
        subStructFileWriteInp.fd = L1desc[l1descInx].l3descInx;
        subStructFileWriteInp.len = len;
        rstrcpy( subStructFileWriteInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( subStructFileWriteInp.resc_hier, dataObjInfo->rescHier, MAX_NAME_LEN );
        bytesWritten = rsSubStructFileWrite( rsComm, &subStructFileWriteInp, dataObjWriteInpBBuf );

    }
    else {
        memset( &fileWriteInp, 0, sizeof( fileWriteInp ) );
        fileWriteInp.fileInx = L1desc[l1descInx].l3descInx;
        fileWriteInp.len = len;
        bytesWritten = rsFileWrite( rsComm, &fileWriteInp,
                                    dataObjWriteInpBBuf );
        if ( bytesWritten > 0 ) {
            L1desc[l1descInx].bytesWritten += bytesWritten;
        }
    }
    return bytesWritten;
}

int
_l3Write( rsComm_t *rsComm, int l3descInx,
          void *buf, int len ) {
    fileWriteInp_t fileWriteInp;
    bytesBuf_t dataObjWriteInpBBuf;
    int bytesWritten;

    dataObjWriteInpBBuf.len = len;
    dataObjWriteInpBBuf.buf = buf;
    memset( &fileWriteInp, 0, sizeof( fileWriteInp ) );
    fileWriteInp.fileInx = l3descInx;
    fileWriteInp.len = len;
    bytesWritten = rsFileWrite( rsComm, &fileWriteInp,
                                &dataObjWriteInpBBuf );
    return bytesWritten;
}
