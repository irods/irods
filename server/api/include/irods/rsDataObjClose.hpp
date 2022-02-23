#ifndef RS_DATA_OBJ_CLOSE_HPP
#define RS_DATA_OBJ_CLOSE_HPP

#include "irods/rcConnect.h"
#include "irods/dataObjInpOut.h"
#include "irods/objInfo.h"
#include "irods/rodsType.h"

int rsDataObjClose( rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp );
int l3Close( rsComm_t *rsComm, int l1descInx );

#endif
