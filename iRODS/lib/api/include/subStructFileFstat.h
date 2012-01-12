/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileFstat.h  
 */

#ifndef SUB_STRUCT_FILE_FSTAT_H
#define SUB_STRUCT_FILE_FSTAT_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "structFileDriver.h"
#include "subStructFileRead.h"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_FSTAT rsSubStructFileFstat
/* prototype for the server handler */
int
rsSubStructFileFstat (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut);
int
_rsSubStructFileFstat (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut);
int
remoteSubStructFileFstat (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut, rodsServerHost_t *rodsServerHost);
#else
#define RS_SUB_STRUCT_FILE_FSTAT NULL
#endif

/* prototype for the client call */
int
rcSubStructFileFstat (rcComm_t *conn, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut);

#endif	/* SUB_STRUCT_FILE_FSTAT_H */
