#ifndef FILE_STAT_H__
#define FILE_STAT_H__

#include "rodsDef.h"
#include "rcConnect.h"

typedef struct {
    rodsHostAddr_t addr;
    char fileName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    rodsLong_t rescId;
} fileStatInp_t;
#define fileStatInp_PI "struct RHostAddr_PI; str fileName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; double rescId;"

#ifdef __cplusplus
extern "C"
#endif
int rcFileStat( rcComm_t *conn, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut );

#endif
