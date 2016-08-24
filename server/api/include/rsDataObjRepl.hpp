#ifndef RS_DATA_OBJ_REPL_HPP
#define RS_DATA_OBJ_REPL_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

int rsDataObjRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp, transferStat_t **transferStat );
int _rsDataObjRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp, transferStat_t *transStat, dataObjInfo_t *outDataObjInfo );
int _rsDataObjReplUpdate( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *srcDataObjInfoHead, dataObjInfo_t *destDataObjInfoHead, transferStat_t *transStat );
int _rsDataObjReplNewCopy( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *srcDataObjInfoHead, const char* _resc_name, transferStat_t *transStat, dataObjInfo_t *outDataObjInfo );
int _rsDataObjReplS( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *srcDataObjInfo, const char *rescName, dataObjInfo_t *destDataObjInfo, int updateFlag );
int dataObjOpenForRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *srcDataObjInfo, const char* _resc_name, dataObjInfo_t *destDataObjInfo, int updateFlag );
int dataObjCopy( rsComm_t *rsComm, int l1descInx );
int l3DataCopySingleBuf( rsComm_t *rsComm, int l1descInx );
int l3DataStageSync( rsComm_t *rsComm, int l1descInx );
int l3FileSync( rsComm_t *rsComm, int srcL1descInx, int destL1descInx );
int l3FileStage( rsComm_t *rsComm, int srcL1descInx, int destL1descInx );
int _l3FileStage( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo, dataObjInfo_t *destDataObjInfo, int mode );
int rsReplAndRequeDataObjInfo( rsComm_t *rsComm, dataObjInfo_t **srcDataObjInfoHead, char *destRescName, char *flagStr );
int replToCacheRescOfCompObj( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *srcDataObjInfoHead, dataObjInfo_t *compObjInfo, dataObjInfo_t *oldDataObjInfo, dataObjInfo_t **outDestDataObjInfo );
int stageBundledData( rsComm_t *rsComm, dataObjInfo_t **subfileObjInfoHead );
int unbunAndStageBunfileObj( rsComm_t *rsComm, char *bunfileObjPath, char **outCacheRescName );
int _unbunAndStageBunfileObj( rsComm_t *rsComm, dataObjInfo_t **bunfileObjInfoHead, keyValPair_t* condInput, char **outCacheRescName, int rmBunCopyFlag );
int stageDataFromCompToCache( rsComm_t *rsComm, dataObjInfo_t *compObjInfo, dataObjInfo_t *outCacheObjInfo );
int stageAndRequeDataToCache( rsComm_t *rsComm, dataObjInfo_t **compObjInfoHead );
int stageBundledData( rsComm_t *rsComm, dataObjInfo_t **subfileObjInfoHead );

#endif
