/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* definitions for osauth routines */

#ifdef  __cplusplus
extern "C" {
#endif

#define OS_AUTH_FLAG     "##OS_AUTH"
#define OS_AUTH_CMD      "genOSAuth"
#define OS_AUTH_ENV_USER "OSAUTH_USERNAME"

int osauthGetAuth(char *challenge, char *username, char *authenticator, int authenticator_buflen);
int osauthVerifyResponse(char *challenge, char *username, char *response);
int osauthGetKey(char **key, int *key_len);
int osauthGetUid(char *username);
int osauthGetUsername(char *username, int username_len);
int osauthGenerateAuthenticator(char *username, int uid, char *challenge, char *key, 
                                int key_len, char *authenticator, int authenticator_len);
#ifdef  __cplusplus
}
#endif

