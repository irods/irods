/**
 * @file  rcDataObjChksum.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjChksum.h for a description of this API call.*/

#include "dataObjChksum.h"

/**
 * \fn rcDataObjChksum (rcComm_t *conn, dataObjInp_t *dataObjInp,
 *       char **outChksum)
 *
 * \brief Compute the md5 checksum of a data object and register the checksum 
 *      value with iCAT.
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
 * Chksum the data object /myZone/home/john/myfile if one does not already 
 * exist in iCAT.
 * \n dataObjInp_t dataObjInp;
 * \n char *outChksum = NULL;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n status = rcDataObjChksum (conn, &dataObjInp, &outChksum);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n VERIFY_CHKSUM_KW -  verify the checksum value in iCAT. If the 
 *            checksum value does not exist, compute and register one.
 *            This keyWd has no value.
 *    \n FORCE_CHKSUM_KW -  checksum the data-object even if a checksum 
 *            already exists in iCAT. This keyWd has no value.
 *    \n CHKSUM_ALL_KW - checksum all replicas.
 *            This keyWd has no value.
 *    \n REPL_NUM_KW - The replica number of the replica to delete.
 *    \n RESC_NAME_KW - delete replica stored in this resource.
 * \param[out] \li char \b **outChksum - a string containing the md5 checksum
 *             value.
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
rcDataObjChksum (rcComm_t *conn, dataObjInp_t *dataObjChksumInp, 
char **outChksum)
{
    int status;
    status = procApiRequest (conn, DATA_OBJ_CHKSUM_AN,  dataObjChksumInp, NULL, 
        (void **) outChksum, NULL);

    return (status);
}

