#ifndef RS_CHK_N_V_PATH_PERM_HPP
#define RS_CHK_N_V_PATH_PERM_HPP

#include "irods/fileOpen.h"
#include "irods/rodsConnect.h"
#include "irods/rcConnect.h"

int rsChkNVPathPerm( rsComm_t *rsComm, fileOpenInp_t *chkNVPathPermInp );
int rsChkNVPathPermByHost( rsComm_t *rsComm, fileOpenInp_t *chkNVPathPermInp, rodsServerHost_t *rodsServerHost );
int _rsChkNVPathPerm( rsComm_t *rsComm, fileOpenInp_t *chkNVPathPermInp );
int remoteChkNVPathPerm( rsComm_t *rsComm, fileOpenInp_t *chkNVPathPermInp, rodsServerHost_t *rodsServerHost );

#endif
