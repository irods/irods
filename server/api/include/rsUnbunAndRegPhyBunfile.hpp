#ifndef RS_UNBUN_AND_REG_PHY_BUNFILE_HPP
#define RS_UNBUN_AND_REG_PHY_BUNFILE_HPP

#include "rodsConnect.h"
#include "objInfo.h"

int rsUnbunAndRegPhyBunfile( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int _rsUnbunAndRegPhyBunfile( rsComm_t *rsComm, dataObjInp_t *dataObjInp, const char *_resc_name );
int remoteUnbunAndRegPhyBunfile( rsComm_t *rsComm, dataObjInp_t *dataObjInp, rodsServerHost_t *rodsServerHost );
int unbunPhyBunFile( rsComm_t *rsComm, char *objPath, const char *_resc_name, char *bunFilePath, char *phyBunDir, char *dataType, int saveLinkedFles, const char* resc_hier ); // JMC _ backport 4657, 4658
int regUnbunPhySubfiles( rsComm_t *rsComm, const char *_resc_name, char *phyBunDir, int rmBunCopyFlag );
int regPhySubFile( rsComm_t *rsComm, char *subfilePath, dataObjInfo_t *bunDataObjInfo, const char *_resc_name );
int rmLinkedFilesInUnixDir( char *phyBunDir );

#endif
