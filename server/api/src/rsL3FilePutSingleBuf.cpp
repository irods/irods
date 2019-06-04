/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See l3FilePutSingleBuf.h for a description of this API call.*/

#include "dataObjOpen.h"
#include "dataObjPut.h"
#include "dataPut.h"
#include "filePut.h"
#include "getRemoteZoneResc.h"
#include "l3FilePutSingleBuf.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "rodsLog.h"
#include "specColl.hpp"
#include "subStructFilePut.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.hpp"
#include "rsFilePut.hpp"
#include "rsGlobalExtern.hpp"
#include "rsL3FilePutSingleBuf.hpp"
#include "rsSubStructFilePut.hpp"

#include "irods_resource_backport.hpp"

int
rsL3FilePutSingleBuf( rsComm_t *rsComm, int *l1descInx,
                      bytesBuf_t *dataObjInBBuf ) {
    int bytesWritten;

    if ( dataObjInBBuf->len >= 0 ) {
        if ( L1desc[*l1descInx].remoteZoneHost != NULL ) {
            bytesWritten = rcL3FilePutSingleBuf(
                               L1desc[*l1descInx].remoteZoneHost->conn,
                               L1desc[*l1descInx].remoteL1descInx, dataObjInBBuf );
        }
        else {
            bytesWritten = l3FilePutSingleBuf( rsComm, *l1descInx, dataObjInBBuf );
        }
    }
    else {
        bytesWritten = 0;
    }

    return bytesWritten;
}

int
l3FilePutSingleBuf( rsComm_t *rsComm, int l1descInx, bytesBuf_t *dataObjInpBBuf ) {
    dataObjInfo_t *dataObjInfo;
    fileOpenInp_t filePutInp;
    int bytesWritten;
    dataObjInp_t *dataObjInp;
    int retryCnt = 0;
    int chkType = 0; // JMC - backport 4774

    dataObjInfo = L1desc[l1descInx].dataObjInfo;
    dataObjInp = L1desc[l1descInx].dataObjInp;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3FilePutSingleBuf - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subFile_t subFile{};
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode( dataObjInp );
        subFile.flags = O_WRONLY | dataObjInp->openFlags;

        if ( ( L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY ) != 0 ) {
            subFile.flags |= FORCE_FLAG;
        }

        bytesWritten = rsSubStructFilePut( rsComm, &subFile, dataObjInpBBuf );
        return bytesWritten;
    } // struct file type >= 0

    memset( &filePutInp, 0, sizeof( filePutInp ) );
    rstrcpy( filePutInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( filePutInp.objPath, dataObjInp->objPath, MAX_NAME_LEN );
    if ( ( L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY ) != 0 ) {
        filePutInp.otherFlags |= FORCE_FLAG;
    }

    rstrcpy( filePutInp.addr.hostAddr, location.c_str(), NAME_LEN );
    rstrcpy( filePutInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    filePutInp.mode = getFileMode( dataObjInp );

    filePutInp.flags = O_WRONLY | dataObjInp->openFlags;
    rstrcpy( filePutInp.in_pdmo, L1desc[l1descInx].in_pdmo, MAX_NAME_LEN );
    // kv pasthru
    copyKeyVal(
        &dataObjInfo->condInput,
        &filePutInp.condInput );

    // =-=-=-=-=-=-=-
    // JMC - backport 4774
    chkType = getchkPathPerm( rsComm, L1desc[l1descInx].dataObjInp, L1desc[l1descInx].dataObjInfo );

    if ( chkType == DISALLOW_PATH_REG ) {
        clearKeyVal( &filePutInp.condInput );
        return PATH_REG_NOT_ALLOWED;
    }
    else if ( chkType == NO_CHK_PATH_PERM ) {
        // =-=-=-=-=-=-=-
        filePutInp.otherFlags |= NO_CHK_PERM_FLAG; // JMC - backport 4758
    }

    filePutOut_t* put_out = 0;
    bytesWritten = rsFilePut( rsComm, &filePutInp, dataObjInpBBuf, &put_out );

    // update the dataObjInfo with the potential changes made by the resource - hcj
    rstrcpy( dataObjInfo->rescHier, filePutInp.resc_hier_, MAX_NAME_LEN );
    if ( put_out ) {
        rstrcpy( dataObjInfo->filePath, put_out->file_name, MAX_NAME_LEN );
        free( put_out );
    }

    /* file already exists ? */
    while ( bytesWritten < 0 && retryCnt < 10 &&
            ( filePutInp.otherFlags & FORCE_FLAG ) == 0 &&
            getErrno( bytesWritten ) == EEXIST ) {

        if ( resolveDupFilePath( rsComm, dataObjInfo, dataObjInp ) < 0 ) {
            break;
        }
        rstrcpy( filePutInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );


        filePutOut_t* put_out = 0;
        bytesWritten = rsFilePut( rsComm, &filePutInp, dataObjInpBBuf, &put_out );
        // update the dataObjInfo with the potential changes made by the resource - hcj
        rstrcpy( dataObjInfo->rescHier, filePutInp.resc_hier_, MAX_NAME_LEN );
        if ( put_out ) {
            rstrcpy( dataObjInfo->filePath, put_out->file_name, MAX_NAME_LEN );
            free( put_out );
        }
        retryCnt ++;
    } // while
    clearKeyVal( &filePutInp.condInput );
    return bytesWritten;

} // l3FilePutSingleBuf
