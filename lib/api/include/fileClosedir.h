#ifndef FILE_CLOSEDIR_H__
#define FILE_CLOSEDIR_H__

#include "rcConnect.h"

typedef struct {
    int fileInx;
} fileClosedirInp_t;
#define fileClosedirInp_PI "int fileInx;"

#ifdef __cplusplus
extern "C"
#endif
int rcFileClosedir( rcComm_t *conn, fileClosedirInp_t *fileClosedirInp );

#endif
