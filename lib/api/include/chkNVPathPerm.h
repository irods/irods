#ifndef CHK_NV_PATH_PERM_H__
#define CHK_NV_PATH_PERM_H__

#include "fileOpen.h"
#include "rodsConnect.h"
#include "rcConnect.h"

#ifdef __cplusplus
extern "C"
#endif
int rcChkNVPathPerm( rcComm_t *conn, fileOpenInp_t *chkNVPathPermInp );

#endif
