#ifndef RS_SUB_STRUCT_FILE_RMDIR_HPP
#define RS_SUB_STRUCT_FILE_RMDIR_HPP

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/rodsConnect.h"

int rsSubStructFileRmdir( rsComm_t *rsComm, subFile_t *subFile );
int _rsSubStructFileRmdir( rsComm_t *rsComm, subFile_t *subFile );
int remoteSubStructFileRmdir( rsComm_t *rsComm, subFile_t *subFile, rodsServerHost_t *rodsServerHost );

#endif
