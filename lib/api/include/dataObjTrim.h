#ifndef DATA_OBJ_TRIM_H__
#define DATA_OBJ_TRIM_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

/* prototype for the client call */
/* rcDataObjTrim - Trim the copies (replica) of an iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *      condInput - conditional Input
 *          COPIES_KW - The number of copies to retain. Default is 2.
 *          REPL_NUM_KW  - "value" = The replica number to trim.
 *          RESC_NAME_KW - "value" = The Resource to trim.
 *          ADMIN_KW - Admin trim other users' files.
 *   return value - The status of the operation.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjTrim( rcComm_t *conn, dataObjInp_t *dataObjInp );

#endif
