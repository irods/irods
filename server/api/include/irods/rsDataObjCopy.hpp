#ifndef RS_DATA_OBJ_COPY_HPP
#define RS_DATA_OBJ_COPY_HPP

#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"
#include "irods/rcConnect.h"
#include "irods/dataObjCopy.h"

int rsDataObjCopy( rsComm_t *rsComm, dataObjCopyInp_t *dataObjCopyInp, transferStat_t **transferStat );

#endif
