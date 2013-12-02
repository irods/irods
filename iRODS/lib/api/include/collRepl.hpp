/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* collRepl.h - recursively replicate a collection
 */

#ifndef COLL_REPL_H
#define COLL_REPL_H

/* This is a Object File I/O API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjWrite.h"
#include "dataObjClose.h"
#include "dataCopy.h"

#if defined(RODS_SERVER)
#define RS_COLL_REPL rsCollRepl
/* prototype for the server handler */
int
rsCollRepl (rsComm_t *rsComm, collInp_t *collReplInp, 
collOprStat_t **collOprStat);
#else
#define RS_COLL_REPL NULL
#endif

#ifdef COMPAT_201
#if defined(RODS_SERVER)
#define RS_COLL_REPL201 rsCollRepl201
/* prototype for the server handler */
int
rsCollRepl201 (rsComm_t *rsComm, dataObjInp_t *collReplInp,
collOprStat_t **collOprStat);
#else
#define RS_COLL_REPL201 NULL
#endif
#endif  /* COMPAT_201 */

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcCollRepl (rcComm_t *conn, collInp_t *collReplInp, int vFlag);
int
_rcCollRepl (rcComm_t *conn, collInp_t *collReplInp,
collOprStat_t **collOprStat);

#ifdef  __cplusplus
}
#endif

#endif	/* COLL_REPL_H */
