#ifndef RS_GENERAL_ADMIN_HPP
#define RS_GENERAL_ADMIN_HPP

/// \file

#include "generalAdmin.h"

struct RsComm;

int rsGeneralAdmin(RsComm* rsComm, GeneralAdminInp* generalAdminInp);

int _rsGeneralAdmin(RsComm* rsComm, GeneralAdminInp* generalAdminInp);

#endif // RS_GENERAL_ADMIN_HPP

