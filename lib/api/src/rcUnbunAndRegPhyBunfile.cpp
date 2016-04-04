#include "unbunAndRegPhyBunfile.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcUnbunAndRegPhyBunfile( rcComm_t *conn, dataObjInp_t *dataObjInp )
 *
 * \brief Unbundle and register a physical bundled file.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - the dataObjInfo input
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcUnbunAndRegPhyBunfile( rcComm_t *conn, dataObjInp_t *dataObjInp ) {
    int status;
    status = procApiRequest( conn, UNBUN_AND_REG_PHY_BUNFILE_AN,  dataObjInp,
                             NULL, ( void ** ) NULL, NULL );

    return status;
}

