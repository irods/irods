#ifndef STRUCT_FILE_EXT_AND_REG_H__
#define STRUCT_FILE_EXT_AND_REG_H__

#include "rcConnect.h"
#include "objInfo.h"

typedef struct StructFileExtAndRegInp {
    char objPath[MAX_NAME_LEN];    // the obj path of the struct file
    char collection[MAX_NAME_LEN]; // the collection under which the extracted files are registered.
    int oprType;                   // see syncMountedColl.h. valid oprType are CREATE_TAR_OPR and ADD_TO_TAR_OPR
    int flags;                     // not used
    keyValPair_t condInput;        // include chksum flag and value
} structFileExtAndRegInp_t;
#define StructFileExtAndRegInp_PI "str objPath[MAX_NAME_LEN]; str collection[MAX_NAME_LEN]; int oprType; int flags; struct KeyValPair_PI;"


#ifdef __cplusplus
extern "C"
#endif
int rcStructFileExtAndReg( rcComm_t *conn, structFileExtAndRegInp_t *structFileExtAndRegInp );

#endif
