/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* collCreate.h
 */

#ifndef COLL_CREATE_H
#define COLL_CREATE_H

/* This is a high level type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"

#if defined(RODS_SERVER)
#define RS_COLL_CREATE rsCollCreate
/* prototype for the server handler */
int
rsCollCreate (rsComm_t *rsComm, collInp_t *collCreateInp);
int
remoteCollCreate (rsComm_t *rsComm, collInp_t *collCreateInp);
int
l3Mkdir (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo);
#else
#define RS_COLL_CREATE NULL
#endif

#ifdef COMPAT_201
#if defined(RODS_SERVER)
#define RS_COLL_CREATE201 rsCollCreate201
/* prototype for the server handler */
int
rsCollCreate201 (rsComm_t *rsComm, collInp201_t *collCreateInp);
#else
#define RS_COLL_CREATE201 NULL
#endif
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcCollCreate (rcComm_t *conn, collInp_t *collCreateInp);

/* rcCollCreate - Create a iRODS collection.
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

#endif	/* COLL_CREATE_H */
