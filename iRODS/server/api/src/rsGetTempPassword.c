/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See getTempPassword.h for a description of this API call.*/

#include "getTempPassword.h"
#include "icatHighLevelRoutines.h"

int
rsGetTempPassword (rsComm_t *rsComm, 
		   getTempPasswordOut_t **getTempPasswordOut)
{
    rodsServerHost_t *rodsServerHost;
    int status;

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, NULL, &rodsServerHost);
    rodsLog (LOG_NOTICE,
		 "rsGetTempPassword get stat=%d", status);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsGetTempPassword (rsComm, getTempPasswordOut);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    } 
    else {
       status = rcGetTempPassword(rodsServerHost->conn,
				  getTempPasswordOut);
    }

    if (status < 0 ) {
        rodsLog (LOG_NOTICE,
		 "rsGetTempPassword: rcGetTempPassword failed, status = %d", 
		 status);
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsGetTempPassword (rsComm_t *rsComm, 
		    getTempPasswordOut_t **getTempPasswordOut)
{
    int status;
    getTempPasswordOut_t *myGetTempPasswordOut;

    myGetTempPasswordOut = (getTempPasswordOut_t*)malloc(sizeof(getTempPasswordOut_t));

    status = chlMakeTempPw(rsComm, 
			  myGetTempPasswordOut->stringToHashWith);
    if (status < 0 ) { 
	  rodsLog (LOG_NOTICE, 
		   "_rsGetTempPassword: getTempPassword, status = %d",
		   status);
       }

    *getTempPasswordOut = myGetTempPasswordOut;

    return (status);
} 
#endif
