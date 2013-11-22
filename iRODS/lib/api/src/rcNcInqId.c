/**
 * @file  rcNcInqId.c
 *
 */

/* This is script-generated code.  */ 
/* See ncInqId.h for a description of this API call.*/

#include "ncInqId.h"

/**
 * \fn rcNcInqId (rcComm_t *conn, ncInqIdInp_t *ncInqIdInp, int **outId)
 *
 * \brief General netcdf inquiry for id (equivalent to nc_inq_dimid, nc_inq_varid, .... This API is superceded by the more comprehensive rcNcInq API.

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
 * nc_open a data object and get the dimension Id of 'time':
 * \n ncOpenInp_t ncOpenInp;
 * \n ncInqIdInp_t ncInqIdInp;
 * \n int *outId;
 * \n int ncid = 0;
 * \n ncInqWithIdOut_t *ncInqWithIdOut = NULL;
 * \n int status;
 * \n bzero (&ncOpenInp, sizeof (ncOpenInp));
 * \n rstrcpy (ncOpenInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n ncOpenInp.mode = NC_NOWRITE;
 * \n status = rcNcOpen (conn, &ncOpenInp, &ncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n bzero (&ncInqIdInp, sizeof (ncInqIdInp));
 * \n ncInqIdInp.ncid = ncid;
 * \n ncInqIdInp.paramType = NC_DIM_T;
 * \n rstrcpy (ncInqIdInp.name, "time", MAX_NAME_LEN);
 * \n status = rcNcInqId (conn, &ncInqIdInp, &outId);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n ncInqIdInp.myid = *outId;
 * \n status = rcNcInqWithId (conn, &ncInqIdInp, &ncInqWithIdOut);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncInqIdInp - Elements of ncInqIdInp_t used :
 *    \li int \b paramType - what to inquire - valid values are defined in ncInqId.h - NC_VAR_T or NC_DIM_T.
 *    \li int \b ncid - the ncid from ncNcOpen.
 *    \li char \b name[MAX_NAME_LEN] - the name of the item to inquire.
 * \param[out] outId - the returned id of the inquiry 
 * \return integer
 * \retval status of the call. success if greater or equal 0. error if negative. * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcNcInqId (rcComm_t *conn, ncInqIdInp_t *ncInqIdInp, int **outId)
{
    int status;

    status = procApiRequest (conn, NC_INQ_ID_AN, ncInqIdInp, NULL, 
        (void **) outId, NULL);

    return (status);
}
