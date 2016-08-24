#ifndef GENERAL_UPDATE_H__
#define GENERAL_UPDATE_H__

#include "rcConnect.h"
#include "rodsGeneralUpdate.h"

#ifdef __cplusplus
extern "C"
#endif
int rcGeneralUpdate( rcComm_t *conn, generalUpdateInp_t *generalUpdateInp );

#endif
