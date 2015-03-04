/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* generalAdmin.h
 */

#ifndef GENERAL_ADMIN_HPP
#define GENERAL_ADMIN_HPP

/* This is a high level file type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

typedef struct {
    const char *arg0;
    const char *arg1;
    const char *arg2;
    const char *arg3;
    const char *arg4;
    const char *arg5;
    const char *arg6;
    const char *arg7;
    const char *arg8;
    const char *arg9;
} generalAdminInp_t;

#define generalAdminInp_PI "str *arg0; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7;  str *arg8;  str *arg9;"

#ifdef __cplusplus
extern "C" {
#endif
#if defined(RODS_SERVER)
#define RS_GENERAL_ADMIN rsGeneralAdmin
/* prototype for the server handler */
int
rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp );

int
_rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp );
#else
#define RS_GENERAL_ADMIN NULL
#endif

/* prototype for the client call */
int
rcGeneralAdmin( rcComm_t *conn, generalAdminInp_t *generalAdminInp );

#ifdef __cplusplus
}
#endif
#endif  /* GENERAL_ADMIN_H */
