#ifndef RS_DATA_OBJ_CLOSE_HPP
#define RS_DATA_OBJ_CLOSE_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"
#include "rodsType.h"

int rsDataObjClose( rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp );
int irsDataObjClose( rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp, dataObjInfo_t **outDataObjInfo );
int _rsDataObjClose( rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp );
int l3Close( rsComm_t *rsComm, int l1descInx );
int _l3Close( rsComm_t *rsComm, int l3descInx );
int l3Stat( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, rodsStat_t **myStat );
int procChksumForClose( rsComm_t *rsComm, int l1descInx, char **chksumStr );

#endif
