/**
 * @file  rcRmColl.c
 *
 */

/* This is script-generated code.  */ 
/* See rmColl.h for a description of this API call.*/

#include "rmColl.h"

int
_rcRmColl (rcComm_t *conn, collInp_t *rmCollInp, 
collOprStat_t **collOprStat)
{
    int status;
    status = procApiRequest (conn, RM_COLL_AN, rmCollInp, NULL, 
        (void **) collOprStat, NULL);

    return (status);
}

/**
 * \fn rcRmColl (rcComm_t *conn, collInp_t *rmCollInp, int vFlag)
 *
 * \brief Delete a collection if the "forceFlag" is set. Otherwise,
 *      the collection is moved to trash. As an option, the collection can be deleted 
 *      recursively meaning the collection and its contents are deleted.
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
 * Recursively delete the collection /myZone/home/john/mydir
 * \n int status;
 * \n collInp_t rmCollInp;
 * \n bzero (&rmCollInp, sizeof (rmCollInp));
 * \n rstrcpy (rmCollInp.collName, "/myZone/home/john/mydir", MAX_NAME_LEN);
 * \n addKeyVal (&rmCollInp->condInput, FORCE_FLAG_KW, "");
 * \n addKeyVal (&rmCollInp->condInput, RECURSIVE_OPR__KW, "");
 * \n status = rcRmColl (conn, &rmCollInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] rmCollInp - Elements of collInp_t used :
 *    \li char \b collName[MAX_NAME_LEN] - full path of the collection.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n RECURSIVE_OPR__KW - Recursively delete the collection and its content. 
 *         This keyWd has no value.
 *    \n FORCE_FLAG_KW - delete the collection. If it is not set, the collection
 *         is moved to trash. This keyWd has no value.
 *    \n UNREG_COLL_KW - The collections and data objects in this collection is 
 *         unregistered instead of deleted. i.e., physical files and directories
 *         in this collection are not deleted. This keyWd has no value.
 *    \n IRODS_RMTRASH_KW - delete the trash in this path.  This keyWd has no value.
 *    \n IRODS_ADMIN_RMTRASH_KW - ddmin user delete other user's trash in this path.
 *         This keyWd has no value.
 * \param[in] vFlag - Verbose flag. Verbose output if set to greater than 0.
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
rcRmColl (rcComm_t *conn, collInp_t *rmCollInp, int vFlag)
{
    int status, retval;
    collOprStat_t *collOprStat = NULL;

    retval = _rcRmColl (conn, rmCollInp, &collOprStat);

    status = cliGetCollOprStat (conn, collOprStat, vFlag, retval);

    return (status);
}

