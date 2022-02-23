#include "irods/rcMisc.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/objInfo.h"
#include "irods/rsDataObjOpen.hpp"

int rsDataObjCreate(rsComm_t *rsComm, dataObjInp_t *dataObjInp) {
    dataObjInp->openFlags |= (O_CREAT | O_RDWR | O_TRUNC);
    return rsDataObjOpen(rsComm, dataObjInp);
} // rsDataObjCreate

