/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* Authenticate.h
 */

#ifndef AUTHENTICATE_HPP
#define AUTHENTICATE_HPP

/* This is a high level file type API call */

#include "rods.hpp"
//#include "procApiRequest.hpp"
//#include "apiNumber.hpp"
//#include "icatDefines.hpp"

#define AUTH_SUBOP_REQ_AUTH "request challenge"
#define AUTH_SUBOP_RESP "challenge response"

#define MAX_PASSWORD_LEN 50
#define CHALLENGE_LEN 64 /* 64 bytes of data and terminating null */
#define RESPONSE_LEN 16  /* 16 bytes of data and terminating null */


typedef struct {
    char *subOp;
    char *response;
    char *username;
} AuthenticateInp_t;

#define AuthenticateInp_PI "str *subOp; str *response; str *username;"

typedef struct {
    char *challenge;
} AuthenticateOut_t;

#define AuthenticateOut_PI "str *challenge;"

#if defined(RODS_SERVER)
#define RS_AUTHENTICATE rsAuthenticate
/* prototype for the server handler */
int
rsAuthenticate( rsComm_t *rsComm, AuthenticateInp_t *authenticateInp,
                AuthenticateOut_t **authenticateOut );

int
_rsAuthenticate( rsComm_t *rsComm, AuthenticateInp_t *authenticateInp,
                 AuthenticateOut_t **authenticateOut );
#else
#define RS_AUTHENTICATE NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcAuthenticate( rcComm_t *conn, AuthenticateInp_t *authenticateInp,
                AuthenticateOut_t **authenticateOut );

#ifdef __cplusplus
}
#endif

#endif	/* AUTHENTICATE_H */
