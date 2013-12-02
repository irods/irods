/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getLimitedPassword.h
   Get a time-limited (short-term) password.
   These are similar to Kerberos, GSI, or PAM-derived
   credentials which are  typically valid for 10 hours or so.
 */

#ifndef GET_LIMITED_PASSWORD_H
#define GET_LIMITED_PASSWORD_H

/* This is a high level Metadata type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"

typedef struct {
   int  ttl;
   char *unused1;  /* currently unused, but available without protocol
		    * change if needed */
} getLimitedPasswordInp_t;

typedef struct {
   char stringToHashWith[MAX_PASSWORD_LEN];
} getLimitedPasswordOut_t;

#define getLimitedPasswordInp_PI "int ttl; str *unused1;"

#define getLimitedPasswordOut_PI "str stringToHashWith[MAX_PASSWORD_LEN];"

#if defined(RODS_SERVER)
#define RS_GET_LIMITED_PASSWORD rsGetLimitedPassword
/* prototype for the server handler */
int
rsGetLimitedPassword (rsComm_t *rsComm, 
		      getLimitedPasswordInp_t *getLimitedPasswordInp,
		      getLimitedPasswordOut_t **getLimitedPasswordOut);
int
_rsGetLimitedPassword (rsComm_t *rsComm, 
		       getLimitedPasswordInp_t *getLimitedPasswordInp,
		       getLimitedPasswordOut_t **getLimitedPasswordOut);
#else
#define RS_GET_LIMITED_PASSWORD NULL
#endif

/* prototype for the client call */
int
rcGetLimitedPassword (rcComm_t *conn, 
		      getLimitedPasswordInp_t *getLimitedPasswordInp,
		      getLimitedPasswordOut_t **getLimitedPasswordOut);

#endif	/* GET_LIMITED_PASSWORD_H */
