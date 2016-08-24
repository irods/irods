#ifndef RS_FILE_CHMOD_HPP
#define RS_FILE_CHMOD_HPP

#include "rodsConnect.h"
#include "fileChmod.h"

int rsFileChmod( rsComm_t *rsComm, fileChmodInp_t *fileChmodInp );
int _rsFileChmod( rsComm_t *rsComm, fileChmodInp_t *fileChmodInp );
int remoteFileChmod( rsComm_t *rsComm, fileChmodInp_t *fileChmodInp, rodsServerHost_t *rodsServerHost );

#endif
