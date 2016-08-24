#ifndef FILE_CLOSE_H__
#define FILE_CLOSE_H__

#include "rcConnect.h"

typedef struct FileCloseInp {
    int fileInx;
    char in_pdmo[MAX_NAME_LEN];
} fileCloseInp_t;

#define fileCloseInp_PI "int fileInx; str in_pdmo[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileClose( rcComm_t *conn, fileCloseInp_t *fileCloseInp );

#endif
