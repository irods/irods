#ifndef FILE_READ_H__
#define FILE_READ_H__

#include "rodsDef.h"
#include "rcConnect.h"

typedef struct FileReadInp {
    int fileInx;
    int len;
} fileReadInp_t;
#define fileReadInp_PI "int fileInx; int len;"


#ifdef __cplusplus
extern "C"
#endif
int rcFileRead( rcComm_t *conn, fileReadInp_t *fileReadInp, bytesBuf_t *fileReadOutBBuf );

#endif
