/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileClose.h
 */

#ifndef SUB_STRUCT_FILE_CLOSE_HPP
#define SUB_STRUCT_FILE_CLOSE_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "subStructFileRead.hpp"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_CLOSE rsSubStructFileClose
/* prototype for the server handler */
int
rsSubStructFileClose( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp );

int
_rsSubStructFileClose( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp );
int
remoteSubStructFileClose( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp,
                          rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_CLOSE NULL
#endif

/* prototype for the client call */
int
rcSubStructFileClose( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileCloseInp );

#endif	/* SUB_STRUCT_FILE_CLOSE_H */
