#ifndef RS_MOD_ACCESS_CONTROL_HPP
#define RS_MOD_ACCESS_CONTROL_HPP

#include "rodsConnect.h"
#include "modAccessControl.h"

int rsModAccessControl( rsComm_t *rsComm, modAccessControlInp_t *modAccessControlInp );
int _rsModAccessControl( rsComm_t *rsComm, modAccessControlInp_t *modAccessControlInp );

#endif
