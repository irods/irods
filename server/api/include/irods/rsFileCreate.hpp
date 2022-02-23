#ifndef RS_FILE_CREATE_HPP
#define RS_FILE_CREATE_HPP

#include "irods/rodsConnect.h"
#include "irods/fileOpen.h"
#include "irods/fileCreate.h"

int rsFileCreate( rsComm_t *rsComm, fileCreateInp_t *fileCreateInp, fileCreateOut_t** );
int _rsFileCreate( rsComm_t *rsComm, fileCreateInp_t *fileCreateInp, rodsServerHost_t *rodsServerHost, fileCreateOut_t** );
int remoteFileCreate( rsComm_t *rsComm, fileCreateInp_t *fileCreateInp, rodsServerHost_t *rodsServerHost, fileCreateOut_t** );
#endif
