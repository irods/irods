#ifndef PHY_PATH_REG_H__
#define PHY_PATH_REG_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

/* rcPhyPathReg - Reg a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *
 * OutPut -
 *   int status - The status of the operation.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcPhyPathReg( rcComm_t *conn, dataObjInp_t *phyPathRegInp );

#endif
