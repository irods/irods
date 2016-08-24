#ifndef RS_FILE_OPENDIR_HPP
#define RS_FILE_OPENDIR_HPP

#include "rodsConnect.h"
#include "fileOpendir.h"

int rsFileOpendir( rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp );
int _rsFileOpendir( rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp, void **dirPtr );
int remoteFileOpendir( rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp, rodsServerHost_t *rodsServerHost );

#endif
