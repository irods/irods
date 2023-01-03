#ifndef IRODS_GET_MISC_SVR_INFO_H
#define IRODS_GET_MISC_SVR_INFO_H

#include "irods/rcConnect.h"

// there is no input struct. Therefore, the inPackInstruct is NULL
// miscSvrInfo_t is the output struct

// definition for server type
#define RCAT_NOT_ENABLED        0
#define RCAT_ENABLED            1

typedef struct MiscSvrInfo {
    int serverType;     // RCAT_ENABLED or RCAT_NOT_ENABLED
    uint serverBootTime;
    char relVersion[NAME_LEN];    // the release version number
    char apiVersion[NAME_LEN];    // the API version number
    char rodsZone[NAME_LEN];      // the zone of this server
} miscSvrInfo_t;

#define MiscSvrInfo_PI "int serverType; int serverBootTime; str relVersion[NAME_LEN]; str apiVersion[NAME_LEN]; str rodsZone[NAME_LEN];"

#ifdef __cplusplus
extern "C" {
#endif

int rcGetMiscSvrInfo(rcComm_t* conn, miscSvrInfo_t** outSvrInfo);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_MISC_SVR_INFO_H
