/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See getLimitedPassword.h for a description of this API call.*/

#include "getLimitedPassword.h"
#include "icatHighLevelRoutines.h"

int
rsGetLimitedPassword (rsComm_t *rsComm, 
		      getLimitedPasswordInp_t *getLimitedPasswordInp,
		      getLimitedPasswordOut_t **getLimitedPasswordOut)
{
    rodsServerHost_t *rodsServerHost;
    int status;

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, NULL, &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsGetLimitedPassword (rsComm, getLimitedPasswordInp,
				       getLimitedPasswordOut);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    } 
    else {
       status = rcGetLimitedPassword(rodsServerHost->conn,
				     getLimitedPasswordInp,
				     getLimitedPasswordOut);
    }

    if (status < 0 ) {
        rodsLog (LOG_NOTICE,
		 "rsGetLimitedPassword: rcGetLimitedPassword failed, status = %d", 
		 status);
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsGetLimitedPassword (rsComm_t *rsComm, 
		       getLimitedPasswordInp_t *getLimitedPasswordInp,
		       getLimitedPasswordOut_t **getLimitedPasswordOut)
{
    int status;
    getLimitedPasswordOut_t *myGetLimitedPasswordOut;

    myGetLimitedPasswordOut = (getLimitedPasswordOut_t*)malloc(sizeof(getLimitedPasswordOut_t));

    status = chlMakeLimitedPw(rsComm, getLimitedPasswordInp->ttl,
			   myGetLimitedPasswordOut->stringToHashWith);
    if (status < 0 ) { 
	  rodsLog (LOG_NOTICE, 
		   "_rsGetLimitedPassword: getLimitedPassword, status = %d",
		   status);
       }

    *getLimitedPasswordOut = myGetLimitedPasswordOut;

    return (status);
} 
#endif
