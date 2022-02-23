#ifndef RS_REG_COLL_HPP
#define RS_REG_COLL_HPP

#include "irods/rcConnect.h"
#include "irods/dataObjInpOut.h"

int rsRegColl( rsComm_t *rsComm, collInp_t *regCollInp );
int _rsRegColl( rsComm_t *rsComm, collInp_t *regCollInp );

#endif
