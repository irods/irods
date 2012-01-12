/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileTruncate.h  
 */

#ifndef SUB_STRUCT_FILE_TRUNCATE_H
#define SUB_STRUCT_FILE_TRUNCATE_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "structFileDriver.h"
#include "fileTruncate.h"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_TRUNCATE rsSubStructFileTruncate
/* prototype for the server handler */
int
rsSubStructFileTruncate (rsComm_t *rsComm, subFile_t *subStructFileTruncateInp);
int
_rsSubStructFileTruncate (rsComm_t *rsComm, subFile_t *subStructFileTruncateInp);
int
remoteSubStructFileTruncate (rsComm_t *rsComm, subFile_t *subStructFileTruncateInp, 
rodsServerHost_t *rodsServerHost);
#else
#define RS_SUB_STRUCT_FILE_TRUNCATE NULL
#endif

/* prototype for the client call */
int
rcSubStructFileTruncate (rcComm_t *conn, subFile_t *subStructFileTruncateInp);

#endif	/* SUB_STRUCT_FILE_TRUNCATE_H */
