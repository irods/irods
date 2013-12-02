/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjCreate.h for a description of this API call.*/

#include "dataObjCreate.h"
#include "dataObjCreateAndStat.h"
#include "rodsLog.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"

int
rsDataObjCreateAndStat (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
openStat_t **openStat)
{
    int status;

    status = rsDataObjCreate (rsComm, dataObjInp);

    if (status >= 0) {
	*openStat = (openStat_t*)malloc (sizeof (openStat_t));
	(*openStat)->dataSize = L1desc[status].dataObjInfo->dataSize;
	rstrcpy ((*openStat)->dataMode, L1desc[status].dataObjInfo->dataMode,
	  SHORT_STR_LEN);
	rstrcpy ((*openStat)->dataType, L1desc[status].dataObjInfo->dataType,
	  NAME_LEN);
	(*openStat)->l3descInx = L1desc[status].l3descInx;
	(*openStat)->replStatus = L1desc[status].replStatus;
        (*openStat)->replNum = L1desc[status].dataObjInfo->replNum;
    } else {
	*openStat = NULL;
    }

    return (status);
}

