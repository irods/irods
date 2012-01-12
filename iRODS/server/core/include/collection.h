/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* collection.h - header file for collection.c
 */



#ifndef COLLECTION_H
#define COLLECTION_H

#include "rods.h"
#include "initServer.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "ruleExecSubmit.h"
#include "rcGlobalExtern.h"
#include "rsGlobalExtern.h"
#include "reIn2p3SysRule.h"

int
checkCollAccessPerm (rsComm_t *rsComm, char *collection, char *accessPerm);
int
rsQueryDataObjInCollReCur (rsComm_t *rsComm, char *collection,
genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut, char *accessPerm,
int singleFlag);
int
rsQueryCollInColl (rsComm_t *rsComm, char *collection,
genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut);
int
isCollEmpty (rsComm_t *rsComm, char *collection);
int
collStat (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rodsObjStat_t **rodsObjStatOut);
int
collStatAllKinds (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rodsObjStat_t **rodsObjStatOut);
int
rsMkCollR (rsComm_t *rsComm, char *startColl, char *destColl);
#endif	/* COLLECTION_H */
