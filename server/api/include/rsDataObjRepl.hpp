#ifndef RS_DATA_OBJ_REPL_HPP
#define RS_DATA_OBJ_REPL_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

int rsDataObjRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp, transferStat_t **transferStat );
int dataObjCopy( rsComm_t *rsComm, int l1descInx );
int replToCacheRescOfCompObj( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *srcDataObjInfoHead, dataObjInfo_t *compObjInfo, dataObjInfo_t *oldDataObjInfo, dataObjInfo_t **outDestDataObjInfo );
int unbunAndStageBunfileObj(rsComm_t* rsComm, const char* bunfileObjPath, char** outCacheRescName);
int _unbunAndStageBunfileObj( rsComm_t *rsComm, dataObjInfo_t **bunfileObjInfoHead, keyValPair_t* condInput, char **outCacheRescName, int rmBunCopyFlag );
int stageDataFromCompToCache( rsComm_t *rsComm, dataObjInfo_t *compObjInfo, dataObjInfo_t *outCacheObjInfo );
int stageAndRequeDataToCache( rsComm_t *rsComm, dataObjInfo_t **compObjInfoHead );

#endif
