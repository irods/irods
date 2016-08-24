#ifndef RS_SUB_STRUCT_FILE_CREATE_HPP
#define RS_SUB_STRUCT_FILE_CREATE_HPP

#include "rodsConnect.h"
#include "objInfo.h"

int rsSubStructFileCreate( rsComm_t *rsComm, subFile_t *subFile );
int _rsSubStructFileCreate( rsComm_t *rsComm, subFile_t *subFile );
int remoteSubStructFileCreate( rsComm_t *rsComm, subFile_t *subFile, rodsServerHost_t *rodsServerHost );

#endif
