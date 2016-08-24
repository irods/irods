#ifndef RS_SUB_STRUCT_FILE_UNLINK_HPP
#define RS_SUB_STRUCT_FILE_UNLINK_HPP

#include "rodsConnect.h"

int rsSubStructFileUnlink( rsComm_t *rsComm, subFile_t *subFile );
int _rsSubStructFileUnlink( rsComm_t *rsComm, subFile_t *subFile );
int remoteSubStructFileUnlink( rsComm_t *rsComm, subFile_t *subFile, rodsServerHost_t *rodsServerHost );

#endif
