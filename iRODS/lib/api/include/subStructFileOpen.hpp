/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileOpen.h
 */

#ifndef SUB_STRUCT_FILE_OPEN_HPP
#define SUB_STRUCT_FILE_OPEN_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_OPEN rsSubStructFileOpen
/* prototype for the server handler */
int
rsSubStructFileOpen( rsComm_t *rsComm, subFile_t *subFile );
int
_rsSubStructFileOpen( rsComm_t *rsComm, subFile_t *subFile );
int
remoteSubStructFileOpen( rsComm_t *rsComm, subFile_t *subFile,
                         rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_OPEN NULL
#endif

/* prototype for the client call */
int
rcSubStructFileOpen( rcComm_t *conn, subFile_t *subFile );

#endif	/* SUB_STRUCT_FILE_OPEN_H */
