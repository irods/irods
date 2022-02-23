#ifndef RS_GET_HOST_FOR_GET_HPP
#define RS_GET_HOST_FOR_GET_HPP

#include "irods/rcConnect.h"
#include "irods/dataObjClose.h"
#include "irods/getHostForGet.h"

int rsGetHostForGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp, char **outHost );

#endif
