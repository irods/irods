#ifndef IRODS_RS_GENERAL_UPDATE_HPP
#define IRODS_RS_GENERAL_UPDATE_HPP

#include "irods/rcConnect.h"
#include "irods/rodsGeneralUpdate.h"

int rsGeneralUpdate(rsComm_t* rsComm, generalUpdateInp_t* generalUpdateInp)
    __attribute__((deprecated("GeneralUpdate is deprecated. Its use should be avoided.")));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
int _rsGeneralUpdate( generalUpdateInp_t *generalUpdateInp );
#pragma GCC diagnostic pop

#endif // IRODS_RS_GENERAL_UPDATE_HPP
