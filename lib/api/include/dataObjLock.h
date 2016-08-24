#ifndef DATA_OBJ_LOCK_H__
#define DATA_OBJ_LOCK_H__

#include "rcConnect.h"
#include "objInfo.h"
#include "dataObjInpOut.h"

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
int rcDataObjLock( rcComm_t *conn, dataObjInp_t *dataObjInp );
#ifdef __cplusplus
extern "C"
#endif
int rcDataObjUnlock( rcComm_t *conn, dataObjInp_t *dataObjInp );


#endif
