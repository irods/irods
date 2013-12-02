/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See getTempPasswordForOther.h for a description of this API call.*/

#include "getTempPasswordForOther.h"
#include "icatHighLevelRoutines.h"

int
rsGetTempPasswordForOther (rsComm_t *rsComm, 
		   getTempPasswordForOtherInp_t *getTempPasswordForOtherInp,
		   getTempPasswordForOtherOut_t **getTempPasswordForOtherOut)
{
    rodsServerHost_t *rodsServerHost;
    int status;

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, NULL, &rodsServerHost);
    rodsLog (LOG_DEBUG,
		 "rsGetTempPasswordForOther get stat=%d", status);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsGetTempPasswordForOther (rsComm, getTempPasswordForOtherInp,
					    getTempPasswordForOtherOut);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    } 
    else {
       status = rcGetTempPasswordForOther(rodsServerHost->conn,
				  getTempPasswordForOtherInp,
				  getTempPasswordForOtherOut);
    }
    if (status < 0 ) {
        rodsLog (LOG_NOTICE,
		 "rsGetTempPasswordForOther: rcGetTempPasswordForOther failed, status = %d", 
		 status);
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsGetTempPasswordForOther (rsComm_t *rsComm, 
		    getTempPasswordForOtherInp_t *getTempPasswordForOtherInp,
		    getTempPasswordForOtherOut_t **getTempPasswordForOtherOut)
{
    int status;
    getTempPasswordForOtherOut_t *myGetTempPasswordForOtherOut;

    myGetTempPasswordForOtherOut = (getTempPasswordForOtherOut_t*)malloc(sizeof(getTempPasswordForOtherOut_t));

    status = chlMakeTempPw(rsComm, 
			   myGetTempPasswordForOtherOut->stringToHashWith,
			   getTempPasswordForOtherInp->otherUser);
    if (status < 0 ) { 
	  rodsLog (LOG_NOTICE, 
		   "_rsGetTempPasswordForOther: getTempPasswordForOther, status = %d",
		   status);
       }

    *getTempPasswordForOtherOut = myGetTempPasswordForOtherOut;

    return (status);
} 
#endif
