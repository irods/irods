/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileGet.h
 */

#ifndef SUB_STRUCT_FILE_GET_H__
#define SUB_STRUCT_FILE_GET_H__

/* This is Object File I/O type API call */

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsDef.h"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_GET rsSubStructFileGet
/* prototype for the server handler */
#include "rodsConnect.h"
int
rsSubStructFileGet( rsComm_t *rsComm, subFile_t *subFile,
                    bytesBuf_t *subFileGetOutBBuf );
int
_rsSubStructFileGet( rsComm_t *rsComm, subFile_t *subFile,
                     bytesBuf_t *subFileGetOutBBuf );
int
remoteSubStructFileGet( rsComm_t *rsComm, subFile_t *subFile,
                        bytesBuf_t *subFileGetOutBBuf, rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_GET NULL
#endif

/* prototype for the client call */
int
rcSubStructFileGet( rcComm_t *conn, subFile_t *subFile,
                    bytesBuf_t *subFileGetOutBBuf );

#endif	// SUB_STRUCT_FILE_GET_H__
