/* To be copied to lib/api/include/, and #included in
 * lib/api/include/apiHeaderAll.h */

#ifndef DATA_OBJ_FSYNC_H
#define DATA_OBJ_FSYNC_H

#include "rods.h"
#include "procApiRequest.h"

#if defined(RODS_SERVER)
#define RS_DATA_OBJ_FSYNC rsDataObjFsync

/* Perform the low-level file I/O operations.  Basically calls the
 * rsFileFsync function with a bit of error checking.
 * @param rsComm Server connection handle.
 * @param l1descInx handle for the file in the file system driver.
 * @return negative on failure. */
int
l3Fsync (rsComm_t *rsComm, int l1descInx);

/* Perform the low-level file I/O operations (by calling l3Fsync),
 * and update system metadata to reflect current reality.
 * @param rsComm Server connection handle.
 * @param dataObjFsyncInp descriptor of data object to be flushed.
 *    Only l1descInx is used.
 * @return negative on failure. */
int
_rsDataObjFsync (rsComm_t *rsComm, openedDataObjInp_t *dataObjFsyncInp);

/* Flush buffers used for writing this data object to disk (or
 * wherever their final destination may be, depending on the resource
 * type).
 * @param rsComm_t *rsComm Server connection handle.
 * @param dataObjFsyncInp descriptor of data object to be flushed.
 *    Only l1descInx is used.
 * @return negative on failure. */
int
rsDataObjFsync (rsComm_t *rsComm, openedDataObjInp_t *dataObjFsyncInp);

#else
#define RS_DATA_OBJ_FSYNC NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* Flush buffers used for writing this data object to disk (or
 * wherever their final destination may be, depending on the resource
 * type).
 * @param rcComm_t *conn Client connection handle.
 * @param dataObjFsyncInp descriptor of data object to be flushed.
 *    Only l1descInx is used.
 * @return negative on failure. */
int
rcDataObjFsync (rcComm_t *conn, openedDataObjInp_t *dataObjFsyncInp);

#ifdef  __cplusplus
}
#endif

#endif /* DATA_OBJ_FSYNC_H */
