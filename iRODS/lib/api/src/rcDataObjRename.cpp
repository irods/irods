/**
 * @file  rcDataObjRename.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjRename.h for a description of this API call.*/

#include "dataObjRename.h"

/**
 * \fn rcDataObjRename (rcComm_t *conn, dataObjCopyInp_t *dataObjRenameInp)
 *
 * \brief Rename an iRODS path. The iRODS path can be a data object or a
 *           collection. This is equivalent to rename of UNIX.
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
 * Rename an iRODS path /myZone/home/john/mypathA to /myZone/home/john/mypathB
 *     and store in myRescource:
 * \n dataObjCopyInp_t dataObjRenameInp;
 * \n bzero (&dataObjRenameInp, sizeof (dataObjRenameInp));
 * \n rstrcpy (dataObjRenameInp.destDataObjInp.objPath,
 *       "/myZone/home/john/mypathB", MAX_NAME_LEN);
 * \n rstrcpy (dataObjRenameInp.srcDataObjInp.objPath,
 *      "/myZone/home/john/mypathA", MAX_NAME_LEN);
 * \n status = rcDataObjRename (conn, &dataObjRenameInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjRenameInp_t - Elements of dataObjCopyInp_t used :
 *    \li char \b srcDataObjInp.objPath[MAX_NAME_LEN] - full path of the
 *         source path.
 *    \li char \b destDataObjInp.objPath[MAX_NAME_LEN] - full path of the
 *         target path.
 *    \li int \b srcDataObjInp.oprType - The operation type. 
 *        Valid values are:
 *      \n RENAME_DATA_OBJ  - renmame a data object.
 *      \n RENAME_COLL  - renmame a collection.
 *      \n 0 - The object type is unknown. Just rename it.
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
rcDataObjRename (rcComm_t *conn, dataObjCopyInp_t *dataObjRenameInp)
{
    int status;


    status = procApiRequest (conn, DATA_OBJ_RENAME_AN,  dataObjRenameInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
