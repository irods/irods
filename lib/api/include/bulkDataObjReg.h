#ifndef BULK_DATA_OBJ_REG_H__
#define BULK_DATA_OBJ_REG_H__

#include "objInfo.h"
#include "rodsGenQuery.h"
#include "rcConnect.h"

/* definition for opreration type */
#define OPR_TYPE_INX    999999
#define OFFSET_INX      999998
#define REGISTER_OPR    "register"
#define MODIFY_OPR      "modify"


/* rcBulkDataObjReg - Bulk Reg of iRODS data objects.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   genQueryOut_t *bulkDataObjRegInp - generic arrays of metadata including
 *      COL_DATA_NAME, COL_DATA_SIZE, COL_DATA_TYPE_NAME, COL_D_RESC_NAME,
 *      COL_D_DATA_PATH and OPR_TYPE_INX.
 *
 * OutPut -
 *    genQueryOut_t *bulkDataObjRegOut - arrays of metadata for COL_D_DATA_ID.
 * Return -
 *   int status - The status of the operation.
 */


#ifdef __cplusplus
extern "C"
#endif
int rcBulkDataObjReg( rcComm_t *conn, genQueryOut_t *bulkDataObjRegInp, genQueryOut_t **bulkDataObjRegOut );

#endif
