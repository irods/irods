#ifndef RS_DATA_OBJ_LSEEK_HPP
#define RS_DATA_OBJ_LSEEK_HPP

#include "rodsConnect.h"
#include "dataObjInpOut.h"
#include "rodsType.h"
#include "fileLseek.h"

int rsDataObjLseek( rsComm_t *rsComm, openedDataObjInp_t *dataObjLseekInp, fileLseekOut_t **dataObjLseekOut );
rodsLong_t _l3Lseek( rsComm_t *rsComm, int l3descInx, rodsLong_t offset, int whence );

#endif
