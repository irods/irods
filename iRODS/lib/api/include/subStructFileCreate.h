/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileCreate.h  
 */

#ifndef SUB_STRUCT_FILE_CREATE_H
#define SUB_STRUCT_FILE_CREATE_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "structFileDriver.h"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_CREATE rsSubStructFileCreate
/* prototype for the server handler */
int
rsSubStructFileCreate (rsComm_t *rsComm, subFile_t *subFile);
int
_rsSubStructFileCreate (rsComm_t *rsComm, subFile_t *subFile);
int
remoteSubStructFileCreate (rsComm_t *rsComm, subFile_t *subFile,
rodsServerHost_t *rodsServerHost);
#else
#define RS_SUB_STRUCT_FILE_CREATE NULL
#endif

/* prototype for the client call */
int
rcSubStructFileCreate (rcComm_t *conn, subFile_t *subFile);

#endif	/* SUB_STRUCT_FILE_CREATE_H */
