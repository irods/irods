/**
 * @file  rcNcOpen.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See ncOpen.h for a description of this API call.*/

#include "ncGetAggElement.h"
#include "ncInq.h"

/**
 * \fn ncGetAggElement (rcComm_t *conn, ncOpenInp_t *ncOpenInp,
ncAggElement_t **ncAggElement)
 *
 * \brief get the ncAggElement of a NETCDF file
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
 * rcNcGetAggElement a data object /myZone/home/john/myfile.nc :
 * \n ncOpenInp_t ncOpenInp;
 * \n ncAggElement_t *ncAggElement = NULL;
 * \n int ncid = 0;
 * \n int status;
 * \n bzero (&ncOpenInp, sizeof (ncOpenInp));
 * \n rstrcpy (ncOpenInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n status = rcNcGetAggElement (conn, &ncOpenInp, &ncAggElement);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncOpenInp - Elements of ncOpenInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 * \param[out] ncAggElement - the ncAggElement of the opened object.
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
rcNcGetAggElement (rcComm_t *conn, ncOpenInp_t *ncOpenInp,
ncAggElement_t **ncAggElement)
{
    int status;

    status = procApiRequest (conn, NC_GET_AGG_ELEMENT_AN,  ncOpenInp, NULL,
        (void **) ncAggElement, NULL);

    return (status);
}

