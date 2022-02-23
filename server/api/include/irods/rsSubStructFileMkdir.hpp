#ifndef RS_SUB_STRUCT_FILE_MKDIR_HPP
#define RS_SUB_STRUCT_FILE_MKDIR_HPP

#include "irods/rodsConnect.h"
#include "irods/objInfo.h"

int rsSubStructFileMkdir( rsComm_t *rsComm, subFile_t *subFile );
int _rsSubStructFileMkdir( rsComm_t *rsComm, subFile_t *subFile );
int remoteSubStructFileMkdir( rsComm_t *rsComm, subFile_t *subFile, rodsServerHost_t *rodsServerHost );

#endif
