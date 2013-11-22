/**
 * @file  rcNcOpenGroup.c
 *
 */

/* This is script-generated code.  */ 
/* See ncOpenGroup.h for a description of this API call.*/

/**
 * \fn rcNcOpenGroup (rcComm_t *conn, ncOpenInp_t *ncOpenGroupInp, int *grpncid)
 *
 * \brief Open a fully qualified group name and get the group id. On the 
 *          server, nc_inq_grp_full_ncid is called to get the grpncid
 *
 * \user client
 *
 * \category NETCDF operations
 *
 * \since 1.0
 *
 * \author  Mike Wan
 * \date    2012
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Get the group id in /myZone/home/john/myfile.nc:
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
 * \n // do the general inq on the group
 * \n bzero (&ncInqInp, sizeof (ncInqInp));
 * \n ncInqInp.ncid = grpncid;
 * \n ncInqInp.paramType = NC_ALL_TYPE;
 * \n ncInqInp.flags = NC_ALL_FLAG;
 * \n status = rcNcInq (conn, &ncInqInp, &ncInqOut);
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

#include "ncOpenGroup.h"

int
rcNcOpenGroup (rcComm_t *conn, ncOpenInp_t *ncOpenGroupInp, int *ncid)
{
    int status;
   int *myncid = NULL;

    status = procApiRequest (conn, NC_OPEN_GROUP_AN, ncOpenGroupInp, NULL, 
        (void **) &myncid, NULL);

    if (myncid != NULL) {
        *ncid = *myncid;
        free (myncid);
    }

    return (status);
}

int
_rcNcOpenGroup (rcComm_t *conn, ncOpenInp_t *ncOpenGroupInp, int **ncid)
{
    int status;
    status = procApiRequest (conn, NC_OPEN_GROUP_AN,  ncOpenGroupInp, NULL,
        (void **) ncid, NULL);

    return (status);
}

