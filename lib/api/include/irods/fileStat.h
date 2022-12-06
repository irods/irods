#ifndef IRODS_FILE_STAT_H
#define IRODS_FILE_STAT_H

#include "irods/rodsDef.h"
#include "irods/rodsType.h"

struct RcComm;
struct rodsStat;

typedef struct FileStatInp {
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
int rcFileStat(struct RcComm* conn, fileStatInp_t* fileStatInp, struct rodsStat** fileStatOut);

#endif // IRODS_FILE_STAT_H
