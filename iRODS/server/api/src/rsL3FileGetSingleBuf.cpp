/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See l3FileGetSingleBuf.h for a description of this API call.*/

#include "l3FileGetSingleBuf.h"
#include "rodsLog.h"
#include "dataGet.h"
#include "fileGet.h"
#include "dataObjOpen.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "getRemoteZoneResc.h"

int
rsL3FileGetSingleBuf (rsComm_t *rsComm, int *l1descInx,
bytesBuf_t *dataObjOutBBuf)
{
    int bytesRead;

    if (L1desc[*l1descInx].dataObjInfo->dataSize > 0) {
        if (L1desc[*l1descInx].remoteZoneHost != NULL) {
	    bytesRead = rcL3FileGetSingleBuf (
	      L1desc[*l1descInx].remoteZoneHost->conn,
	      L1desc[*l1descInx].remoteL1descInx, dataObjOutBBuf);
        } else {
	    bytesRead = l3FileGetSingleBuf (rsComm, *l1descInx, dataObjOutBBuf);
	}
    } else {
	bytesRead = 0;
	bzero (dataObjOutBBuf, sizeof (bytesBuf_t));
    }
    return (bytesRead);
}

