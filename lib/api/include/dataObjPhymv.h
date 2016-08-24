#ifndef DATA_OBJ_PHYMV_H__
#define DATA_OBJ_PHYMV_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

/* rcDataObjPhymv - Move an iRODS data object from one resource to another.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object to be moved.
 *      condInput - condition input (optional).
 *          ADMIN_KW - Admin moving other users' files.
 *          REPL_NUM_KW  - "value" = The replica number of the copy to
 *              be moved.
 *          RESC_NAME_KW - "value" = The resource of the physical data to
 *              be moved.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource of the move.
 *
 * OutPut -
 *   int status of the operation - >= 0 ==> success, < 0 ==> failure.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcDataObjPhymv( rcComm_t *conn, dataObjInp_t *dataObjInp );
int _rcDataObjPhymv( rcComm_t *conn, dataObjInp_t *dataObjInp, transferStat_t **transferStat );

#endif
