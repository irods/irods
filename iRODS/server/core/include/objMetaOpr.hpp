/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* objMetaOpr.h - header file for objMetaOpr.c
 */



#ifndef OBJ_META_OPR_HPP
#define OBJ_META_OPR_HPP

#include "rods.hpp"
#include "objInfo.hpp"
#include "dataObjInpOut.hpp"
#include "rcGlobalExtern.hpp"
#include "rsGlobalExtern.hpp"

extern "C" {

    int
    svrCloseQueryOut( rsComm_t *rsComm, genQueryOut_t *genQueryOut );
    int
    isData( rsComm_t *rsComm, char *objName, rodsLong_t *dataId );
    int
    isColl( rsComm_t *rsComm, char *objName, rodsLong_t *collId );
    int
    isCollAllKinds( rsComm_t *rsComm, char *objName, rodsLong_t *collId );
    int
    isUser( rsComm_t *rsComm, char *objName );
    int
    isResc( rsComm_t *rsComm, char *objName );
    int
    isMeta( rsComm_t *rsComm, char *objName );
    int
    isToken( rsComm_t *rsComm, char *objName );
    int
    getObjType( rsComm_t *rsComm, char *objName, char * objType );
    int
    addAVUMetadataFromKVPairs( rsComm_t *rsComm, char *objName, char *inObjType,
                               keyValPair_t *kVP );
    int
    setAVUMetadataFromKVPairs( rsComm_t *rsComm, char *objName, char *inObjType, // JMC - backport 4836
                               keyValPair_t *kVP );
    int
    removeAVUMetadataFromKVPairs( rsComm_t *rsComm, char *objName, char *inObjType,
                                  keyValPair_t *kVP );
    int
    getStructFileType( specColl_t *specColl );

    extern int
    checkPermissionByObjType( rsComm_t *rsComm, char *objName, char *objType, char *user, char *zone, char *oper );
    int
    checkDupReplica( rsComm_t *rsComm, rodsLong_t dataId, char *rescName, char *filePath ); // JMC - backport 4497
    int
    getNumSubfilesInBunfileObj( rsComm_t *rsComm, char *objPath ); // JMC - backport 4552
    int
    getPhyPath( rsComm_t *rsComm, char *objName,  char *resource, char *phyPath, char* rescHier ); // JMC - backport 4680
}

#endif	/* OBJ_META_OPR_H */
