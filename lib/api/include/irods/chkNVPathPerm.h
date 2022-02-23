#ifndef CHK_NV_PATH_PERM_H__
#define CHK_NV_PATH_PERM_H__

#include "irods/fileOpen.h"
#include "irods/rodsConnect.h"
#include "irods/rcConnect.h"

#ifdef __cplusplus
extern "C"
#endif
int rcChkNVPathPerm( rcComm_t *conn, fileOpenInp_t *chkNVPathPermInp );

#endif
