#ifndef RS_GET_TEMP_PASSWORD_HPP
#define RS_GET_TEMP_PASSWORD_HPP

#include "rcConnect.h"
#include "getTempPassword.h"

int rsGetTempPassword( rsComm_t *rsComm, getTempPasswordOut_t **getTempPasswordOut );
int _rsGetTempPassword( rsComm_t *rsComm, getTempPasswordOut_t **getTempPasswordOut );

#endif
