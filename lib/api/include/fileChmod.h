#ifndef FILE_CHMOD_H__
#define FILE_CHMOD_H__

#include "rodsDef.h"
#include "rcConnect.h"

typedef struct {
    rodsHostAddr_t addr;
    char fileName[MAX_NAME_LEN];
    int mode;
    char rescHier[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
} fileChmodInp_t;

#define fileChmodInp_PI "struct RHostAddr_PI; str fileName[MAX_NAME_LEN]; int mode; str rescHier[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileChmod( rcComm_t *conn, fileChmodInp_t *fileChmodInp );

#endif
