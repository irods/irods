#ifndef IRODS_FILE_READDIR_H
#define IRODS_FILE_READDIR_H

#include "irods/rodsType.h"
#include "irods/rcConnect.h"

typedef struct FileReadDirInp
{
    int fileInx;
} fileReaddirInp_t;
#define fileReaddirInp_PI "int fileInx;"

#ifdef __cplusplus
extern "C"
#endif
int rcFileReaddir( rcComm_t *conn, fileReaddirInp_t *fileReaddirInp, rodsDirent_t **fileReaddirOut );

#endif // IRODS_FILE_READDIR_H
