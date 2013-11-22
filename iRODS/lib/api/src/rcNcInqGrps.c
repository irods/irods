/**
 * @file  rcNcInqGrps.c
 *
 */

/* This is script-generated code.  */ 
/* See ncInqGrps.h for a description of this API call.*/

#include "ncInqGrps.h"

/**
 * \fn rcNcInqGrps (rcComm_t *conn, ncInqGrpsInp_t *ncInqGrpsInp, ncInqGrpsOut_t **ncInqGrpsOut)
 *
 * \brief Given the ncid, returns all full group paths. On the server, the 
 *          nc_inq_grpname_len and nc_inq_grpname_full are called 

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
 * Get the full group path in /myZone/home/john/myfile.nc:
 * \n ncOpenInp_t ncOpenInp;
 * \n int ncid = 0;
 * \n int grpncid = 0;
 * \n int status;
 * \n ncInqGrpsInp_t ncInqGrpsInp;
 * \n ncInqGrpsOut_t *ncInqGrpsOut = NULL;
 * \n ncInqInp_t ncInqInp;
 * \n ncInqOut_t *ncInqOut = NULL;
 * \n bzero (&ncOpenInp, sizeof (ncOpenInp));
 * \n rstrcpy (ncOpenInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n ncOpenInp.mode = NC_NOWRITE;
 * \n status = rcNcOpen (conn, &ncOpenInp, &ncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n bzero (&ncInqGrpsInp, sizeof (ncInqGrpsInp));
 * \n ncInqGrpsInp.ncid = ncid;
 * \n status = rcNcInqGrps (conn, &ncInqGrpsInp, &ncInqGrpsOut);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n if (ncInqGrpsOut->ngrps <= 0) {
 * \n .... handle the error
 * \n }
 * \n rstrcpy (ncOpenInp.objPath, ncInqGrpsOut->grpName[0], MAX_NAME_LEN);
 * \n freeNcInqGrpsOut (&ncInqGrpsOut);
 * \n ncOpenInp.rootNcid = ncid;
 * \n status = rcNcOpenGroup (conn, &ncOpenInp, &grpncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncInqGrpsInp - Elements of ncInqGrpsInp_t used :
 *    \li int \b ncid - the ncid of the opened object from rcNcOpen.
 * \param[out] ncInqGrpsOut - Elements of ncInqGrpsOut_t:
 *    \li int \b ngrps - number of groups
 *    \li char \b **grpName - An array of ngrps pointers containing the group names.
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
rcNcInqGrps (rcComm_t *conn, ncInqGrpsInp_t *ncInqGrpsInp,
ncInqGrpsOut_t **ncInqGrpsOut)
{
    int status;

    status = procApiRequest (conn, NC_INQ_GRPS_AN, ncInqGrpsInp, NULL, 
        (void **) ncInqGrpsOut, NULL);

    return (status);
}

int
freeNcInqGrpsOut (ncInqGrpsOut_t **ncInqGrpsOut)
{
    ncInqGrpsOut_t *myNInqGrpsOut;
    int i;

    if (ncInqGrpsOut == NULL || *ncInqGrpsOut == NULL) return 0;

    myNInqGrpsOut = *ncInqGrpsOut;
    for (i = 0; i < myNInqGrpsOut->ngrps; i++) {
	free (myNInqGrpsOut->grpName[i]);
    }
    if (myNInqGrpsOut->grpName != NULL) free (myNInqGrpsOut->grpName);
    free (myNInqGrpsOut);
    *ncInqGrpsOut = NULL;
    return 0;
}

