/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* modAccessControl.h
 */

#ifndef MOD_ACCESS_CONTROL_HPP
#define MOD_ACCESS_CONTROL_HPP

/* This is a Metadata type API call */

/*
   This call performs various operations to modify the access control
   metadata for various types of objects.  This is used by the ichmod
   command and processed by the chlModAccessControl routine.
*/

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

#define MOD_RESC_PREFIX "resource:"  /* Used to indicate a resource
* instead of requiring a change
* to the protocol */
#define MOD_ADMIN_MODE_PREFIX "admin:" /* To indicate admin mode,
without protocol change. */

typedef struct {
    int recursiveFlag;
    char *accessLevel;
    char *userName;
    char *zone;
    char *path;
} modAccessControlInp_t;

#define modAccessControlInp_PI "int recursiveFlag; str *accessLevel; str *userName; str *zone; str *path;"

#if defined(RODS_SERVER)
#define RS_MOD_ACCESS_CONTROL rsModAccessControl
/* prototype for the server handler */
int
rsModAccessControl( rsComm_t *rsComm, modAccessControlInp_t *modAccessControlInp );

int
_rsModAccessControl( rsComm_t *rsComm, modAccessControlInp_t *modAccessControlInp );
#else
#define RS_MOD_ACCESS_CONTROL NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcModAccessControl( rcComm_t *conn, modAccessControlInp_t *modAccessControlInp );

#ifdef __cplusplus
}
#endif
#endif	/* MOD_ACCESS_CONTROL_H */
