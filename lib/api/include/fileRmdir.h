#ifndef FILE_RMDIR_H__
#define FILE_RMDIR_H__

#include "rodsDef.h"
#include "rcConnect.h"

// definition for flags of fileRmdirInp_t
#define RMDIR_RECUR     0x1

typedef struct {
    int flags;
    rodsHostAddr_t addr;
    char dirName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
} fileRmdirInp_t;

#define fileRmdirInp_PI "int flag; struct RHostAddr_PI; str dirName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileRmdir( rcComm_t *conn, fileRmdirInp_t *fileRmdirInp );

#endif
