/**
 * @file  rcDataObjWrite.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjWrite.h for a description of this API call.*/

#include "dataObjWrite.h"

/**
 * \fn rcDataObjWrite (rcComm_t *conn, openedDataObjInp_t *dataObjWriteInp,
 * bytesBuf_t *dataObjWriteOutBBuf)
 *
 * \brief Write a chunk of data to an opened data object.
 *    This is equivalent to write of UNIX.
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
 * Write 12345 bytes from an open a data object:
 * \n dataObjInp_t dataObjInp;
 * \n openedDataObjInp_t dataObjWriteInp;
 * \n bytesBuf_t dataObjWriteOutBBuf;
 * \n int bytesWrite;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n bzero (&dataObjWriteInp, sizeof (dataObjWriteInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.createMode = 0750;
 * \n dataObjInp.dataSize = 12345;
 * \n dataObjInp.openFlags = O_WRONLY;
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n dataObjWriteInp.l1descInx = rcDataObjCreate (conn, &dataObjInp);
 * \n if (dataObjWriteInp.l1descInx < 0) {
 * \n .... handle the error
 * \n }
 * \n bzero (&dataObjWriteOutBBuf, sizeof (dataObjWriteOutBBuf));
 * \n dataObjWriteInp.len = 12345;
 * \n dataObjWriteInp.buf = malloc (12345);
 * \n if (dataObjWriteInp.buf == NULL) {
 * \n .... handle the error
 * \n } else {
 * \n       copy date to dataObjWriteInp.buf
 * \n }
 * \n bytesWrite = rcDataObjWrite (conn, &dataObjWriteInp, &dataObjWriteInpBBuf);
 * \n if (bytesWrite < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjWriteInp - Elements of openedDataObjInp_t used :
 *    \li int \b l1descInx - the opened data object descriptor from
 *      rcDataObjOpen or rcDataObjCreate.
 *    \li int \b len - the length to write
 * \param[in] dataObjWriteOutBBuf - A pointer to a bytesBuf_t containing the
 *      data to write.
 * \return integer
 * \retval the number of bytes write on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcDataObjWrite (rcComm_t *conn, openedDataObjInp_t *dataObjWriteInp,
bytesBuf_t *dataObjWriteInpBBuf)
{
    int status;
    status = procApiRequest (conn, DATA_OBJ_WRITE_AN,  dataObjWriteInp, 
      dataObjWriteInpBBuf, (void **) NULL, NULL);

    return (status);
}
