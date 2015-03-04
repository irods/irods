/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileWrite.h
 */

#ifndef SUB_STRUCT_FILE_WRITE_HPP
#define SUB_STRUCT_FILE_WRITE_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "subStructFileRead.hpp"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_WRITE rsSubStructFileWrite
/* prototype for the server handler */
int
rsSubStructFileWrite( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
                      bytesBuf_t *subStructFileWriteOutBBuf );
int
_rsSubStructFileWrite( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
                       bytesBuf_t *subStructFileWriteOutBBuf );
int
remoteSubStructFileWrite( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
                          bytesBuf_t *subStructFileWriteOutBBuf, rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_WRITE NULL
#endif

/* prototype for the client call */
int
rcSubStructFileWrite( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileWriteInp,
                      bytesBuf_t *subStructFileWriteOutBBuf );

#endif	/* SUB_STRUCT_FILE_WRITE_H */
