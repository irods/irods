#ifndef RS_SUB_STRUCT_FILE_OPENDIR_HPP
#define RS_SUB_STRUCT_FILE_OPENDIR_HPP

#include "irods/rodsConnect.h"
#include "irods/objInfo.h"

int rsSubStructFileOpendir( rsComm_t *rsComm, subFile_t *subFile );
int _rsSubStructFileOpendir( rsComm_t *rsComm, subFile_t *subFile );
int remoteSubStructFileOpendir( rsComm_t *rsComm, subFile_t *subFile, rodsServerHost_t *rodsServerHost );

#endif
