/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See generalUpdate.h for a description of this API call.*/

#include "generalUpdate.h"
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

int
rsGeneralUpdate (rsComm_t *rsComm, generalUpdateInp_t *generalUpdateInp )
{
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog(LOG_DEBUG, "generalUpdate");

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, NULL, &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsGeneralUpdate (rsComm, generalUpdateInp);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
       status = rcGeneralUpdate(rodsServerHost->conn,
			       generalUpdateInp);
    }

    if (status < 0) { 
       rodsLog (LOG_NOTICE,
		"rsGeneralUpdate: rcGeneralUpdate failed");
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsGeneralUpdate(rsComm_t *rsComm, generalUpdateInp_t *generalUpdateInp )
{
    int status;

    status  = chlGeneralUpdate(*generalUpdateInp);

    return(status);
} 
#endif
