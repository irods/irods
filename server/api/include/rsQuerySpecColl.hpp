#ifndef RS_QUERY_SPEC_COLL_HPP
#define RS_QUERY_SPEC_COLL_HPP

#include "rodsConnect.h"
#include "rodsType.h"
#include "querySpecColl.h"

int rsQuerySpecColl( rsComm_t *rsComm, dataObjInp_t *querySpecCollInp, genQueryOut_t **genQueryOut );
int _rsQuerySpecColl( rsComm_t *rsComm, int specCollInx, dataObjInp_t *dataObjInp, genQueryOut_t *genQueryOut, int continueFlag );
int initOutForQuerySpecColl( genQueryOut_t **genQueryOut );
int l3Opendir( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo );
int specCollReaddir( rsComm_t *rsComm, int specCollInx, rodsDirent_t **rodsDirent );
int specCollClosedir( rsComm_t *rsComm, int specCollInx );
int openSpecColl( rsComm_t *rsComm, dataObjInp_t *dataObjInp, int parentInx );

#endif
