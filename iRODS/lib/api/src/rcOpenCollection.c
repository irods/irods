/**
 * @file  rcOpenCollection.c
 *
 */

/* This is script-generated code.  */ 
/* See openCollection.h for a description of this API call.*/

#include "openCollection.h"

/**
 * \fn rcOpenCollection (rcComm_t *conn, collInp_t *openCollInp)
 *
 * \brief Open a collection for subsequent read (rcReadCollection). 
 *    This is equivalent to opendir of UNIX.
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
 * \param[in] openCollInp - Elements of collInp_t used :
 *    \li char \b collName[MAX_NAME_LEN] - full path of the collection.
 *    \li int \b flags - Valid flags are:
 *    \n LONG_METADATA_FG - the subsequent read (rcReadCollection) returns 
 *         verbose metadata.
 *    \n VERY_LONG_METADATA_FG - the subsequent read returns very verbose 
 *          metadata.
 *    \n RECUR_QUERY_FG - the query is recursive. i.e., contents of all 
 *        sub-collections are also returned.
 *    \n DATA_QUERY_FIRST_FG - the query returns data objects in the 
 *         collection first, then the sub-collections. 
 *         Normally it is the other way around.
 *    \n INCLUDE_CONDINPUT_IN_QUERY - additional selection screening base on 
 *         condInput.
 *    \n These flags can be combined with the "or" operator. 
 *    \n e.g., flags = LONG_METADATA_FG|RECUR_QUERY_FG.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n RESC_NAME_KW - selection only data objects in this resource. 
 *        The INCLUDE_CONDINPUT_IN_QUERY flag must be on to be effective.
 *    \n RESC_NAME_KW - selection only data objects in this resource group. 
 *        The INCLUDE_CONDINPUT_IN_QUERY flag must be on to be effective.
 * \return integer
 * \retval the opened collection handle on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa rclOpenCollection/rclReadCollection/rclCloseCollection. The query
 * for these functions is done by the client and the query results are
 * cached locally making it more efficient. 
 * \bug  no known bugs
**/

int
rcOpenCollection (rcComm_t *conn, collInp_t *openCollInp)
{
    int status;
    status = procApiRequest (conn, OPEN_COLLECTION_AN, openCollInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
