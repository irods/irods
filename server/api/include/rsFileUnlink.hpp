#ifndef RS_FILE_UNLINK_HPP
#define RS_FILE_UNLINK_HPP

#include "rcConnect.h"
#include "rodsConnect.h"
#include "fileUnlink.h"

int rsFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp );
int _rsFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp );
int remoteFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp, rodsServerHost_t *rodsServerHost );

#endif
