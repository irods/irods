#ifndef STRUCT_FILE_SYNC_H__
#define STRUCT_FILE_SYNC_H__

#include "rcConnect.h"
#include "rodsDef.h"
#include "objInfo.h"

typedef struct StructFileOprInp {
    rodsHostAddr_t addr;
    int oprType;  // see syncMountedColl.h
    int flags;
    specColl_t *specColl;
    keyValPair_t condInput;   // include chksum flag and value
} structFileOprInp_t;

#define StructFileOprInp_PI "struct RHostAddr_PI; int oprType; int flags; struct *SpecColl_PI; struct KeyValPair_PI;"

int rcStructFileSync( rcComm_t *conn, structFileOprInp_t *structFileOprInp );

#endif
