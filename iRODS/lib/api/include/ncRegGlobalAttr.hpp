/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncRegGlobalAttr.h
 */

#ifndef NC_REG_GLOBAL_ATTR_HPP
#define NC_REG_GLOBAL_ATTR_HPP

/* This is a NETCDF API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"
#include "dataObjInpOut.hpp"
#ifdef NETCDF_API
#include "netcdf.hpp"
#endif

typedef struct {
    char objPath[MAX_NAME_LEN];
    int flags;		/* not used */
    int numAttrName;	/* number of AttrName in attrNameArray */
    char **attrNameArray;    /* array of pointers */
    keyValPair_t condInput;
} ncRegGlobalAttrInp_t;

#define NcRegGlobalAttrInp_PI "str objPath[MAX_NAME_LEN]; int flags; int numAttrName; str *attrNameArray[numAttrName]; struct KeyValPair_PI;"

#if defined(RODS_SERVER) && defined(NETCDF_API)
#define RS_NC_REG_GLOBAL_ATTR rsNcRegGlobalAttr
/* prototype for the server handler */
int
rsNcRegGlobalAttr( rsComm_t *rsComm, ncRegGlobalAttrInp_t *ncRegGlobalAttrInp );
int
_rsNcRegGlobalAttr( rsComm_t *rsComm, ncRegGlobalAttrInp_t *ncRegGlobalAttrInp );
#else
#define RS_NC_REG_GLOBAL_ATTR NULL
#endif

extern "C" {

    /* rcNcRegGlobalAttr - Register the NETCDF global variables as AUV in the
     * iRODS data object.
     * Input -
     *   rcComm_t *conn - The client connection handle.
     *   ncRegGlobalAttrInp_t *ncRegGlobalAttrInp - Relevant items are:
     *	objPath - the path of the NETCDF data object.
     *	condInput - condition input.
     *        ADMIN_KW - admin reg on behalf of other users
     * OutPut -
     *   int the ncid of the opened object - an integer descriptor.
     */

    /* prototype for the client call */
    int
    rcNcRegGlobalAttr( rcComm_t *conn, ncRegGlobalAttrInp_t *ncRegGlobalAttrInp );

    int
    clearRegGlobalAttrInp( ncRegGlobalAttrInp_t *ncRegGlobalAttrInp );

}

#endif	/* NC_REG_GLOBAL_ATTR_H */
