/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

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
#include "initServer.hpp"
#include "icatDefines.hpp"

typedef struct {
    char *arg0;
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    char *arg5;
    char *arg6;
    char *arg7;
    char *arg8;
    char *arg9;
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
