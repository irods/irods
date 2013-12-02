/**
 * @file  rcDataObjLseek.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjLseek.h for a description of this API call.*/

#include "dataObjLseek.h"

/**
 * \fn rcDataObjLseek (rcComm_t *conn, openedDataObjInp_t *dataObjLseekInp,
fileLseekOut_t **dataObjLseekOut)
 *
 * \brief Repositions the offset of the open file associated
 *     with the file descriptor to the argument offset according to the
 *     directive whence. This is equivalent to lseek of UNIX. 
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
 * Advance the offset 12345 bytes from current position of an open 
 *    data object:
 * \n int status;
 * \n dataObjInp_t dataObjInp;
 * \n openedDataObjInp_t dataObjLseekInp;
 * \n fileLseekOut_t *dataObjLseekOut = NULL;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n bzero (&dataObjLseekInp, sizeof (dataObjLseekInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.openFlags = O_RDONLY;
 * \n dataObjLseekInp.l1descInx = rcDataObjOpen (conn, &dataObjInp);
 * \n if (dataObjLseekInp.l1descInx < 0) {
 * \n .... handle the error
 * \n }
 * \n dataObjLseekInp.offset = 12345;
 * \n dataObjLseekInp.whence = SEEK_CUR;
 * \n status = rcDataObjLseek (conn, &dataObjLseekInp, &dataObjLseekOut);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjLseekInp - Elements of openedDataObjInp_t used :
 *    \li int \b l1descInx - the opened data object descriptor from
 *      rcDataObjOpen or rcDataObjCreate.
 *    \li int \b whence - Similar to lseek of UNIX. Valid values are:
 *        \n SEEK_SET - The offset is set to offset bytes.
 *        \n SEEK_CUR - The offset is set to its current location plus 
 *             offset bytes.
 *        \n SEEK_END - The offset is set to the size of the file plus 
 *             offset bytes. 
 *    \li rodsLong \b offset - the offset.
 * \param[out] fileLseekOut_t **dataObjLseekOut - pointer to a fileLseekOut_t
 *        containing the resulting offset location in bytes from the beginning 
 *        of the file.
 * \return integer
 * \retval 0 on success. 
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcDataObjLseek (rcComm_t *conn, openedDataObjInp_t *dataObjLseekInp,
fileLseekOut_t **dataObjLseekOut)
{
    int status;
    status = procApiRequest (conn, DATA_OBJ_LSEEK_AN,  dataObjLseekInp, NULL, 
        (void **) dataObjLseekOut, NULL);

    return (status);
}
