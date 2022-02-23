#ifndef RS_FILE_TRUNCATE_HPP
#define RS_FILE_TRUNCATE_HPP

#include "irods/rodsConnect.h"
#include "irods/fileOpen.h"


int rsFileTruncate( rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp );
int _rsFileTruncate( rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp );
int remoteFileTruncate( rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp, rodsServerHost_t *rodsServerHost );

#endif
