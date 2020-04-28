#include "rcMisc.h"
#include "rsGlobalExtern.hpp"
#include "objInfo.h"
#include "rsDataObjOpen.hpp"

int rsDataObjCreate(rsComm_t *rsComm, dataObjInp_t *dataObjInp) {
    dataObjInp->openFlags |= O_CREAT | O_RDWR;
    return rsDataObjOpen(rsComm, dataObjInp);
} // rsDataObjCreate

