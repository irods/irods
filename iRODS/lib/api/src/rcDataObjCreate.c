/**
 * @file  rcDataObjCreate.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjCreate.h for a description of this API call.*/

#include "dataObjCreate.h"

/**
 * \fn rcDataObjCreate (rcComm_t *conn, dataObjInp_t *dataObjInp)
 *
 * \brief Create a data object in the iCAT. This is equivalent to creat of UNIX.
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
 * Create a data object /myZone/home/john/myfile in myRescource:
 * \n dataObjInp_t dataObjInp;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.createMode = 0750;
 * \n dataObjInp.dataSize = 12345;
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n status = rcDataObjCreate (conn, &dataObjInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li int \b createMode - the file mode of the data object.
 *    \li rodsLong_t \b dataSize - the size of the data object. 
 *	Input 0 if not known.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n DATA_TYPE_KW - the data type of the data object.
 *    \n DEST_RESC_NAME_KW - The resource to store this data object
 *    \n FILE_PATH_KW - The physical file path for this data object if the
 *             normal resource vault is not used.
 *    \n FORCE_FLAG_KW - overwrite existing copy. This keyWd has no value
 *
 * \return integer
 * \retval an opened object descriptor on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcDataObjCreate (rcComm_t *conn, dataObjInp_t *dataObjInp)
{
    int status;
    status = procApiRequest (conn, DATA_OBJ_CREATE_AN,  dataObjInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}

