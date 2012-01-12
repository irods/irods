/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See authRequest.h for a description of this API call.*/

#include "authRequest.h"
int get64RandomBytes(char *buf);

static char buf[CHALLENGE_LEN+MAX_PASSWORD_LEN+1];

int
rsAuthRequest (rsComm_t *rsComm, authRequestOut_t **authRequestOut)
{
    authRequestOut_t *result;
    char *bufp;

    *authRequestOut = (authRequestOut_t*)malloc(sizeof(authRequestOut_t));
    memset((char *)*authRequestOut, 0, sizeof(authRequestOut_t));

    memset(buf, 0, sizeof(buf));
    get64RandomBytes(buf);
    bufp = (char*)malloc(CHALLENGE_LEN+2);
    if (bufp == NULL) return(SYS_MALLOC_ERR);
    strncpy(bufp, buf, CHALLENGE_LEN+2);
    result = *authRequestOut;
    result->challenge = bufp;
    return(0);
} 

char *
_rsAuthRequestGetChallenge()
{
   return ((char *)&buf);
}
