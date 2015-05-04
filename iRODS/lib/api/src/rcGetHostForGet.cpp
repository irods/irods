/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "getHostForGet.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcGetHostForGet( rcComm_t *conn, dataObjInp_t *dataObjInp, char **outHost )
 *
 * \brief Get the best host for the get operation.
 *
 * \user client
 *
 * \ingroup miscellaneous
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - generic dataObj input. Relevant items are:
 *      \n      \b objPath - the path of the target collection.
 *      \n      \b condInput - conditional Input
 *      \n              -- REPL_NUM_KW  - The replica number of the copy to upload.
 *      \n              -- RESC_NAME_KW - The default destination resource. Only used
 *                              to create a new file, no overwrite of existing files.
 * \param[out] outHost - the address of the best host.
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcGetHostForGet( rcComm_t *conn, dataObjInp_t *dataObjInp,
                 char **outHost ) {
    int status;
    status = procApiRequest( conn, GET_HOST_FOR_GET_AN,  dataObjInp, NULL,
                             ( void ** ) outHost, NULL );

    return status;
}
