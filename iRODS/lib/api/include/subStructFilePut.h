/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFilePut.h  
 */

#ifndef SUB_STRUCT_FILE_PUT_H
#define SUB_STRUCT_FILE_PUT_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "structFileDriver.h"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_PUT rsSubStructFilePut
/* prototype for the server handler */
int
rsSubStructFilePut (rsComm_t *rsComm, subFile_t *subFile, 
bytesBuf_t *subFilePutOutBBuf);
int
_rsSubStructFilePut (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFilePutOutBBuf);
int
remoteSubStructFilePut (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFilePutOutBBuf, rodsServerHost_t *rodsServerHost);
#else
#define RS_SUB_STRUCT_FILE_PUT NULL
#endif

/* prototype for the client call */
int
rcSubStructFilePut (rcComm_t *conn, subFile_t *subFile, 
bytesBuf_t *subFilePutOutBBuf);

#endif	/* SUB_STRUCT_FILE_PUT_H */
