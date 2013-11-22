/**
 * @file  rcNcClose.c
 *
 */

/* This is script-generated code.  */ 
/* See ncClose.h for a description of this API call.*/

#include "ncClose.h"

/**
 * \fn rcNcClose (rcComm_t *conn, ncCloseInp_t *ncCloseInp)
 *
 * \brief netcdf close an opened object (equivalent to nc_close).
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
 * nc_close an opened NETCDF data object:
 * \n ncOpenInp_t ncOpenInp;
 * \n ncCloseInp_t ncCloseInp;
 * \n int ncid = 0;
* \n int status;
 * \n bzero (&ncOpenInp, sizeof (ncOpenInp));
 * \n rstrcpy (ncOpenInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n ncOpenInp.mode = NC_NOWRITE;
 * \n status = rcNcOpen (conn, &ncOpenInp, &ncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n .... do something with ncid
 * \n ncCloseInp.ncid = ncid;
 * \n status = rcNcClose (conn, &ncCloseInp);
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncCloseInp - Elements of ncCloseInp_t used :
 *    \li int \b ncid - the ncid to close 
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
rcNcClose (rcComm_t *conn, ncCloseInp_t *ncCloseInp)
{
    int status;
    status = procApiRequest (conn, NC_CLOSE_AN, ncCloseInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
