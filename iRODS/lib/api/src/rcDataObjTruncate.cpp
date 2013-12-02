/**
 * @file  rcDataObjTruncate.c
 *
 */

/* This is script-generated code.  */ 
/* See dataObjTruncate.h for a description of this API call.*/

#include "dataObjTruncate.h"

/**
 * \fn rcDataObjTruncate (rcComm_t *conn, dataObjInp_t *dataObjInp)
 *
 * \brief Truncate a data object to the specified size and register the 
 *       new size with iCAT. The old checksum value associated with the
 *       the data object will be cleared.
 *
 * \user client
 *
 * \category data object operations
 *
 * \since 1.0
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Truncate size the data object /myZone/home/john/myfile to 12345.
 * \n dataObjInp_t dataObjInp;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.dataSize = 12345;
 * \n status = rcDataObjTruncate (conn, &dataObjInp);
 * \n if (status < 0) {
* \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li rodsLong_t \b dataSize - the new size of the data object.
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcDataObjTruncate (rcComm_t *conn, dataObjInp_t *dataObjInp)
{
    int status;
    status = procApiRequest (conn, DATA_OBJ_TRUNCATE_AN, dataObjInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
