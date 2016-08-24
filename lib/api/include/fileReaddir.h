#ifndef FILE_READDIR_H__
#define FILE_READDIR_H__

#include "rodsType.h"
#include "rcConnect.h"

typedef struct {
    int fileInx;
} fileReaddirInp_t;
#define fileReaddirInp_PI "int fileInx;"


#ifdef __cplusplus
extern "C"
#endif
int rcFileReaddir( rcComm_t *conn, fileReaddirInp_t *fileReaddirInp, rodsDirent_t **fileReaddirOut );

#endif
