/**
 * @file  rcNcCreate.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See ncCreate.h for a description of this API call.*/

#include "ncCreate.h"

/**
 * \fn rcNcCreate (rcComm_t *conn, ncOpenInp_t *ncCreateInp, int *ncid)
 *
 * \brief netcdf open an iRODS data object (equivalent to nc_open).
 *
 * \user client
 *
 * \category NETCDF operations
 *
 * \since 3.1
 *
 * \author  Mike Wan
 * \date    2011
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * nc_open a data object /myZone/home/john/myfile.nc for write:
 * \n ncOpenInp_t ncCreateInp;
 * \n int ncid = 0;
 * \n int status;
 * \n bzero (&ncCreateInp, sizeof (ncCreateInp));
 * \n rstrcpy (ncCreateInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n dataObjInp.mode = NC_NOWRITE;
 * \n status = rcNcCreate (conn, &ncCreateInp, &ncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncCreateInp - Elements of ncOpenInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li int \b mode - the mode of the open - valid values are given in netcdf.h - NC_NOWRITE, NC_WRITE
 * \param[out] ncid - the ncid of the opened object.
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
rcNcCreate (rcComm_t *conn, ncOpenInp_t *ncCreateInp, int *ncid)
{
    int status;
    int *myncid = NULL;

    status = procApiRequest (conn, NC_CREATE_AN,  ncCreateInp, NULL,
        (void **) &myncid, NULL);

    if (myncid != NULL) {
	*ncid = *myncid;
	free (myncid);
    }
    return (status);
}

int
_rcNcCreate (rcComm_t *conn, ncOpenInp_t *ncCreateInp, int **ncid)
{
    int status;
    status = procApiRequest (conn, NC_CREATE_AN,  ncCreateInp, NULL, 
        (void **) ncid, NULL);

    return (status);
}

