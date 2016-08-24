#ifndef DATA_OBJ_UNLINK_H__
#define DATA_OBJ_UNLINK_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

/* rcDataObjUnlink - Unlink a iRODS data object. By defult, the file will
 * be moved to the trash, but the FORCE_FLAG_KW will force the removal.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *	objPath - the path of the data object.
 *      condInput - conditional Input
 *          FORCE_FLAG_KW - remove data object instead of moving it to trash.
 *          REPL_NUM_KW  - "value" = The replica number of the copy to
 *              remove.
 *
 * OutPut -
 *   int status - The status of the operation.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjUnlink( rcComm_t *conn, dataObjInp_t *dataObjUnlinkInp );

#endif
