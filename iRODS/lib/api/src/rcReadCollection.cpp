/**
 * @file  rcReadCollection.c
 *
 */

/* This is script-generated code.  */ 
/* See readCollection.h for a description of this API call.*/

#include "readCollection.h"

/**
 * \fn rcReadCollection (rcComm_t *conn, int handleInxInp, collEnt_t **collEnt)
 *
 * \brief Read from an opened collection (from rcOpenCollection). 
 *    The result is returned in a collEnt_t structure.
 *    This is equivalent to readdir of UNIX.
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
 * Open the collection /myZone/home/john/coll1/coll2 for recursive query
 * and read from it.
 * \n int status, handleInx;
 * \n collInp_t collOpenInp;
 * \n collEnt_t *collEnt = NULL;
 * \n bzero (&collOpenInp, sizeof (collOpenInp));
 * \n rstrcpy (collOpenInp.collName, "/myZone/home/john/coll1/coll2",
 *     MAX_NAME_LEN);
 * \n collOpenInp.flags = RECUR_QUERY_FG | VERY_LONG_METADATA_FG;
 * \n handleInx = rcOpenCollection (conn, &collOpenInp);
 * \n if (handleInx < 0) {
 * \n .... handle the error
 * \n }
 * \n while ((status = rcReadCollection (conn, handleInx, &collEnt)) >= 0) {
 * \n     if (collEnt->objType == DATA_OBJ_T) {
 * \n         # handle the data object
 * \n     } else if (collEnt->objType == COLL_OBJ_T) {
 * \n         # handle the subcollection
 * \n     }
 * \n     freeCollEnt (collEnt);
 * \n }
 * \n # close the collection
 * \n rcCloseCollection (conn, handleInx);
 * \n
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in]  handleInxInp - The opened collection index from rcOpenCollection.
 * \param[out] collEnt - A point to a collEnt_t structure containing the
 *     result of the read.
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa rclOpenCollection/rclReadCollection/rclCloseCollection. The query
 * for these functions is done by the client and the query results are
 * cached locally making it more efficient.
 * \bug  no known bugs
**/

int
rcReadCollection (rcComm_t *conn, int handleInxInp,
collEnt_t **collEnt)
{
    int status;
    status = procApiRequest (conn, READ_COLLECTION_AN, &handleInxInp, NULL, 
        (void **) collEnt, NULL);

    return (status);
}
