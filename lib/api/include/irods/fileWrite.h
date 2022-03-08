#ifndef FILE_WRITE_H__
#define FILE_WRITE_H__

#include "irods/rcConnect.h"
#include "irods/rodsDef.h"

typedef struct {
    int fileInx;
    int len;
} fileWriteInp_t;

#define fileWriteInp_PI "int fileInx; int len;"

#ifdef __cplusplus
extern "C"
#endif
int rcFileWrite( rcComm_t *conn, const fileWriteInp_t *fileWriteInp, const bytesBuf_t *fileWriteInpBBuf );

#endif
