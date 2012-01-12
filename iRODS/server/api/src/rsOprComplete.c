/* rsOprComplete.c */
#include "oprComplete.h"
#include "dataObjClose.h"
#include "rsGlobalExtern.h"

int rsOprComplete (rsComm_t *rsComm, int *retval)
{
    openedDataObjInp_t dataObjCloseInp;

    if (*retval >= 2) {
	int l1descInx = *retval;

        if (L1desc[l1descInx].remoteZoneHost != NULL) {
            *retval = rcOprComplete (L1desc[l1descInx].remoteZoneHost->conn,
	      L1desc[l1descInx].remoteL1descInx);
	    freeL1desc (l1descInx);
	} else {
            memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
            dataObjCloseInp.l1descInx = l1descInx;
	    if (L1desc[*retval].oprType == PUT_OPR) {
	        dataObjCloseInp.bytesWritten = L1desc[*retval].dataSize;
	    }
            *retval = rsDataObjClose (rsComm, &dataObjCloseInp);
	}
    }

    if (*retval >= 0) {
	return (SYS_HANDLER_DONE_NO_ERROR);
    } else {
        return (*retval);
    }
}

