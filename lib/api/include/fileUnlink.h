#ifndef FILE_UNLINK_H__
#define FILE_UNLINK_H__

#include "rodsDef.h"
#include "rcConnect.h"

typedef struct {
    rodsHostAddr_t addr;
    char fileName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    char in_pdmo[MAX_NAME_LEN];
} fileUnlinkInp_t;
#define fileUnlinkInp_PI "struct RHostAddr_PI; str fileName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; str in_pdmo[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileUnlink( rcComm_t *conn, fileUnlinkInp_t *fileUnlinkInp );

#endif
