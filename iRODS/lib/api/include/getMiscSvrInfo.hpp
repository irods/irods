/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef GET_MISC_SVR_INFO_HPP
#define GET_MISC_SVR_INFO_HPP

#include "rods.h"
#include "rcMisc.hpp"
#include "procApiRequest.h"
#include "apiNumber.h"

/* there is no input struct. Therefore, the inPackInstruct is NULL */

/* definition for server type */

#define RCAT_NOT_ENABLED	0
#define RCAT_ENABLED		1

/* miscSvrInfo_t is the output struct */

typedef struct MiscSvrInfo {
    int serverType;     /* RCAT_ENABLED or RCAT_NOT_ENABLED */
    uint serverBootTime;
    char relVersion[NAME_LEN];    /* the release version number */
    char apiVersion[NAME_LEN];    /* the API version number */
    char rodsZone[NAME_LEN];      /* the zone of this server */
} miscSvrInfo_t;

#define MiscSvrInfo_PI "int serverType; int serverBootTime; str relVersion[NAME_LEN]; str apiVersion[NAME_LEN]; str rodsZone[NAME_LEN];"

#if defined(RODS_SERVER)
#define RS_GET_MISC_SVR_INFO rsGetMiscSvrInfo
/* prototype for the server handler */
int
rsGetMiscSvrInfo( rsComm_t *rsComm, miscSvrInfo_t **outSvrInfo );
#else
#define RS_GET_MISC_SVR_INFO NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif

int
rcGetMiscSvrInfo( rcComm_t *conn, miscSvrInfo_t **outSvrInfo );

#ifdef __cplusplus
}
#endif

#endif	/* GET_MISC_SVR_INFO_H */
