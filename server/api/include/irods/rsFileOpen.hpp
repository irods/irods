#ifndef RS_FILE_OPEN_HPP
#define RS_FILE_OPEN_HPP

#include "irods/rodsType.h"
#include "irods/rodsDef.h"
#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/fileOpen.h"

int rsFileOpen( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp );
int rsFileOpenByHost( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp, rodsServerHost_t *rodsServerHost );
int _rsFileOpen( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp );
int remoteFileOpen( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp, rodsServerHost_t *rodsServerHost );

#endif
