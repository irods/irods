/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* collection.h - header file for collection.c
 */



#ifndef COLLECTION_HPP
#define COLLECTION_HPP

#include "irods/rods.h"
#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"
#include "irods/ruleExecSubmit.h"
#include "irods/rcGlobalExtern.h"
//#include "irods/rsGlobalExtern.hpp"
//#include "irods/reIn2p3SysRule.hpp"

int
checkCollAccessPerm( rsComm_t *rsComm, char *collection, char *accessPerm );
int
rsQueryDataObjInCollReCur( rsComm_t *rsComm, char *collection,
                           genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut, char *accessPerm,
                           int singleFlag );
int
rsQueryCollInColl( rsComm_t *rsComm, char *collection,
                   genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut );
int
isCollEmpty( rsComm_t *rsComm, char *collection );
int
collStat( rsComm_t *rsComm, const dataObjInp_t *dataObjInp,
          rodsObjStat_t **rodsObjStatOut );
int
collStatAllKinds( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                  rodsObjStat_t **rodsObjStatOut );
int
rsMkCollR( rsComm_t *rsComm, const char *startColl, const char *destColl );
#endif	/* COLLECTION_H */
