#ifndef GET_TEMP_PASSWORD_FOR_OTHER_H__
#define GET_TEMP_PASSWORD_FOR_OTHER_H__

#include "rcConnect.h"
#include "authenticate.h"

typedef struct {
    char *otherUser;
    char *unused;  // Added to the protocol in case needed later
} getTempPasswordForOtherInp_t;
#define getTempPasswordForOtherInp_PI "str *targetUser; str *unused;"

typedef struct {
    char stringToHashWith[MAX_PASSWORD_LEN];
} getTempPasswordForOtherOut_t;
#define getTempPasswordForOtherOut_PI "str stringToHashWith[MAX_PASSWORD_LEN];"


int rcGetTempPasswordForOther( rcComm_t *conn, getTempPasswordForOtherInp_t *getTempPasswordForOtherInp, getTempPasswordForOtherOut_t **getTempPasswordForOtherOut );

#endif
