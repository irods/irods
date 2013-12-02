/**
 * @file  rcNccfGetVara.c
 *
 */

/* This is script-generated code.  */ 
/* See nccfGetVara.h for a description of this API call.*/

#include "nccfGetVara.h"

/**
 * \fn rcNccfGetVara (rcComm_t *conn,   nccfGetVarInp_t *nccfGetVarInp, nccfGetVarOut_t ** nccfGetVarOut)
 *
 * \brief libcf subsetting function. libcf function nccf_get_vara is called
 *         on the server. The iRODS software must be built with LIB_CF enabled
 *         (the line LIB_CF uncommented in config.mk). 
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
 * \n int status, i;
 * \n ncInqInp_t ncInqInp;
 * \n ncInqOut_t *ncInqOut = NULL;
 * \n nccfGetVarInp_t nccfGetVarInp;
 * \n nccfGetVarOut_t *nccfGetVarOut = NULL;
 * \n float *mydata;
 * \n bzero (&ncOpenInp, sizeof (ncOpenInp));
 * \n rstrcpy (ncOpenInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n ncOpenInp.mode = NC_NOWRITE;
 * \n status = rcNcOpen (conn, &ncOpenInp, &ncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n // inq the pressure var
 * \n bzero (&ncInqInp, sizeof (ncInqInp));
 * \n ncInqInp.ncid = ncid;
 * \n ncInqInp.paramType = NC_VAR_TYPE;
 * \n rstrcpy (ncInqInp.name, "pressure", MAX_NAME_LEN);
 * \n ncInqInp.flags = 0;
 * \n status = rcNcInq (conn, &ncInqInp, &ncInqOut);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n // pressure subset 
 * \n bzero (&nccfGetVarInp, sizeof (nccfGetVarInp));
 * \n nccfGetVarInp.ncid = ncid;
 * \n nccfGetVarInp.varid = ncInqOut->var->id;
 * \n nccfGetVarInp.latRange[0] = 30.0;
 * \n nccfGetVarInp.latRange[1] = 41.0;
 * \n nccfGetVarInp.lonRange[0] = -120.0;
 * \n nccfGetVarInp.lonRange[1] = -96.0;
 * \n nccfGetVarInp.maxOutArrayLen = 1000;
 * \n status = rcNccfGetVara (conn, &nccfGetVarInp, &nccfGetVarOut);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n mydata = (float *) nccfGetVarOut->dataArray->buf;
 * \n printf ("pressure values: ");
 * \n for (i = 0; i <  nccfGetVarOut->dataArray->len; i++) {
 * \n     printf (" %f\n", mydata[i]);
 * \n }
 * \n freeNccfGetVarOut (&nccfGetVarOut);
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] nccfGetVarInp - Elements of nccfGetVarInp_t used :
 *    \li int \b ncid - the ncid from ncNcOpen.
 *    \li int \b varid - the variable Id from rcNcInq.
 *    \li int \b lvlIndex - A zero-based index number for the verticle level of interest (Ignored if data has no vertical axis).
 *    \li int \b timestep - A zero-based index number for the timestep of interest (Ignored if data has no time axis).
 *    \li float \b latRange[2] - holds the latitude start and stop values for the range of interest.
 *    \li float \b holds the longitude start and stop values for the range of interest (Wrapping around the dateline is not allowed!).
 * \param[out] nccfGetVarOut - a nccfGetVarOut_t. Elements of nccfGetVarOut_t:
 *    \li int \b nlat - The number of latitude values which fall within the range.
 *    \li int \b nlon - The number of longitude values which fall within the range.
 *    \li char \b dataType_PI[NAME_LEN] - Packing instruction of the dataType.
 *    \li dataArray_t \b *dataArray - returned values of the variable. dataArray->type gives the var type; dataArray->len gives the var length; dataArray->buf contains the var values.
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
rcNccfGetVara (rcComm_t *conn,   nccfGetVarInp_t *nccfGetVarInp, 
 nccfGetVarOut_t ** nccfGetVarOut)
{
    int status;
    status = procApiRequest (conn, NCCF_GET_VARA_AN, nccfGetVarInp, NULL, 
        (void **) nccfGetVarOut, NULL);

    return (status);
}

int
freeNccfGetVarOut (nccfGetVarOut_t **nccfGetVarOut)
{
    if (nccfGetVarOut == NULL || *nccfGetVarOut == NULL) return
      USER__NULL_INPUT_ERR;

    if ((*nccfGetVarOut)->dataArray != NULL) {
        if ((*nccfGetVarOut)->dataArray->buf != NULL) {
            free ((*nccfGetVarOut)->dataArray->buf);
        }
        free ((*nccfGetVarOut)->dataArray);
    }
    free (*nccfGetVarOut);
    *nccfGetVarOut = NULL;
    return 0;
}

