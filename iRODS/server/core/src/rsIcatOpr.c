/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsIcatOpr.c - misc Icat operation
 */



#include "rsIcatOpr.h"
#include "initServer.h"
#include "rsGlobalExtern.h"
#include "readServerConfig.h"
#include "icatHighLevelRoutines.h"

#ifdef RODS_CAT
int
connectRcat (rsComm_t *rsComm) 
{
    int status = 0;
    rodsServerHost_t *tmpRodsServerHost;
    int gotRcatHost = 0;
    rodsServerConfig_t serverConfig;

    if (IcatConnState == INITIAL_DONE) {
	return (0);
    }

    /* zone has not been initialized yet. can't use getRcatHost */
    tmpRodsServerHost = ServerHostHead;

    while (tmpRodsServerHost != NULL) {
	if (tmpRodsServerHost->rcatEnabled == LOCAL_ICAT ||
	  tmpRodsServerHost->rcatEnabled == LOCAL_SLAVE_ICAT) {
	    if (tmpRodsServerHost->localFlag == LOCAL_HOST) {
	        memset(&serverConfig, 0, sizeof(serverConfig));
		status = readServerConfig(&serverConfig);
	        status = chlOpen (serverConfig.DBUsername,
				  serverConfig.DBPassword);
	        memset(&serverConfig, 0, sizeof(serverConfig));
		if (status < 0) {
        	    rodsLog (LOG_NOTICE,
         	    "connectRcat: chlOpen Error. Status = %d", status);
		} else {
		    IcatConnState = INITIAL_DONE;
		    gotRcatHost ++;
		}
    	    } else {
		gotRcatHost ++;
	    }
	}
        tmpRodsServerHost = tmpRodsServerHost->next;
    }

    if (gotRcatHost == 0) {
	if (status >= 0) {
	    status = SYS_NO_ICAT_SERVER_ERR;
	}
        rodsLog (LOG_SYS_FATAL,
          "initServerInfo: no rcatHost error, status = %d",
          status);
    } else {
	status = 0;
    }

    return (status);
}

int
disconnectRcat (rsComm_t *rsComm)
{
    int status; 

    if (IcatConnState == INITIAL_DONE) {
        if ((status = chlClose ()) != 0) {
            rodsLog (LOG_NOTICE,
             "initInfoWithRcat: chlClose Error. Status = %d",
             status);
	}
	IcatConnState = INITIAL_NOT_DONE;
    } else {
	status = 0;
    }
    return (status);
}

int
resetRcat (rsComm_t *rsComm)
{
    IcatConnState = INITIAL_NOT_DONE;
    return 0;
}

#endif
