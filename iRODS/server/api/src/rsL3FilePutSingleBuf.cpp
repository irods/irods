/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See l3FilePutSingleBuf.h for a description of this API call.*/

#include "l3FilePutSingleBuf.hpp"
#include "rodsLog.h"
#include "dataPut.hpp"
#include "filePut.hpp"
#include "dataObjOpen.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "rsApiHandler.hpp"
#include "objMetaOpr.hpp"
#include "getRemoteZoneResc.hpp"

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

