#ifndef RS_DATA_PUT_HPP
#define RS_DATA_PUT_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"

int rsDataPut( rsComm_t *rsComm, dataOprInp_t *dataPutInp, portalOprOut_t **portalOprOut );
int remoteDataPut( rsComm_t *rsComm, dataOprInp_t *dataPutInp, portalOprOut_t **portalOprOut, rodsServerHost_t *rodsServerHost );
int _rsDataPut( rsComm_t *rsComm, dataOprInp_t *dataPutInp, portalOprOut_t **portalOprOut );

#endif
