#ifndef IRODS_FILE_CREATE_H
#define IRODS_FILE_CREATE_H

#include "irods/rcConnect.h"
#include "irods/fileOpen.h"

typedef fileOpenInp_t fileCreateInp_t;

typedef struct FileCreateOut
{
    char file_name[MAX_NAME_LEN];
} fileCreateOut_t;

#define fileCreateOut_PI "str file_name[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileCreate( rcComm_t *conn, fileCreateInp_t *fileCreateInp, fileCreateOut_t** );

#endif // IRODS_FILE_CREATE_H
