#ifndef IRODS_MOD_AVU_METADATA_H
#define IRODS_MOD_AVU_METADATA_H

/// \file

#include "objInfo.h"

struct RcComm;

typedef struct ModifyAVUMetadataInput {
    char *arg0; // option add, adda, rm, rmw, rmi, cp, mod, or set
    char *arg1; // item type -d,-D,-c,-C,-r,-R,-u,-U
    char *arg2; // item name
    char *arg3; // attr name
    char *arg4; // attr value
    char *arg5; // attr unit
    char *arg6; // new attr name (for mod or set)
    char *arg7; // new attr value (for mod or set)
    char *arg8; // new attr unit (for mod or set)
    char *arg9; // unused
    struct KeyValPair condInput;
} modAVUMetadataInp_t;

#define ModAVUMetadataInp_PI "str *arg0; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7;  str *arg8;  str *arg9; struct KeyValPair_PI;"

#ifdef __cplusplus
extern "C" {
#endif

int rcModAVUMetadata(struct RcComm* comm, struct ModifyAVUMetadataInput* modAVUMetadataInp);

void clearModAVUMetadataInp(void* voidInp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_MOD_AVU_METADATA_H
