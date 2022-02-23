#ifndef RS_FILE_UNLINK_HPP
#define RS_FILE_UNLINK_HPP

#include "irods/rcConnect.h"
#include "irods/rodsConnect.h"
#include "irods/fileUnlink.h"

int rsFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp );
int _rsFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp );
int remoteFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp, rodsServerHost_t *rodsServerHost );

#endif
