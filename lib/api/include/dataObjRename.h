#ifndef DATA_OBJ_RENAME_H__
#define DATA_OBJ_RENAME_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "dataObjCopy.h"
#include "objInfo.h"


/* prototype for the client call */
/* rcDataObjRename - Rename a iRODS data object or collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjCopyInp_t *dataObjRenameInp - Relevant items are:
 *      dataObjInp_t srcDataObjInp - The source dataObj. Relevant items are:
 *          objPath - the source object path.
 *          oprType - type of rename:
 *              RENAME_DATA_OBJ - rename a data object
 *              RENAME_COLL - rename a collection
 *              RENAME_UNKNOWN_TYPE - rename a data object or collection
 *      destDataObjInp - The destination dataObj. Relevant items are:
 *          objPath - the destination object path.
 * OutPut -
 *    int status of the operation - >= 0 ==> success, < 0 ==> failure.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcDataObjRename( rcComm_t *conn, dataObjCopyInp_t *dataObjRenameInp );

#endif
