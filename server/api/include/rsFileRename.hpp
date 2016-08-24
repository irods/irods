#ifndef RS_FILE_RENAME_HPP
#define RS_FILE_RENAME_HPP

#include "rodsConnect.h"
#include "fileRename.h"

int rsFileRename( rsComm_t *rsComm, fileRenameInp_t *fileRenameInp, fileRenameOut_t** );
int _rsFileRename( rsComm_t *rsComm, fileRenameInp_t *fileRenameInp, fileRenameOut_t** );
int remoteFileRename( rsComm_t *rsComm, fileRenameInp_t *fileRenameInp, fileRenameOut_t**, rodsServerHost_t *rodsServerHost );

#endif
