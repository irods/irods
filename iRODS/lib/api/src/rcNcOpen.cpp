/**
 * @file  rcNcOpen.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See ncOpen.h for a description of this API call.*/

#include "ncOpen.h"

/**
 * \fn rcNcOpen (rcComm_t *conn, ncOpenInp_t *ncOpenInp, int *ncid)
 *
 * \brief open an iRODS data object for netcdf operation (equivalent to nc_open).
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
 * nc_open a data object /myZone/home/john/myfile.nc for write:
 * \n ncOpenInp_t ncOpenInp;
 * \n int ncid = 0;
 * \n int status;
 * \n bzero (&ncOpenInp, sizeof (ncOpenInp));
 * \n rstrcpy (ncOpenInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n ncOpenInp.mode = NC_NOWRITE;
 * \n status = rcNcOpen (conn, &ncOpenInp, &ncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncOpenInp - Elements of ncOpenInp_t used :
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
rcNcOpen (rcComm_t *conn, ncOpenInp_t *ncOpenInp, int *ncid)
{
    int status;
    int *myncid = NULL;

    status = procApiRequest (conn, NC_OPEN_AN,  ncOpenInp, NULL,
        (void **) &myncid, NULL);

    if (myncid != NULL) {
	*ncid = *myncid;
	free (myncid);
    }
    return (status);
}

int
_rcNcOpen (rcComm_t *conn, ncOpenInp_t *ncOpenInp, int **ncid)
{
    int status;
    status = procApiRequest (conn, NC_OPEN_AN,  ncOpenInp, NULL, 
        (void **) ncid, NULL);

    return (status);
}

