#ifndef RS_DATA_OBJ_CREATE_HPP
#define RS_DATA_OBJ_CREATE_HPP

#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"
#include "irods/rcConnect.h"
#include <string>

int rsDataObjCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp );

#endif
