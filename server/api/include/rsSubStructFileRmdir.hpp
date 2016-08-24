#ifndef RS_SUB_STRUCT_FILE_RMDIR_HPP
#define RS_SUB_STRUCT_FILE_RMDIR_HPP

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsConnect.h"

int rsSubStructFileRmdir( rsComm_t *rsComm, subFile_t *subFile );
int _rsSubStructFileRmdir( rsComm_t *rsComm, subFile_t *subFile );
int remoteSubStructFileRmdir( rsComm_t *rsComm, subFile_t *subFile, rodsServerHost_t *rodsServerHost );

#endif
