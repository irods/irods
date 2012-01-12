/**
 * @file  rcDataObjClose.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjClose.h for a description of this API call.*/

#include "dataObjClose.h"

/**
 * \fn rcDataObjClose (rcComm_t *conn, openedDataObjInp_t *dataObjCloseInp)
 *
 * \brief Close an opened data object.
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
 * Close an open a data object:
 * \n dataObjInp_t dataObjInp;
 * \n openedDataObjInp_t dataObjCloseInp;
 * \n int status;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n bzero (&dataObjCloseInp, sizeof (dataObjCloseInp));
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
 * \bug  no known bugs
**/

int
rcDataObjClose (rcComm_t *conn, openedDataObjInp_t *dataObjCloseInp)
{
    int status;
    status = procApiRequest (conn, DATA_OBJ_CLOSE_AN,  dataObjCloseInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
