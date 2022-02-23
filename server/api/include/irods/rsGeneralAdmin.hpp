#ifndef RS_GENERAL_ADMIN_HPP
#define RS_GENERAL_ADMIN_HPP

#include "irods/rcConnect.h"
#include "irods/generalAdmin.h"

int rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp );
int _rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp );

#endif
