#ifndef RS_DATA_OBJ_CREATE_HPP
#define RS_DATA_OBJ_CREATE_HPP

#include "objInfo.h"
#include "dataObjInpOut.h"
#include "rcConnect.h"
#include <string>


int rsDataObjCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int _rsDataObjCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int specCollSubCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int dataObjCreateAndReg( rsComm_t *rsComm, int l1descInx );
int dataCreate( rsComm_t *rsComm, int l1descInx );
int l3Create( rsComm_t *rsComm, int l1descInx );
int l3CreateByObjInfo( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *dataObjInfo );
int _rsDataObjCreateWithResc( rsComm_t *rsComm, dataObjInp_t *dataObjInp, const std::string& _resc_name );
int getRescForCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp, std::string& _resc_name );

#endif
