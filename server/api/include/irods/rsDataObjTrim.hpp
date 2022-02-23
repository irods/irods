#ifndef RS_DATA_OBJ_TRIM_HPP
#define RS_DATA_OBJ_TRIM_HPP

#include "irods/rcConnect.h"
#include "irods/dataObjInpOut.h"

#define DEF_MIN_COPY_CNT        2

int rsDataObjTrim( rsComm_t *rsComm, dataObjInp_t *dataObjInp );

#endif
