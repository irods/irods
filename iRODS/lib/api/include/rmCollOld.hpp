/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rmColl.h
 */

#ifndef RM_COLL_OLD_H
#define RM_COLL_OLD_H

/* This is a Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"
#include "miscUtil.h"

#if defined(RODS_SERVER)
#define RS_RM_COLL_OLD rsRmCollOld
/* prototype for the server handler */
int
rsRmCollOld (rsComm_t *rsComm, collInp_t *rmCollInp);
int
remoteRmCollOld (rsComm_t *rsComm, collInp_t *rmCollInp);
int
_rsRmCollOld (rsComm_t *rsComm, collInp_t *rmCollInp);
int
_rsRmCollRecurOld (rsComm_t *rsComm, collInp_t *rmCollInp); 
int
rsMvCollToTrash (rsComm_t *rsComm, collInp_t *rmCollInp);
int
rsMkTrashPath (rsComm_t *rsComm, char *objPath, char *trashPath);
int
rsPhyRmCollRecurOld (rsComm_t *rsComm, collInp_t *rmCollInp);
int
svrRmCollOld (rsComm_t *rsComm, collInp_t *rmCollInp);
int
svrRmSpecColl (rsComm_t *rsComm, collInp_t *rmCollInp, 
dataObjInfo_t *dataObjInfo);
int
svrRmSpecCollRecur (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo);
int
l3Rmdir (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo);
#else
#define RS_RM_COLL_OLD NULL
#endif

#ifdef COMPAT_201
#if defined(RODS_SERVER)
#define RS_RM_COLL_OLD201 rsRmCollOld201
/* prototype for the server handler */
int
rsRmCollOld201 (rsComm_t *rsComm, collInp201_t *rmCollInp);
#else
#define RS_RM_COLL_OLD201 NULL
#endif
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcRmCollOld (rcComm_t *conn, collInp_t *rmCollInp);

/* rcRmCollOld - Remove a iRODS collection.
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

#endif	/* RM_COLL_OLD_H */
