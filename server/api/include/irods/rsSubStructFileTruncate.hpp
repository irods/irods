#ifndef RS_SUB_STRUCT_FILE_TRUNCATE_HPP
#define RS_SUB_STRUCT_FILE_TRUNCATE_HPP


#include "irods/rodsConnect.h"
#include "irods/objInfo.h"

int rsSubStructFileTruncate(rsComm_t *rsComm, subFile_t *subStructFileTruncateInp );
int _rsSubStructFileTruncate(rsComm_t *rsComm, subFile_t *subStructFileTruncateInp );
int remoteSubStructFileTruncate(rsComm_t *rsComm, subFile_t *subStructFileTruncateInp, rodsServerHost_t *rodsServerHost );

#endif
