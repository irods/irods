/**
 * @file  rcDataObjCreateAndStat.c
 *
 */

/* This is script-generated code.  */ 
/* See dataObjCreateAndStat.h for a description of this API call.*/

#include "dataObjCreateAndStat.h"

/**
 * \fn rcDataObjCreateAndStat (rcComm_t *conn, dataObjInp_t *dataObjInp,
 * openStat_t **openStat)
 *
 * \brief Create a data object in the iCAT. This API is the same as 
 *	the rcDataObjCreate API except an additional openStat_t struct 
 *	containing the stat of the just created object is returned.
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
 * \n openStat_t *openStat = NULL;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.createMode = 0750;
 * \n dataObjInp.dataSize = 12345;
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n status = rcDataObjCreateAndStat (conn, &dataObjInp, &openStat);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li int \b createMode - the file mode of the data object.
 *    \li rodsLong_t \b dataSize - the size of the data object.
 *      Input 0 if not known.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n DATA_TYPE_KW - the data type of the data object.
 *    \n DEST_RESC_NAME_KW - The resource to store this data object
 *    \n FILE_PATH_KW - The physical file path for this data object if the
 *             normal resource vault is not used.
 *    \n FORCE_FLAG_KW - overwrite existing copy. This keyWd has no value
 *
 * \param[out] openStat - Pointer to a openStat_t containing the stat of the
 *	just created object.
 * \return integer
 * \retval an opened object descriptor on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcDataObjCreateAndStat (rcComm_t *conn, dataObjInp_t *dataObjInp,
openStat_t **openStat)
{
    int status;
    status = procApiRequest (conn, DATA_OBJ_CREATE_AND_STAT_AN, dataObjInp, NULL,
        (void **) openStat, NULL);

    return (status);
}


