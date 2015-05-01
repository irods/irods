/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef MOD_AVU_METADATA_HPP
#define MOD_AVU_METADATA_HPP

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "icatDefines.hpp"

typedef struct {
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
} modAVUMetadataInp_t;

#define ModAVUMetadataInp_PI "str *arg0; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7;  str *arg8;  str *arg9;"

#if defined(RODS_SERVER)
#define RS_MOD_AVU_METADATA rsModAVUMetadata
/* prototype for the server handler */
int
rsModAVUMetadata( rsComm_t *rsComm, modAVUMetadataInp_t *modAVUMetadataInp );

int
_rsModAVUMetadata( rsComm_t *rsComm, modAVUMetadataInp_t *modAVUMetadataInp );
#else
#define RS_MOD_AVU_METADATA NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcModAVUMetadata( rcComm_t *conn, modAVUMetadataInp_t *modAVUMetadataInp );

void
clearModAVUMetadataInp( void * voidInp );

#ifdef __cplusplus
}
#endif
#endif	/* MOD_AVU_METADATA_H */
