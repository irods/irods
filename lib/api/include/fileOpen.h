#ifndef FILE_OPEN_H__
#define FILE_OPEN_H__

#include "rodsType.h"
#include "rodsDef.h"
#include "objInfo.h"
#include "rcConnect.h"

// definition for otherFlags
#define NO_CHK_PERM_FLAG        0x1
#define UNIQUE_REM_COMM_FLAG    0x2
#define FORCE_FLAG              0x4

typedef struct {
    char resc_name_[MAX_NAME_LEN];
    char resc_hier_[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    int otherFlags;     // for chkPerm, uniqueRemoteConn
    rodsHostAddr_t addr;
    char fileName[MAX_NAME_LEN];
    int flags;
    int mode;
    rodsLong_t dataSize;
    keyValPair_t condInput;
    char in_pdmo[MAX_NAME_LEN];
} fileOpenInp_t;
#define fileOpenInp_PI "str resc_name_[MAX_NAME_LEN]; str resc_hier_[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; int otherFlags; struct RHostAddr_PI; str fileName[MAX_NAME_LEN]; int flags; int mode; double dataSize; struct KeyValPair_PI; str in_pdmo[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileOpen( rcComm_t *conn, fileOpenInp_t *fileOpenInp );

#endif
