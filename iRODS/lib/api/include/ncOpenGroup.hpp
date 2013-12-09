/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncOpen.h
 */

#ifndef NC_OPEN_GROUP_HPP
#define NC_OPEN_GROUP_HPP

/* This is a NETCDF API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"
#include "dataObjInpOut.hpp"
#include "ncOpen.hpp"
#ifdef NETCDF_API
#include "netcdf.hpp"
#endif

#define NcOpenGroupInp_PI "str objPath[MAX_NAME_LEN]; int mode; int rootNcid; double intialsz; double bufrsizehint; struct KeyValPair_PI;"

#if defined(RODS_SERVER)
#define RS_NC_OPEN_GROUP rsNcOpenGroup
/* prototype for the server handler */
int
rsNcOpenGroup( rsComm_t *rsComm, ncOpenInp_t *ncOpenGroupInp, int **ncid );
#else
#define RS_NC_OPEN_GROUP NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

    /* rcNcOpenGroup - open a HDF5 group. On the server, the nc_inq_grp_full_ncid
     * is call to get the grp_ncid which will be used for future processing.
     * Input -
     *   rcComm_t *conn - The client connection handle.
     *   ncOpenInp_t *ncOpenGroupInp - generic nc open/create input. Relevant items are:
     *	objPath - the full group path.
     *      rootNcid - the ncid of the root group (obtained by the rcNcOpen call.
     *	condInput - condition input (not used).
     * OutPut -
     *   int the ncid of the opened group - an integer descriptor.
     */

    /* prototype for the client call */
    int
    rcNcOpenGroup( rcComm_t *conn, ncOpenInp_t *ncOpenGroupInp, int *ncid );
    int
    _rcNcOpenGroup( rcComm_t *conn, ncOpenInp_t *ncOpenGroupInp, int **ncid );

#ifdef  __cplusplus
}
#endif

#endif	/* NC_OPEN_GROUP_H */
