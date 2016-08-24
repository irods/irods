#ifndef MOD_ACCESS_CONTROL_H__
#define MOD_ACCESS_CONTROL_H__

#include "rcConnect.h"

#define MOD_RESC_PREFIX "resource:"  // Used to indicate a resource instead of requiring a change to the protocol
#define MOD_ADMIN_MODE_PREFIX "admin:" // To indicate admin mode, without protocol change.

typedef struct {
    int recursiveFlag;
    char *accessLevel;
    char *userName;
    char *zone;
    char *path;
} modAccessControlInp_t;
#define modAccessControlInp_PI "int recursiveFlag; str *accessLevel; str *userName; str *zone; str *path;"

#ifdef __cplusplus
extern "C"
#endif
int rcModAccessControl( rcComm_t *conn, modAccessControlInp_t *modAccessControlInp );

#endif
