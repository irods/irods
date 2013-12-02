/**
 * @file  rcObjStat.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See objStat.h for a description of this API call.*/

#include "objStat.h"

/**
 * \fn rcObjStat (rcComm_t *conn, dataObjInp_t *dataObjInp,
 *   rodsObjStat_t **rodsObjStatOut)
 *
 * \brief Return information about an iRODS path. This path can be a data objection or
 * a collection. This call returns a rodsObjStat_t structure containing the information.
 * This is equivalent to stat of UNIX.
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
 * Get information on an iRODS path /myZone/home/john/myfile:
 * \n dataObjInp_t dataObjInp;
 * \n rodsObjStat_t *rodsObjStatOut = NULL;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n status = rcObjStat (conn, &dataObjInp, &rodsObjStatOut);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n # process the info contained in rodsObjStatOut and the free it.
 * \n ......
 * \n freeRodsObjStat (rodsObjStatOut);
 * \n
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the object. The path can be a
 *         data object or a collection.
 * \param[out]  rodsObjStatOut - Pointer to a rodsObjStat_t containing information
 * on the data object or collection.

 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcObjStat (rcComm_t *conn, dataObjInp_t *dataObjInp,
rodsObjStat_t **rodsObjStatOut)
{
    int status;

    status = procApiRequest (conn, OBJ_STAT_AN,  dataObjInp, NULL, 
        (void **) rodsObjStatOut, NULL);

    return (status);
}

