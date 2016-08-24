#ifndef RS_DATA_OBJ_TRUNCATE_HPP
#define RS_DATA_OBJ_TRUNCATE_HPP

#include "rodsConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

int rsDataObjTruncate( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int _rsDataObjTruncate( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *dataObjInfoHead );
int dataObjTruncateS( rsComm_t *rsComm, dataObjInp_t *dataObjTruncateInp, dataObjInfo_t *dataObjInfo );
int l3Truncate( rsComm_t *rsComm, dataObjInp_t *dataObjTruncateInp, dataObjInfo_t *dataObjInfo );

#endif
