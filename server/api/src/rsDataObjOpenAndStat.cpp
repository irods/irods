/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjOpen.h for a description of this API call.*/

#include "dataObjOpen.h"
#include "dataObjOpenAndStat.h"
#include "rodsLog.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rsDataObjOpen.hpp"

int rsDataObjOpenAndStat(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    openStat_t **openStat ) {

    if (!dataObjInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    int l1descInx = rsDataObjOpen( rsComm, dataObjInp );
    if ( l1descInx < 0 ) {
        *openStat = NULL;
        return l1descInx;
    }

    *openStat = (openStat_t*)malloc(sizeof(openStat_t));
    bzero(*openStat, sizeof(openStat_t));
    (*openStat)->dataSize = L1desc[l1descInx].dataObjInfo->dataSize;
    rstrcpy((*openStat)->dataMode, L1desc[l1descInx].dataObjInfo->dataMode, SHORT_STR_LEN);
    rstrcpy((*openStat)->dataType, L1desc[l1descInx].dataObjInfo->dataType, NAME_LEN);
    (*openStat)->l3descInx = L1desc[l1descInx].l3descInx;
    (*openStat)->replStatus = L1desc[l1descInx].replStatus;
    (*openStat)->replNum = L1desc[l1descInx].dataObjInfo->replNum;
    return l1descInx;
}
