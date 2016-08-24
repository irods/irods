#ifndef RS_SUB_STRUCT_FILE_OPEN_HPP
#define RS_SUB_STRUCT_FILE_OPEN_HPP

#include "rodsConnect.h"
#include "objInfo.h"

int rsSubStructFileOpen( rsComm_t *rsComm, subFile_t *subFile );
int _rsSubStructFileOpen( rsComm_t *rsComm, subFile_t *subFile );
int remoteSubStructFileOpen( rsComm_t *rsComm, subFile_t *subFile, rodsServerHost_t *rodsServerHost );

#endif
