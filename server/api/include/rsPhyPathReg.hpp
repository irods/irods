#ifndef RS_PHY_PATH_REG_HPP
#define RS_PHY_PATH_REG_HPP

#include "rodsConnect.h"
#include "dataObjInpOut.h"

int rsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int phyPathRegNoChkPerm( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int irsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int remotePhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, rodsServerHost_t *rodsServerHost );
int _rsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, const char *_resc_name, rodsServerHost_t *rodsServerHost );
int filePathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, const char *_resc_name );
int filePathRegRepl( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath, const char *_resc_name );
int dirPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath, const char *_resc_name );
int mountFileDir( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath, const char *rescVaultPath );
int structFileReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int unmountFileDir( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int structFileSupport( rsComm_t *rsComm, char *collection, char *collType, char* );
int linkCollReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );

#endif
