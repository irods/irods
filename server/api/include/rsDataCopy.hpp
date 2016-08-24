#ifndef RS_DATA_COPY_HPP
#define RS_DATA_COPY_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "dataCopy.h"
#include "rodsConnect.h"

int rsDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );
int remoteDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp, rodsServerHost_t *rodsServerHost );
int _rsDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );

#endif
