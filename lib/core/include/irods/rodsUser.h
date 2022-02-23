#ifndef RODS_USER_H__
#define RODS_USER_H__

#include "irods/rodsDef.h"

/* definition for authScheme of authInfo_t */

#define PASSWORD        0
#define GSI             1

#define PASSWORD_AUTH_KEYWD     "PASSWORD"
#define GSI_AUTH_KEYWD          "GSI"

/* env variable for overwriting clientUser */
#define CLIENT_USER_NAME_KEYWD  "clientUserName"
#define CLIENT_RODS_ZONE_KEYWD  "clientRodsZone"

/* definition for authFlag in authInfo_t */
/* REMOTE or LOCAL refers to the Zone */
/* priv or not indicates whether the user is iRODS Admin: privileged */
#define NO_USER_AUTH            0       /* not authenticated yet */
#define PUBLIC_USER_AUTH        1       /* not authenticated yet */
#define REMOTE_USER_AUTH        2       /* authenticated as remote user */
#define LOCAL_USER_AUTH         3       /* authenticated as local user */
// Authenticated as a privileged remote user. This authentication level is reserved for iRODS
// users from a remote zone which have been given administrative privileges in the local zone.
// The authenticated privileged remote user can perform actions on behalf of a user from the
// same remote zone in the local zone. Users with this level of authentication are not allowed
// to perform actions on behalf of users from other zones and cannot perform certain privileged
// actions in the local zone, which would require authentication level LOCAL_PRIV_USER_AUTH.
#define REMOTE_PRIV_USER_AUTH   4       /* auth as a remote priv user */
// Authenticated as a local privileged user. This authentication level is reserved for iRODS
// administrators in the local zone. Users with this level of authentication are allowed to
// perform almost any action within the system, including actions on behalf of other users.
#define LOCAL_PRIV_USER_AUTH    5       /* auth as local priv user */

typedef struct AuthInfo {
    char authScheme[NAME_LEN];     /* Authentication scheme */
    int authFlag;                  /* the status of authentication */
    int flag;
    int ppid;                      /* session ppid */
    char host[NAME_LEN];           /* session host */
    char authStr[NAME_LEN];        /* for gsi, the dn */
} authInfo_t;

typedef struct UserOtherInfo {
    char userInfo[NAME_LEN];
    char userComments[NAME_LEN];
    char userCreate[TIME_LEN];
    char userModify[TIME_LEN];
} userOtherInfo_t;

/* definition for flag in authInfo_t */
#define AUTH_IN_FILE    0x1     /* the authStr is in a file */

/* definition for privFlag of userInfo_t */

#define REG_USER        0       /* regular user */
#define LOC_PRIV_USER   1       /* local zone privileged user */
#define REM_PRIV_USER   2       /* remote zone privileged user */

typedef struct UserInfo {
    char userName[NAME_LEN];
    char rodsZone[NAME_LEN];
    char userType[NAME_LEN];
    int sysUid;         /* the unix system uid */
    authInfo_t authInfo;
    userOtherInfo_t userOtherInfo;
} userInfo_t;

#endif /* RODS_USER_H__ */
