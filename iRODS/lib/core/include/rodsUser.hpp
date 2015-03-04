/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsUser.h - common header file for rods user info
 */



#ifndef RODS_USER_HPP
#define RODS_USER_HPP

#include "rodsDef.h"



/* definition for authScheme of authInfo_t */

#define PASSWORD	0
#define GSI		1

#define PASSWORD_AUTH_KEYWD	"PASSWORD"
#define GSI_AUTH_KEYWD          "GSI"

/* env variable for overwriting clientUser */
#define CLIENT_USER_NAME_KEYWD	"clientUserName"
#define CLIENT_RODS_ZONE_KEYWD	"clientRodsZone"

/* definition for authFlag in authInfo_t */
/* REMOTE or LOCAL refers to the Zone */
/* priv or not indicates whether the user is iRODS Admin: privileged */
#define NO_USER_AUTH		0	/* not authenticated yet */
#define PUBLIC_USER_AUTH	1	/* not authenticated yet */
#define REMOTE_USER_AUTH	2	/* authenticated as remote user */
#define LOCAL_USER_AUTH		3	/* authenticated as local user */
#define REMOTE_PRIV_USER_AUTH	4	/* auth as a remote priv user */
#define LOCAL_PRIV_USER_AUTH	5	/* auth as local priv user */

#define STORAGE_ADMIN_USER      0x1000   /* allow a storageadmin to execute */
#define STORAGE_ADMIN_USER_TYPE "storageadmin"

#define XMSG_SVR_ONLY		0x4000   /* execute only xmsgServer only */
#define XMSG_SVR_ALSO		0x8000   /* execute on xmsgServer server also
* in addition to agent server */
typedef struct {
    char authScheme[NAME_LEN];     /* Authentication scheme */
    int authFlag;	/* the status of authentication */
    int flag;
    int ppid;		/* session ppid */
    char host[NAME_LEN]; /* session host */
    char authStr[NAME_LEN];      /* for gsi, the dn */
} authInfo_t;

typedef struct {
    char userInfo[NAME_LEN];
    char userComments[NAME_LEN];
    char userCreate[TIME_LEN];
    char userModify[TIME_LEN];
} userOtherInfo_t;

/* definition for flag in authInfo_t */
#define AUTH_IN_FILE	0x1	/* the authStr is in a file */

/* definition for privFlag of userInfo_t */

#define REG_USER        0       /* regular user */
#define LOC_PRIV_USER   1       /* local zone privileged user */
#define REM_PRIV_USER   2       /* remote zone privileged user */

typedef struct {
    char userName[NAME_LEN];
    char rodsZone[NAME_LEN];
    char userType[NAME_LEN];
    int sysUid;		/* the unix system uid */
    authInfo_t authInfo;
    userOtherInfo_t userOtherInfo;
} userInfo_t;


#endif	/* RODS_USER_H */
