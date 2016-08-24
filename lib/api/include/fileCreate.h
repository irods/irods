#ifndef FILE_CREATE_H__
#define FILE_CREATE_H__

#include "rcConnect.h"
#include "fileOpen.h"

typedef fileOpenInp_t fileCreateInp_t;

typedef struct {
    char file_name[ MAX_NAME_LEN ];
} fileCreateOut_t;

#define fileCreateOut_PI "str file_name[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileCreate( rcComm_t *conn, fileCreateInp_t *fileCreateInp, fileCreateOut_t** );

#endif
