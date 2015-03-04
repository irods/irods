/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileTruncate.h
 */

#ifndef SUB_STRUCT_FILE_TRUNCATE_HPP
#define SUB_STRUCT_FILE_TRUNCATE_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "fileTruncate.hpp"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_TRUNCATE rsSubStructFileTruncate
/* prototype for the server handler */
int
rsSubStructFileTruncate( rsComm_t *rsComm, subFile_t *subStructFileTruncateInp );
int
_rsSubStructFileTruncate( rsComm_t *rsComm, subFile_t *subStructFileTruncateInp );
int
remoteSubStructFileTruncate( rsComm_t *rsComm, subFile_t *subStructFileTruncateInp,
                             rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_TRUNCATE NULL
#endif

/* prototype for the client call */
int
rcSubStructFileTruncate( rcComm_t *conn, subFile_t *subStructFileTruncateInp );

#endif	/* SUB_STRUCT_FILE_TRUNCATE_H */
