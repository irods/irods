#ifndef RS_RM_COLL_HPP
#define RS_RM_COLL_HPP

#include "rcConnect.h"
#include "objInfo.h"
#include "dataObjInpOut.h"

int rsRmColl( rsComm_t *rsComm, collInp_t *rmCollInp, collOprStat_t **collOprStat );
int _rsRmColl( rsComm_t *rsComm, collInp_t *rmCollInp, collOprStat_t **collOprStat );
int svrUnregColl( rsComm_t *rsComm, collInp_t *rmCollInp );
int _rsRmCollRecur( rsComm_t *rsComm, collInp_t *rmCollInp, collOprStat_t **collOprStat );
int _rsPhyRmColl( rsComm_t *rsComm, collInp_t *rmCollInp, dataObjInfo_t *dataObjInfo, collOprStat_t **collOprStat );
int rsMvCollToTrash( rsComm_t *rsComm, collInp_t *rmCollInp );
int rsMkTrashPath( rsComm_t *rsComm, char *objPath, char *trashPath );
int l3Rmdir( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo );

#endif
