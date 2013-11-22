/**
 * @file  rcNcRegGlobalAttr.c
 *
 */

/* This is script-generated code.  */ 
/* See ncRegGlobalAttr.h for a description of this API call.*/

#include "ncRegGlobalAttr.h"

/**
 * \fn rcNcRegGlobalAttr (rcComm_t *conn, ncRegGlobalAttrInp_t *ncRegGlobalAttrInp)
 *
 * \brief Extract the NETCDF global variables in an iRODS data object and 
 *          register them as AUV. 

 *
 * \user client
 *
 * \category NETCDF operations
 *
 * \since 3.1
 *
 * \author  Mike Wan
 * \date    2012
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Extract and register all global attributes of "/myZone/home/john/myfile.nc"
 * \n ncRegGlobalAttrInp_t ncRegGlobalAttrInp;
 * \n int status;
 * \n bzero (&ncRegGlobalAttrInp, sizeof (ncRegGlobalAttrInp));
 * \n rstrcpy (ncRegGlobalAttrInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n status = rcNcRegGlobalAttr (conn, &ncRegGlobalAttrInp, &ncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncRegGlobalAttrInp - Elements of ncRegGlobalAttrInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li numAttrName - number of attributes to extract and register. If it is 0, ALL attributes will be done.
 *    \li char \b **attrNameArray - An array of numAttrName pointers containing the atrribute names.
 * \param[out] none
 *
 * \return integer
 * \retval status of the call. success if greater or equal 0. error if negative.

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcNcRegGlobalAttr (rcComm_t *conn, ncRegGlobalAttrInp_t *ncRegGlobalAttrInp)
{
    int status;
    status = procApiRequest (conn, NC_REG_GLOBAL_ATTR_AN, ncRegGlobalAttrInp, 
        NULL, (void **) NULL, NULL);

    return (status);
}

int
clearRegGlobalAttrInp (ncRegGlobalAttrInp_t *ncRegGlobalAttrInp)
{
    int i;

    if (ncRegGlobalAttrInp == NULL || ncRegGlobalAttrInp->numAttrName <= 0 ||
      ncRegGlobalAttrInp->attrNameArray == NULL)
        return (0);

    for (i = 0; i < ncRegGlobalAttrInp->numAttrName; i++) {
        free (ncRegGlobalAttrInp->attrNameArray[i]);
    }
    free (ncRegGlobalAttrInp->attrNameArray);
    clearKeyVal (&ncRegGlobalAttrInp->condInput);
    memset (ncRegGlobalAttrInp, 0, sizeof (ncRegGlobalAttrInp_t));
    return(0);

}

