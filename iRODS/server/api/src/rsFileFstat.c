/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileFstat.h for a description of this API call.*/

#include "fileFstat.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"


int
rsFileFstat (rsComm_t *rsComm, fileFstatInp_t *fileFstatInp, 
rodsStat_t **fileFstatOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    *fileFstatOut = NULL;

    remoteFlag = getServerHostByFileInx (fileFstatInp->fileInx,
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileFstat (rsComm, fileFstatInp, fileFstatOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileFstat (rsComm, fileFstatInp, fileFstatOut,
	 rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileFstat: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileFstat (rsComm_t *rsComm, fileFstatInp_t *fileFstatInp,
rodsStat_t **fileFstatOut, rodsServerHost_t *rodsServerHost)
{   
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileFstat: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    fileFstatInp->fileInx = convL3descInx (fileFstatInp->fileInx);
    status = rcFileFstat (rodsServerHost->conn, fileFstatInp, fileFstatOut);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileFstat: rcFileFstat failed for %s",
          FileDesc[fileFstatInp->fileInx].fileName);
    }

    return status;
}

// =-=-=-=-=-=-=-
// 
int _rsFileFstat (rsComm_t *rsComm, fileFstatInp_t *fileFstatInp, rodsStat_t **fileFstatOut ) {
	// =-=-=-=-=-=-=-
	// make call to stat via resource plugin
    struct stat myFileStat;
    eirods::file_object file_obj( rsComm, 
                                  FileDesc[fileFstatInp->fileInx].fileName, 
	                              FileDesc[fileFstatInp->fileInx].fd,
								  0, 0 );
    eirods::error stat_err = fileFstat( file_obj, &myFileStat );

	// =-=-=-=-=-=-=-
	// log error if necessary
    if( !stat_err.ok() ) {
		std::stringstream msg;
		msg << "_rsFileFstat: fileFstat for ";
		msg << FileDesc[fileFstatInp->fileInx].fileName;
		msg << ", status = ";
		msg << stat_err.code();
		eirods::error ret_err = PASS( false, stat_err.code(), msg.str(), stat_err );
		eirods::log( ret_err );

		return ret_err.code();
    }

	// =-=-=-=-=-=-=-
	// convert unix stat struct to an irods stat struct
    *fileFstatOut = (rodsStat_t*)malloc (sizeof (rodsStat_t));
    int status = statToRodsStat(*fileFstatOut, &myFileStat);

	// =-=-=-=-=-=-=-
	// manage error if necessary
    if (status < 0) {
		free (*fileFstatOut);
		*fileFstatOut = NULL;
    }

    return status;

} // _rsFileFstat





