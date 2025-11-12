#ifndef CHK_NV_PATH_PERM_H__
#define CHK_NV_PATH_PERM_H__

// Remove in iRODS 6.0
#if __has_include("irods/rodsConnect.h")
#  include "irods/rodsConnect.h"
#endif

#include "irods/fileOpen.h"
#include "irods/rcConnect.h"

#ifdef __cplusplus
extern "C"
#endif
int rcChkNVPathPerm( rcComm_t *conn, fileOpenInp_t *chkNVPathPermInp );

#endif
