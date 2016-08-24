#ifndef FILE_PUT_H__
#define FILE_PUT_H__

#include "rcConnect.h"
#include "rodsDef.h"
#include "fileOpen.h"

typedef struct {
    char file_name[ MAX_NAME_LEN ];
} filePutOut_t;
#define filePutOut_PI "str file_name[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFilePut( rcComm_t *conn, fileOpenInp_t *filePutInp, bytesBuf_t *filePutInpBBuf, filePutOut_t** );

#endif
