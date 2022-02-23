#ifndef RS_GET_TEMP_PASSWORD_FOR_OTHER_HPP
#define RS_GET_TEMP_PASSWORD_FOR_OTHER_HPP

#include "irods/rcConnect.h"
#include "irods/getTempPasswordForOther.h"

int rsGetTempPasswordForOther( rsComm_t *rsComm, getTempPasswordForOtherInp_t *getTempPasswordForOtherInp, getTempPasswordForOtherOut_t **getTempPasswordForOtherOut );
int _rsGetTempPasswordForOther( rsComm_t *rsComm, getTempPasswordForOtherInp_t *getTempPasswordForOtherInp, getTempPasswordForOtherOut_t **getTempPasswordForOtherOut );

#endif
