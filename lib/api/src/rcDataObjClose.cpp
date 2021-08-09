#include "dataObjClose.h"
#include "procApiRequest.h"
#include "apiNumber.h"

#include <cstring>

/**
 * \fn rcDataObjClose (rcComm_t *conn, openedDataObjInp_t *dataObjCloseInp)
 *
 * \brief Close an opened data object.
 *
 * \user client
 *
 * \ingroup data_object
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Close an open a data object:
 * \n dataObjInp_t dataObjInp;
 * \n openedDataObjInp_t dataObjCloseInp;
 * \n int status;
 * \n memset(&dataObjInp, 0, sizeof(dataObjInp));
 * \n memset(&dataObjCloseInp, 0, sizeof(dataObjCloseInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.openFlags = O_WRONLY;
 * \n dataObjCloseInp.l1descInx = rcDataObjOpen (conn, &dataObjInp);
 * \n if (dataObjCloseInp.l1descInx < 0) {
 * \n .... handle the error
 * \n }
 * \n    .... do some I/O (rcDataObjWrite) ....
 * \n status = rcDataObjClose (conn, &dataObjCloseInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjCloseInp - Elements of openedDataObjInp_t used :
 *    \li int \b l1descInx - the opened data object descriptor from
 *      rcDataObjOpen or rcDataObjCreate.
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int rcDataObjClose(rcComm_t* conn, openedDataObjInp_t* dataObjCloseInp)
{
    return procApiRequest(conn, DATA_OBJ_CLOSE_AN,  dataObjCloseInp, NULL, (void **) NULL, NULL);
}
