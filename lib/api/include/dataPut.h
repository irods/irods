#ifndef DATA_PUT_H__
#define DATA_PUT_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

#ifdef __cplusplus
extern "C"
#endif
int rcDataPut( rcComm_t *conn, dataOprInp_t *dataPutInp, portalOprOut_t **portalOprOut );

#endif
