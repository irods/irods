/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* definitions for osauth routines */

#ifndef OS_AUTH_H__
#define OS_AUTH_H__


#define OS_AUTH_CMD      "var/lib/irods/clients/bin/genOSAuth"
#define OS_AUTH_ENV_USER "OSAUTH_USERNAME"
#define OS_AUTH_KEYFILE "etc/irods/irods.key"

#ifdef __cplusplus
extern "C" {
#endif

int osauthGetAuth( char *challenge, char *username, char *authenticator, int authenticator_buflen );
int osauthVerifyResponse( char *challenge, char *username, char *response );
int osauthGetKey( char **key, int *key_len );
int osauthGetUid( char *username );
int osauthGetUsername( char *username, int username_len );
int osauthGenerateAuthenticator( char *username, int uid, char *challenge, char *key,
                                 int key_len, char *authenticator, int authenticator_len );
#ifdef __cplusplus
}
#endif

#endif // OS_AUTH_H__
