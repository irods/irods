#ifndef RS_SUB_STRUCT_FILE_CLOSEDIR_HPP
#define RS_SUB_STRUCT_FILE_CLOSEDIR_HPP

#include "rodsConnect.h"
#include "subStructFileRead.h"

int rsSubStructFileClosedir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileClosedirInp );
int _rsSubStructFileClosedir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileClosedirInp );
int remoteSubStructFileClosedir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileClosedirInp, rodsServerHost_t *rodsServerHost );

#endif
