/**
 * @file  rcPhyPathReg.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See phyPathReg.h for a description of this API call.*/

#include "phyPathReg.h"

/**
 * \fn rcPhyPathReg (rcComm_t *conn, dataObjInp_t *phyPathRegInp)
 *
 * \brief Register a physical file into iRODS as a data object.
 *     The file must already exist on the server where the input resource is located.
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
 * Register a UNIX file /data/john/myfile as an iRODS data object 
 * /myZone/home/john/myfile. This file is located on the same server as 
 * "myRescource" resource:
 * \n dataObjInp_t phyPathRegInp;
 * \n bzero (&phyPathRegInp, sizeof (phyPathRegInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n addKeyVal (&dataObjInp.condInput, FILE_PATH_KW, "/data/john/myfile");
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n status = rcPhyPathReg (conn, &dataObjInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n FILE_PATH_KW - The physical file path  of this data object. The server
 *         UNIX user (the UNIX user that runs the server) must have read/write
 *         access to this file.
 *    \n DATA_TYPE_KW - the data type of the data object.
 *    \n DEST_RESC_NAME_KW - The resource where the physical file is located. 
 *    \n RESC_GROUP_NAME_KW - The resource group of the resource. The resource
 *         given with DEST_RESC_NAME_KW must be a member of this resource group. 
 *    \n REG_REPL_KW - The file is registered as a replica of an existing data 
 *         object.
 *    \n REG_CHKSUM_KW - register the given checksum value with the iCAT.
 *    \n VERIFY_CHKSUM_KW -  calculate the checksum of the file on the iRODS 
 *        server and register the value with the iCAT. This keyWd has no value.
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
rcPhyPathReg (rcComm_t *conn, dataObjInp_t *phyPathRegInp)
{
    int status;
    status = procApiRequest (conn, PHY_PATH_REG_AN,  phyPathRegInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}

