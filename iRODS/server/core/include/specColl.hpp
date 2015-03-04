/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* specColl.h - header file for specColl.c
 */



#ifndef SPEC_COLL_HPP
#define SPEC_COLL_HPP

#include "rods.hpp"
#include "objInfo.hpp"
#include "dataObjInpOut.hpp"
#include "ruleExecSubmit.hpp"
#include "rcGlobalExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "reIn2p3SysRule.hpp"

extern "C" {

    int
    modCollInfo2( rsComm_t *rsComm, specColl_t *specColl, int clearFlag );
    int
    querySpecColl( rsComm_t *rsComm, char *objPath, genQueryOut_t **genQueryOut );
    int
    queueSpecCollCache( rsComm_t *rsComm, genQueryOut_t *genQueryOut, char *objPath ); // JMC - backport 4680
    int
    queueSpecCollCacheWithObjStat( rodsObjStat_t *rodsObjStatOut );
    specCollCache_t *
    matchSpecCollCache( char *objPath );
    int
    getSpecCollCache( rsComm_t *rsComm, char *objPath, int inCachOnly,
                      specCollCache_t **specCollCache );
    int
    statPathInSpecColl( rsComm_t *rsComm, char *objPath,
                        int inCachOnly, rodsObjStat_t **rodsObjStatOut );
    int
    specCollSubStat( rsComm_t *rsComm, specColl_t *specColl,
                     char *subPath, specCollPerm_t specCollPerm, dataObjInfo_t **dataObjInfo );
    int
    resolvePathInSpecColl( rsComm_t *rsComm, char *objPath,
                           specCollPerm_t specCollPerm, int inCachOnly, dataObjInfo_t **dataObjInfo );
    int
    resolveLinkedPath( rsComm_t *rsComm, char *objPath,
                       specCollCache_t **specCollCache, keyValPair_t *condInput );

}

#endif	/* SPEC_COLL_H */
