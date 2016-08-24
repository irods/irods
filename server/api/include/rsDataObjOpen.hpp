#ifndef RS_DATA_OBJ_OPEN_HPP
#define RS_DATA_OBJ_OPEN_HPP

#include "rodsConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

int rsDataObjOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int _rsDataObjOpenWithObjInfo( rsComm_t *rsComm, dataObjInp_t *dataObjInp, int phyOpenFlag, dataObjInfo_t *dataObjInfo );
int _rsDataObjOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *dataObjInfoHead );
int dataOpen( rsComm_t *rsComm, int l1descInx );
int l3Open( rsComm_t *rsComm, int l1descInx );
int _l3Open( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, int mode, int flags );
int l3OpenByHost( rsComm_t *rsComm, int l3descInx, int flags );
int applyPreprocRuleForOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t **dataObjInfoHead );
int createEmptyRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t **dataObjInfoHead );
int procDataObjOpenForWrite( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t **dataObjInfoHead, dataObjInfo_t **compDataObjInfo );

#endif
