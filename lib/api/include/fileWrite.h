#ifndef FILE_WRITE_H__
#define FILE_WRITE_H__

#include "rcConnect.h"
#include "rodsDef.h"

typedef struct {
    int fileInx;
    int len;
} fileWriteInp_t;

#define fileWriteInp_PI "int fileInx; int len;"

#ifdef __cplusplus
extern "C"
#endif
int rcFileWrite( rcComm_t *conn, fileWriteInp_t *fileWriteInp, bytesBuf_t *fileWriteInpBBuf );

#endif
