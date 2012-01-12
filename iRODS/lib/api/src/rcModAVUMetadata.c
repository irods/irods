/* This is script-generated code.  */ 
/* See modAVUMetadata.h for a description of this API call.*/

#include "modAVUMetadata.h"

int
rcModAVUMetadata (rcComm_t *conn, modAVUMetadataInp_t *modAVUMetadataInp)
{
    int status;
    status = procApiRequest (conn, MOD_AVU_METADATA_AN, modAVUMetadataInp,
			     NULL, (void **) NULL, NULL);

    return (status);
}
