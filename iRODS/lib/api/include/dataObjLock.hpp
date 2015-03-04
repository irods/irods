/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjLock.h
 */

#ifndef DATA_OBJ_LOCK_HPP
#define DATA_OBJ_LOCK_HPP

/* This is a Object File I/O API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"

/* lock type definition */
#define READ_LOCK_TYPE "readLockType"
#define WRITE_LOCK_TYPE        "writeLockType"
#define UNLOCK_TYPE    "unlockType"

/* lock command definition */
#define SET_LOCK_CMD   "setLockCmd"
#define SET_LOCK_WAIT_CMD "setLockWaitCmd"
#define GET_LOCK_CMD   "getLockCmd"

#if defined(RODS_SERVER)
#define RS_DATA_OBJ_LOCK rsDataObjLock
#define RS_DATA_OBJ_UNLOCK rsDataObjUnlock
/* prototype for the server handler */
int
rsDataObjLock( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int
_rsDataObjLock( dataObjInp_t *dataObjInp );
int
getLockCmdAndType( keyValPair_t *condInput, int *cmd, int *type );
int
rsDataObjUnlock( rsComm_t *rsComm, dataObjInp_t *dataObjInp ); // JMC - backport 4604
int
_rsDataObjUnlock( dataObjInp_t *dataObjInp );

#else
#define RS_DATA_OBJ_LOCK NULL
#define RS_DATA_OBJ_UNLOCK NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
/* rcDataObjLock - Lock a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *     objPath - the path of the data object.
 *     condInput - condition input (optional).
 * OutPut -
 *   int the file descriptor of the locked object - an integer descriptor.
 */

int
rcDataObjLock( rcComm_t *conn, dataObjInp_t *dataObjInp );
int
rcDataObjUnlock( rcComm_t *conn, dataObjInp_t *dataObjInp );

#ifdef __cplusplus
}
#endif
#endif /* DATA_OBJ_LOCK_H */

