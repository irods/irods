#ifndef RS_FILE_CLOSEDIR_HPP
#define RS_FILE_CLOSEDIR_HPP

#include "irods/rodsConnect.h"
#include "irods/fileClosedir.h"

int rsFileClosedir( rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp );
int _rsFileClosedir( rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp );
int remoteFileClosedir( rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp, rodsServerHost_t *rodsServerHost );

#endif
