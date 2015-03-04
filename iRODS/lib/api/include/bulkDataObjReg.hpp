/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* bulkDataObjReg.h
 */

#ifndef BULK_DATA_OBJ_REG_HPP
#define BULK_DATA_OBJ_REG_HPP

/* This is a Object File I/O API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"

/* definition for opreration type */
#define OPR_TYPE_INX    999999
#define OFFSET_INX      999998
#define REGISTER_OPR    "register"
#define MODIFY_OPR      "modify"

#if defined(RODS_SERVER)
#define RS_BULK_DATA_OBJ_REG rsBulkDataObjReg
/* prototype for the server handler */
int
rsBulkDataObjReg( rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp,
                  genQueryOut_t **bulkDataObjRegOut );
int
_rsBulkDataObjReg( rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp,
                   genQueryOut_t **bulkDataObjRegOut );
int
modDataObjSizeMeta( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                    char *strDataSize );
int
svrRegReplByDataObjInfo( rsComm_t *rsComm, dataObjInfo_t *destDataObjInfo );
#else
#define RS_BULK_DATA_OBJ_REG NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcBulkDataObjReg( rcComm_t *conn, genQueryOut_t *bulkDataObjRegInp,
                  genQueryOut_t **bulkDataObjRegOut );

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
}
#endif
#endif  /* BULK_DATA_OBJ_REG_H */
