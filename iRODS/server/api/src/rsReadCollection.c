/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsReadCollection.c
 */

#include "openCollection.h"
#include "readCollection.h"
#include "objMetaOpr.h"
#include "rcGlobalExtern.h"
#include "rsGlobalExtern.h"

int
rsReadCollection (rsComm_t *rsComm, int *handleInxInp,
collEnt_t **collEnt)
{
    int status;
    collHandle_t *collHandle;

    int handleInx = *handleInxInp;

    if (handleInx < 0 || handleInx >= NUM_COLL_HANDLE ||
      CollHandle[handleInx].inuseFlag != FD_INUSE) {
        rodsLog (LOG_NOTICE,
          "rsReadCollection: handleInx %d out of range",
          handleInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }

    collHandle = &CollHandle[handleInx];
    *collEnt = (collEnt_t *) malloc (sizeof (collEnt_t));

    status = readCollection (collHandle, *collEnt);

    if (status < 0) {
	free (*collEnt);
	*collEnt = NULL;
    }

    return status;
}

