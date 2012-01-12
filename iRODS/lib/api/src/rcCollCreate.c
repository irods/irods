/**
 * @file  rcCollCreate.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See collCreate.h for a description of this API call.*/

#include "collCreate.h"

/**
 * \fn rcCollCreate (rcComm_t *conn, collInp_t *collCreateInp)
 *
 * \brief Create a collection. This is equivalent to mkdir of UNIX.
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
 * Create a collection /myZone/home/john/coll1/coll2 and make parent collections as 
 * needed:
 * \n int status;
 * \n collInp_t collCreateInp;
 * \n bzero (&collCreateInp, sizeof (collCreateInp));
 * \n rstrcpy (collCreateInp.collName, "/myZone/home/john/coll1/coll2", MAX_NAME_LEN);
 * \n addKeyVal (&collCreateInp.condInput, RECURSIVE_OPR__KW, "");
 * \n status = rcCollCreate (conn, &collCreateInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] collCreateInp - Elements of collInp_t used :
 *    \li char \b collName[MAX_NAME_LEN] - full path of the collection.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n RECURSIVE_OPR__KW - make parent collections as needed. This keyWd has no value.
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcCollCreate (rcComm_t *conn, collInp_t *collCreateInp)
{
    int status;
    status = procApiRequest (conn, COLL_CREATE_AN,  collCreateInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
