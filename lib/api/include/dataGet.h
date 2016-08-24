#ifndef DATA_GET_H__
#define DATA_GET_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

int rcDataGet( rcComm_t *conn, dataOprInp_t *dataGetInp, portalOprOut_t **portalOprOut );

#endif
