/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* modMetadata.h
 */

#ifndef MOD_AVU_METADATA_HPP
#define MOD_AVU_METADATA_HPP

/* This is a metadata type API call */

/*
   This call performs various operations on the Attribute-Value-Units
   (AVU) triplets type of metadata.  The Units are optional, so these
   are frequently Attribute-Value pairs.  ATUs are user-defined
   metadata items.  The imeta command makes extensive use of this and
   the genQuery call.
*/

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

typedef struct {
    char *arg0;
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    char *arg5;
    char *arg6;
    char *arg7;
    char *arg8;
    char *arg9;
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
