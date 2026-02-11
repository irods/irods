#ifndef IRODS_DATA_OBJ_LOCK_H
#define IRODS_DATA_OBJ_LOCK_H

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"

// All macros defined by this header file are deprecated as of 4.3.5.

// lock type definition
#define READ_LOCK_TYPE   "readLockType"
#define WRITE_LOCK_TYPE  "writeLockType"
#define UNLOCK_TYPE      "unlockType"

// lock command definition
#define SET_LOCK_CMD      "setLockCmd"
#define SET_LOCK_WAIT_CMD "setLockWaitCmd"
#define GET_LOCK_CMD      "getLockCmd"


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
#ifdef __cplusplus
extern "C"
#endif
__attribute__((deprecated("Logical locking now manages access to data objects.")))
int rcDataObjLock( rcComm_t *conn, dataObjInp_t *dataObjInp );
#ifdef __cplusplus
extern "C"
#endif
__attribute__((deprecated("Logical locking now manages access to data objects.")))
int rcDataObjUnlock( rcComm_t *conn, dataObjInp_t *dataObjInp );

#endif // IRODS_DATA_OBJ_LOCK_H
