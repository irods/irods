/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rmColl.h
 */

#ifndef RM_COLL_H
#define RM_COLL_H

/* This is a Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"
#include "miscUtil.h"

#if defined(RODS_SERVER)
#define RS_RM_COLL rsRmColl
/* prototype for the server handler */
int
rsRmColl (rsComm_t *rsComm, collInp_t *rmCollInp, 
collOprStat_t **collOprStat);
int
_rsRmColl (rsComm_t *rsComm, collInp_t *rmCollInp,
collOprStat_t **collOprStat);
int
svrUnregColl (rsComm_t *rsComm, collInp_t *rmCollInp);
int
_rsRmCollRecur (rsComm_t *rsComm, collInp_t *rmCollInp,
collOprStat_t **collOprStat);
int
_rsPhyRmColl (rsComm_t *rsComm, collInp_t *rmCollInp,
dataObjInfo_t *dataObjInfo, collOprStat_t **collOprStat);
#else
#define RS_RM_COLL NULL
#endif

#ifdef COMPAT_201
#if defined(RODS_SERVER)
#define RS_RM_COLL201 rsRmColl201
/* prototype for the server handler */
int
rsRmColl201 (rsComm_t *rsComm, collInp201_t *rmCollInp,
collOprStat_t **collOprStat);
#else
#define RS_RM_COLL201 NULL
#endif
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcRmColl (rcComm_t *conn, collInp_t *rmCollInp, int vFlag);
int
_rcRmColl (rcComm_t *conn, collInp_t *rmCollInp, 
collOprStat_t **collOprStat);

/* rcRmColl - Recursively Remove a iRODS collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   collInp_t *collInp - generic coll input. Relevant items are:
 *      collName - the collection to be registered.
 *      condInput - condition input (optional) - currently not used.
 *
 * OutPut -
 *   int status - status of the operation.
 */

#ifdef  __cplusplus
}
#endif

#endif	/* RM_COLL_H */
