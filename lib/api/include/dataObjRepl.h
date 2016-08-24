#ifndef DATA_OBJ_REPL_H__
#define DATA_OBJ_REPL_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"


/* prototype for the client call */
/* rcDataObjRepl - Replicate an iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *      condInput - conditional Input
 *          ALL_KW - update all copies.
 *          DATA_TYPE_KW - "value" = the data type of the file.
 *          REPL_NUM_KW  - "value" = The replica number to use as source
 *              copy. (optional)
 *          RESC_NAME_KW - "value" = The source Resource (optional).
 *          DEST_RESC_NAME_KW - "value" = The destination Resource.
 *          ADMIN_KW - Admin removing other users' files. Only files
 *              in trash can be removed.
 *          BACKUP_RESC_NAME_KW - backup resource (backup mode).
 *   return value - The status of the operation.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcDataObjRepl( rcComm_t *conn, dataObjInp_t *dataObjInp );
int _rcDataObjRepl( rcComm_t *conn, dataObjInp_t *dataObjInp, transferStat_t **transferStat );

#endif
