/**
 * @file  rcNcInqWithId.c
 *
 */

/* This is script-generated code.  */ 
/* See ncInqWithId.h for a description of this API call.*/

#include "ncInqWithId.h"

/**
 * \fn rcNcInqWithId (rcComm_t *conn, ncInqIdInp_t *ncInqWithIdInp, ncInqWithIdOut_t **ncInqWithIdOut)
 *
 * \brief General netcdf inquiry with id (equivalent nc_inq_dim, nc_inq_dim, nc_inq_var ....) This API is superceded by the more comprehensive rcNcInq API.

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
 * nc_open a data object and inquire the 'time dimension:
 * \n ncOpenInp_t ncOpenInp;
 * \n ncInqIdInp_t ncInqIdInp;
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
 * \n status = rcNcInqId (conn, &ncInqIdInp, &ncInqIdOut);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n ncInqIdInp.myid = *outId;
 * \n status = rcNcInqWithId (conn, &ncInqIdInp, &ncInqWithIdOut);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncInqIdInp - Elements of ncInqIdInp_t used :
 *    \li int \b paramType - what to inquire - valid values are defined in ncInqId.h - NC_VAR_T or NC_DIM_T.
 *    \li int \b ncid - the ncid from ncNcOpen.
 *    \li int \b myid - the id from rcNcInqId.
 * \param[out] ncInqWithIdOut - a ncInqWithIdOut_t. Elements of ncInqWithIdOut_t:
 *    \li rodsLong_t \b mylong - Content depends on paramType.For NC_DIM_T, this is arrayLen. not used for NC_VAR_T.
 *    \li int \b dataType - data type for NC_VAR_T.
 *    \li int \b natts - number of attrs for NC_VAR_T.
 *    \li char \b name[MAX_NAME_LEN] - name of the parameter.
 *    \li int \b ndim - number of dimensions (rank) for NC_VAR_T.
 *    \li int \b *intArray - int array of dimIds and ndim length for NC_VAR_T.
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
rcNcInqWithId (rcComm_t *conn, ncInqIdInp_t *ncInqWithIdInp,
ncInqWithIdOut_t **ncInqWithIdOut)
{
    int status;

    status = procApiRequest (conn, NC_INQ_WITH_ID_AN, ncInqWithIdInp, NULL, 
        (void **) ncInqWithIdOut, NULL);

    return (status);
}
