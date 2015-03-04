/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sslEnd.h
 */

/* This call is used to ask the agent to turn SSL off for the
   communication socket. */

#ifndef SSL_END_HPP
#define SSL_END_HPP

/* This is a SSL type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

typedef struct {
    char *arg0;
} sslEndInp_t;

#define sslEndInp_PI "str *arg0;"


#if defined(RODS_SERVER)
#define RS_SSL_END rsSslEnd
/* prototype for the server handler */
int
rsSslEnd( rsComm_t *rsComm, sslEndInp_t *sslEndInp );
#else
#define RS_SSL_END NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcSslEnd( rcComm_t *conn, sslEndInp_t *sslEndInp );

#ifdef __cplusplus
}
#endif
#endif	/* SSL_END_H */
