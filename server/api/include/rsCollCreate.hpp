#ifndef RS_COLL_CREATE_HPP
#define RS_COLL_CREATE_HPP

#include "rodsConnect.h"
#include "objInfo.h"
#include "dataObjInpOut.h"

int rsCollCreate( rsComm_t *rsComm, collInp_t *collCreateInp );
int remoteCollCreate( rsComm_t *rsComm, collInp_t *collCreateInp );
int l3Mkdir( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo );

#endif
