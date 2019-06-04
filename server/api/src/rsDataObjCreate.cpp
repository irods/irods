/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjCreate.h for a description of this API call.*/

#include "rcMisc.h"
#include "rsGlobalExtern.hpp"
#include "objInfo.h"
#include "rsDataObjOpen.hpp"

int
rsDataObjCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    dataObjInp->openFlags |= O_CREAT | O_RDWR;
    return rsDataObjOpen(rsComm, dataObjInp);
}

