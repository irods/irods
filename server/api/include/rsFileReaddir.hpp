#ifndef RS_FILE_READDIR_HPP
#define RS_FILE_READDIR_HPP

#include "rodsConnect.h"
#include "rodsType.h"

int rsFileReaddir( rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp, rodsDirent_t **fileReaddirOut );
int _rsFileReaddir( rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp, rodsDirent_t **fileReaddirOut );
int remoteFileReaddir( rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp, rodsDirent_t **fileReaddirOut, rodsServerHost_t *rodsServerHost );

#endif
