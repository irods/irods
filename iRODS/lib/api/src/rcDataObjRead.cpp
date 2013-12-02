/**
 * @file  rcDataObjRead.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjRead.h for a description of this API call.*/

#include "dataObjRead.h"
/**
 * \fn rcDataObjRead (rcComm_t *conn, openedDataObjInp_t *dataObjReadInp,
 * bytesBuf_t *dataObjReadOutBBuf)
 *
 * \brief Read a chunk of data from an opened data object.
 *    This is equivalent to read of UNIX.
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
 * Read 12345 bytes from an open a data object:
 * \n dataObjInp_t dataObjInp;
 * \n openedDataObjInp_t dataObjReadInp;
 * \n bytesBuf_t dataObjReadOutBBuf;
 * \n int bytesRead;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n bzero (&dataObjReadInp, sizeof (dataObjReadInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.openFlags = O_RDONLY;
 * \n dataObjReadInp.l1descInx = rcDataObjOpen (conn, &dataObjInp);
 * \n if (dataObjReadInp.l1descInx < 0) {
 * \n .... handle the error
 * \n }
 * \n bzero (&dataObjReadOutBBuf, sizeof (dataObjReadOutBBuf));
 * \n dataObjReadInp.len = 12345;
 * \n bytesRead = rcDataObjRead (conn, &dataObjReadInp, &dataObjReadInpBBuf);
 * \n if (bytesRead < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjReadInp - Elements of openedDataObjInp_t used :
 *    \li int \b l1descInx - the opened data object descriptor from 
 *	rcDataObjOpen or rcDataObjCreate.
 *    \li int \b len - the length to read 
 * \param[out] dataObjReadOutBBuf - A pointer to a bytesBuf_t containing the
 *	data read.
 * \return integer
 * \retval the number of bytes read on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcDataObjRead (rcComm_t *conn, openedDataObjInp_t *dataObjReadInp,
bytesBuf_t *dataObjReadOutBBuf)
{
    int status;
    status = procApiRequest (conn, DATA_OBJ_READ_AN,  dataObjReadInp, NULL, 
        (void **) NULL, dataObjReadOutBBuf);

    return (status);
}
