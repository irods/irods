#ifndef IRODS_GENERAL_UPDATE_H
#define IRODS_GENERAL_UPDATE_H

#include "irods/rcConnect.h"
#include "irods/rodsGeneralUpdate.h"

#ifdef __cplusplus
extern "C" {
#endif

int rcGeneralUpdate(rcComm_t* conn, generalUpdateInp_t* generalUpdateInp)
    __attribute__((deprecated("GeneralUpdate is deprecated. Its use should be avoided.")));

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GENERAL_UPDATE_H
