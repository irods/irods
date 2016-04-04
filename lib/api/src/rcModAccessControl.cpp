
#include "modAccessControl.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcModAccessControl( rcComm_t *conn, modAccessControlInp_t *modAccessControlInp )
 *
 * \brief Modifies the access control metadata for iRODS entities.
 *
 * \user client and server
 *
 * \ingroup metadata
 *
 * \since .5
 *
 *
 * \remark
 *  This call performs various operations to modify the access control
 *  metadata for various types of objects.  This is used by the ichmod
 *  command and processed by the chlModAccessControl routine.
 *
 * \note none
 *
 * \usage
 *
 * \param[in] conn - A rcComm_t connection handle to the server
 * \param[in] modAccessControlInp
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
rcModAccessControl( rcComm_t *conn, modAccessControlInp_t *modAccessControlInp ) {
    int status;
    status = procApiRequest( conn, MOD_ACCESS_CONTROL_AN,  modAccessControlInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
