#ifndef DATA_GET_H__
#define DATA_GET_H__

#include "irods/rcConnect.h"
#include "irods/dataObjInpOut.h"

int rcDataGet( rcComm_t *conn, dataOprInp_t *dataGetInp, portalOprOut_t **portalOprOut );

#endif
