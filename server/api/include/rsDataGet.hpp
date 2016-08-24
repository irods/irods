#ifndef RS_DATA_GET_HPP
#define RS_DATA_GET_HPP

#include "rodsConnect.h"
#include "dataObjInpOut.h"

int rsDataGet( rsComm_t *rsComm, dataOprInp_t *dataGetInp, portalOprOut_t **portalOprOut );
int remoteDataGet( rsComm_t *rsComm, dataOprInp_t *dataGetInp, portalOprOut_t **portalOprOut, rodsServerHost_t *rodsServerHost );
int _rsDataGet( rsComm_t *rsComm, dataOprInp_t *dataGetInp, portalOprOut_t **portalOprOut );

#endif
