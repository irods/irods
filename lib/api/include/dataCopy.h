#ifndef DATA_COPY_H__
#define DATA_COPY_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

typedef struct DataCopyInp {
    dataOprInp_t dataOprInp;
    portalOprOut_t portalOprOut;
} dataCopyInp_t;
#define DataCopyInp_PI "struct DataOprInp_PI; struct PortalOprOut_PI;"


int rcDataCopy( rcComm_t *conn, dataCopyInp_t *dataCopyInp );

#endif
