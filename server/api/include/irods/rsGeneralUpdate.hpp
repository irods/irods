#ifndef RS_GENERAL_UPDATE_HPP
#define RS_GENERAL_UPDATE_HPP

#include "irods/rcConnect.h"
#include "irods/rodsGeneralUpdate.h"

int rsGeneralUpdate( rsComm_t *rsComm, generalUpdateInp_t *generalUpdateInp );
int _rsGeneralUpdate( generalUpdateInp_t *generalUpdateInp );

#endif
