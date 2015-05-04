#include "phyBundleColl.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcPhyBundleColl( rcComm_t *conn, structFileExtAndRegInp_t *phyBundleCollInp )
 *
 * \brief Bundle a physical collection
 *
 * \user client
 *
 * \ingroup collection_object
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] phyBundleCollInp - The phyBundleColl structFileExtAndRegInp_t
 *
 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcPhyBundleColl( rcComm_t *conn,
                 structFileExtAndRegInp_t *phyBundleCollInp ) {
    int status;
    status = procApiRequest( conn, PHY_BUNDLE_COLL_AN, phyBundleCollInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
