#ifndef RS_USER_ADMIN_HPP
#define RS_USER_ADMIN_HPP

#include "irods/userAdmin.h"

int rsUserAdmin( rsComm_t *rsComm, userAdminInp_t *userAdminInp );
int _rsUserAdmin( rsComm_t *rsComm, userAdminInp_t *userAdminInp );

#endif
