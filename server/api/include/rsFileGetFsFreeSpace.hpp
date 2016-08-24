#ifndef RS_FILE_GET_FS_FREE_SPACE_HPP
#define RS_FILE_GET_FS_FREE_SPACE_HPP

#include "rodsConnect.h"
#include "fileGetFsFreeSpace.h"


int rsFileGetFsFreeSpace( rsComm_t *rsComm, fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp, fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut );
int _rsFileGetFsFreeSpace( rsComm_t *rsComm, fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp, fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut );
int remoteFileGetFsFreeSpace( rsComm_t *rsComm, fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp, fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut, rodsServerHost_t *rodsServerHost );

#endif
