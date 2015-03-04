/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* authRequest.h
 */

#ifndef AUTH_REQUEST_HPP
#define AUTH_REQUEST_HPP

/* This is a Metadata API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

#define CHALLENGE_LEN 64 /* 64 bytes of data and terminating null */
#define RESPONSE_LEN 16  /* 16 bytes of data and terminating null */


typedef struct {
    char *challenge;
} authRequestOut_t;

#define authRequestOut_PI "bin *challenge(CHALLENGE_LEN);"

#if defined(RODS_SERVER)
#define RS_AUTH_REQUEST rsAuthRequest
/* prototype for the server handler */
int
rsAuthRequest( rsComm_t *rsComm, authRequestOut_t **authRequestOut );

/* return the previous challenge */
char *
_rsAuthRequestGetChallenge();

#else
#define RS_AUTH_REQUEST NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcAuthRequest( rcComm_t *conn, authRequestOut_t **authRequestOut );

#ifdef __cplusplus
}
#endif
#endif	/* AUTH_REQUEST_H */
