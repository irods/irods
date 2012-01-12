/**
 * @file  rcDataObjFsync.c
 *
 */

#include "dataObjFsync.h"

/**
 * \fn rcDataObjFsync (rcComm_t *conn, openedDataObjInp_t *dataObjFsyncInp)
 *
 * \brief Synchronize the size of the data object in iCAT to the physical size 
 *   in the storage vault. In addition, if the storage resource is a UNIX
 *   file system, a fsync call is also made to Synchronize the in-core data to
 *   disk.
 *
 * \user client
 *
 * \category data object operations
 *
 * \since 1.0
 *
 * \author  John Knutson, U of Texas
 * \date    2011
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Fsync an open a data object:
 * \n dataObjInp_t dataObjInp;
 * \n openedDataObjInp_t dataObjFsyncInp;
 * \n int status;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n bzero (&dataObjFsyncInp, sizeof (dataObjFsyncInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.openFlags = O_WRONLY;
 * \n dataObjFsyncInp.l1descInx = rcDataObjOpen (conn, &dataObjInp);
 * \n if (dataObjFsyncInp.l1descInx < 0) {
 * \n .... handle the error
 * \n }
 * \n status = rcDataObjFsync (conn, &dataObjFsyncInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjFsyncInp - Elements of openedDataObjInp_t used :
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
rcDataObjFsync (rcComm_t *conn, openedDataObjInp_t *dataObjFsyncInp)
{
   int status;
   rodsLog (LOG_NOTICE, "rcDataObjFsync calling procApiRequest");
   status = procApiRequest (conn, DATA_OBJ_FSYNC_AN, dataObjFsyncInp, NULL,
                            NULL, NULL);

   return (status);
}

