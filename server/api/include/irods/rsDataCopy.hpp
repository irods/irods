#ifndef RS_DATA_COPY_HPP
#define RS_DATA_COPY_HPP

#include "irods/rcConnect.h"
#include "irods/dataObjInpOut.h"
#include "irods/dataCopy.h"
#include "irods/rodsConnect.h"

int rsDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );
int remoteDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp, rodsServerHost_t *rodsServerHost );
int _rsDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );

#endif
