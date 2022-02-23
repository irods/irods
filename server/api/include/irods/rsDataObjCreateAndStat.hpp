#ifndef RS_DATA_OBJ_CREATE_AND_STAT_HPP
#define RS_DATA_OBJ_CREATE_AND_STAT_HPP

#include "irods/rcConnect.h"
#include "irods/dataObjInpOut.h"
#include "irods/dataObjOpenAndStat.h"

int rsDataObjCreateAndStat( rsComm_t *rsComm, dataObjInp_t *dataObjInp, openStat_t **openStat );

#endif
