/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getTempPasswordForOther.h
 */

/* This client/server call is admin-only and is used to generate a
   temporary password for another user.  This is similar to how
   rcGetTempPassword is used to get a temporary password for the
   current user (admin or not), but allows the admin user another
   way to operate on behalf of other users.
*/

#ifndef GET_TEMP_PASSWORD_FOR_OTHER_HPP
#define GET_TEMP_PASSWORD_FOR_OTHER_HPP

/* This is a high level Metadata type API call */

#include "rods.h"
#include "rcMisc.hpp"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "authenticate.hpp"

typedef struct {
    char *otherUser;
    char *unused;  /* Added to the protocol in case needed later */
} getTempPasswordForOtherInp_t;

#define getTempPasswordForOtherInp_PI "str *targetUser; str *unused;"

typedef struct {
    char stringToHashWith[MAX_PASSWORD_LEN];
} getTempPasswordForOtherOut_t;

#define getTempPasswordForOtherOut_PI "str stringToHashWith[MAX_PASSWORD_LEN];"

#if defined(RODS_SERVER)
#define RS_GET_TEMP_PASSWORD_FOR_OTHER rsGetTempPasswordForOther
/* prototype for the server handler */
int
rsGetTempPasswordForOther( rsComm_t *rsComm,
                           getTempPasswordForOtherInp_t *getTempPasswordForOtherInp,
                           getTempPasswordForOtherOut_t **getTempPasswordForOtherOut );
int
_rsGetTempPasswordForOther( rsComm_t *rsComm,
                            getTempPasswordForOtherInp_t *getTempPasswordForOtherInp,
                            getTempPasswordForOtherOut_t **getTempPasswordForOtherOut );
#else
#define RS_GET_TEMP_PASSWORD_FOR_OTHER NULL
#endif

/* prototype for the client call */
int
rcGetTempPasswordForOther( rcComm_t *conn,
                           getTempPasswordForOtherInp_t *getTempPasswordForOtherInp,
                           getTempPasswordForOtherOut_t **getTempPasswordForOtherOut );

#endif	/* GET_TEMP_PASSWORD_FOR_OTHER_H */
