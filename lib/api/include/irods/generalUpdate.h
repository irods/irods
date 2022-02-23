#ifndef GENERAL_UPDATE_H__
#define GENERAL_UPDATE_H__

#include "irods/rcConnect.h"
#include "irods/rodsGeneralUpdate.h"

#ifdef __cplusplus
extern "C"
#endif
int rcGeneralUpdate( rcComm_t *conn, generalUpdateInp_t *generalUpdateInp );

#endif
