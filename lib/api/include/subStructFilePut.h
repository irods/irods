/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFilePut.h
 */

#ifndef SUB_STRUCT_FILE_PUT_H__
#define SUB_STRUCT_FILE_PUT_H__

/* This is Object File I/O type API call */

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsDef.h"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_PUT rsSubStructFilePut
/* prototype for the server handler */
#include "rodsConnect.h"
int
rsSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile,
                    bytesBuf_t *subFilePutOutBBuf );
int
_rsSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile,
                     bytesBuf_t *subFilePutOutBBuf );
int
remoteSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile,
                        bytesBuf_t *subFilePutOutBBuf, rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_PUT NULL
#endif

/* prototype for the client call */
int
rcSubStructFilePut( rcComm_t *conn, subFile_t *subFile,
                    bytesBuf_t *subFilePutOutBBuf );

#endif	// SUB_STRUCT_FILE_PUT_H__
