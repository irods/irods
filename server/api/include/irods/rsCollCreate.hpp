#ifndef RS_COLL_CREATE_HPP
#define RS_COLL_CREATE_HPP

#include "irods/rodsConnect.h"
#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"

int rsCollCreate( rsComm_t *rsComm, collInp_t *collCreateInp );
int remoteCollCreate( rsComm_t *rsComm, collInp_t *collCreateInp );
int l3Mkdir( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo );

#endif
