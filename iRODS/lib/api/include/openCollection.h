/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* openCollection.h
 */

#ifndef OPEN_COLLECTION_H
#define OPEN_COLLECTION_H

/* This is a high level type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"

#ifdef COMPAT_201
typedef struct OpenCollInp {
    char collName[MAX_NAME_LEN];
    int flags;
    int dummy;
    keyValPair_t condInput;
} openCollInp_t;

#define OpenCollInp_PI "str collName[MAX_NAME_LEN]; int flags; int dummy; struct KeyValPair_PI;"
#endif

#define NUM_COLL_HANDLE	40

#if defined(RODS_SERVER)
#define RS_OPEN_COLLECTION rsOpenCollection
/* prototype for the server handler */
int
rsOpenCollection (rsComm_t *rsComm, collInp_t *openCollInp);
#else
#define RS_OPEN_COLLECTION NULL
#endif

#ifdef COMPAT_201
#if defined(RODS_SERVER)
#define RS_OPEN_COLLECTION201 rsOpenCollection201
/* prototype for the server handler */
int
rsOpenCollection201 (rsComm_t *rsComm, openCollInp_t *openCollInp);
#else
#define RS_OPEN_COLLECTION201 NULL
#endif
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */

int
rcOpenCollection (rcComm_t *conn, collInp_t *openCollInp);

/* rcOpenCollection - Open a iRODS collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   openCollInp_t *collInp - generic coll input. Relevant items are:
 *      collName - the collection to be registered.
 *      flags - supports LONG_METADATA_FG, VERY_LONG_METADATA_FG
 *	  and VERY_LONG_METADATA_FG.
 *
 * OutPut -
 *   int status - status of the operation.
 */
#ifdef  __cplusplus
}
#endif

#endif	/* OPEN_COLLECTION_H */
