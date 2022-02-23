#ifndef DATA_PUT_H__
#define DATA_PUT_H__

#include "irods/rcConnect.h"
#include "irods/dataObjInpOut.h"

#ifdef __cplusplus
extern "C"
#endif
int rcDataPut( rcComm_t *conn, dataOprInp_t *dataPutInp, portalOprOut_t **portalOprOut );

#endif
