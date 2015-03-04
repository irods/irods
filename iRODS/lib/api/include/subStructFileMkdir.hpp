/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileMkdir.h
 */

#ifndef SUB_STRUCT_FILE_MKDIR_HPP
#define SUB_STRUCT_FILE_MKDIR_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_MKDIR rsSubStructFileMkdir
/* prototype for the server handler */
int
rsSubStructFileMkdir( rsComm_t *rsComm, subFile_t *subFile );
int
_rsSubStructFileMkdir( rsComm_t *rsComm, subFile_t *subFile );
int
remoteSubStructFileMkdir( rsComm_t *rsComm, subFile_t *subFile,
                          rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_MKDIR NULL
#endif

/* prototype for the client call */
int
rcSubStructFileMkdir( rcComm_t *conn, subFile_t *subFile );

#endif	/* SUB_STRUCT_FILE_MKDIR_H */
