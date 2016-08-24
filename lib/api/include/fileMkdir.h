#ifndef FILE_MKDIR_H__
#define FILE_MKDIR_H__

#include "rodsDef.h"
#include "rcConnect.h"

typedef struct {
    rodsHostAddr_t addr;
    char dirName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    int mode;
    keyValPair_t condInput;
} fileMkdirInp_t;
#define fileMkdirInp_PI "struct RHostAddr_PI; str dirName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; int mode; struct KeyValPair_PI;"

#ifdef __cplusplus
extern "C"
#endif
int rcFileMkdir( rcComm_t *conn, fileMkdirInp_t *fileMkdirInp );

#endif
