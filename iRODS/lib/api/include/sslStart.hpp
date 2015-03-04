/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sslStart.h
 */

/* This call is used to ask the agent to turn SSL on for the
   communication socket. */

#ifndef SSL_START_HPP
#define SSL_START_HPP

/* This is a SSL type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

typedef struct {
    char *arg0;
} sslStartInp_t;

#define sslStartInp_PI "str *arg0;"


#if defined(RODS_SERVER)
#define RS_SSL_START rsSslStart
/* prototype for the server handler */
int
rsSslStart( rsComm_t *rsComm, sslStartInp_t *sslStartInp );
#else
#define RS_SSL_START NULL
#endif

/* prototype for the client call */
#ifdef __cplusplus
extern "C" {
#endif
int
rcSslStart( rcComm_t *conn, sslStartInp_t *sslStartInp );

#ifdef __cplusplus
}
#endif
#endif	/* SSL_START_H */
