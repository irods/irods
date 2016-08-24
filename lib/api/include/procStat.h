#ifndef PROC_STAT_H__
#define PROC_STAT_H__

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsGenQuery.h"

// fake attri index for procStatOut
#define PID_INX                 1000001
#define STARTTIME_INX           1000002
#define CLIENT_NAME_INX         1000003
#define CLIENT_ZONE_INX         1000004
#define PROXY_NAME_INX          1000005
#define PROXY_ZONE_INX          1000006
#define REMOTE_ADDR_INX         1000007
#define PROG_NAME_INX           1000008
#define SERVER_ADDR_INX         1000009

typedef struct {
    char addr[LONG_NAME_LEN];       // if non empty, stat at this addr
    char rodsZone[NAME_LEN];	    // the zone
    keyValPair_t condInput;
} procStatInp_t;

#define ProcStatInp_PI "str addr[LONG_NAME_LEN];str rodsZone[NAME_LEN];struct KeyValPair_PI;"

#define MAX_PROC_STAT_CNT      2000

#ifdef __cplusplus
extern "C"
#endif
int rcProcStat( rcComm_t *conn, procStatInp_t *procStatInp, genQueryOut_t **procStatOut );

#endif
