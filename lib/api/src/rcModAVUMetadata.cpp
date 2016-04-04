
#include "modAVUMetadata.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcModAVUMetadata( rcComm_t *conn, modAVUMetadataInp_t *modAVUMetadataInp )
 *
 * \brief Modifies the Attribute-Value-Units of various iRODS entities.
 *
 * \user client and server
 *
 * \ingroup metadata
 *
 * \since .5
 *
 *
 * \remark
 *  This call performs various operations on the Attribute-Value-Units
 *  (AVU) triples type of metadata.  The Units are optional, so these
 *  are frequently Attribute-Value pairs.  ATUs are user-defined
 *  metadata items.  The imeta command makes extensive use of this and
 *  the genQuery call.
 *
 * \note none
 *
 * \usage
 *
 * \param[in] conn - A rcComm_t connection handle to the server
 * \param[in] modAVUMetadataInp
 *
 * \return integer
 * \retval 0 on success
 *
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcModAVUMetadata( rcComm_t *conn, modAVUMetadataInp_t *modAVUMetadataInp ) {
    int status;
    status = procApiRequest( conn, MOD_AVU_METADATA_AN, modAVUMetadataInp,
                             NULL, ( void ** ) NULL, NULL );

    return status;
}
