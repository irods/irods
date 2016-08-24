#ifndef RS_PHY_BUNDLE_COLL_HPP
#define RS_PHY_BUNDLE_COLL_HPP

#include "rodsConnect.h"
#include "phyBundleColl.h"
#include <string>

int rsPhyBundleColl( rsComm_t *rsComm, structFileExtAndRegInp_t *phyBundleCollInp );
int remotePhyBundleColl( rsComm_t *rsComm, structFileExtAndRegInp_t *phyBundleCollInp, rodsServerHost_t *rodsServerHost );
int _rsPhyBundleColl( rsComm_t *rsComm, structFileExtAndRegInp_t *phyBundleCollInp, const char *_resc_name );
int createPhyBundleDataObj( rsComm_t *rsComm, char *collection, const std::string& _resc_name, const char* rescHier, dataObjInp_t *dataObjInp, char *dataType );
int createPhyBundleDir( rsComm_t *rsComm, char *bunFilePath, char *outPhyBundleDir, char* hier );
int rsMkBundlePath( rsComm_t *rsComm, char *collection, char *outPath, int myRanNum );
int replDataObjForBundle( rsComm_t *rsComm, char *collName, char *dataName, const char *rescName, char* rescHier, char* destRescHier, int adminFlag, dataObjInfo_t *outCacheObjInfo );
int isDataObjBundled( collEnt_t *collEnt );
int setSubPhyPath( char *phyBunDir, rodsLong_t dataId, char *subBunPhyPath );
int addSubFileToDir( curSubFileCond_t *curSubFileCond, bunReplCacheHeader_t *bunReplCacheHeader );
int replAndAddSubFileToDir( rsComm_t *rsComm, curSubFileCond_t *curSubFileCond, const char *myRescName, char *phyBunDir, bunReplCacheHeader_t *bunReplCacheHeader );
int bundleAndRegSubFiles( rsComm_t *rsComm, int l1descInx, char *phyBunDir, char *collection, bunReplCacheHeader_t *bunReplCacheHeader, int chksumFlag );
int phyBundle( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char *phyBunDir, char *collection, int oprType );

#endif
