#ifndef RS_FILE_RMDIR_HPP
#define RS_FILE_RMDIR_HPP

#include "rodsConnect.h"
#include "fileRmdir.h"

int rsFileRmdir( rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp );
int _rsFileRmdir( rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp );
int remoteFileRmdir( rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp, rodsServerHost_t *rodsServerHost );

#endif
