/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef PROC_STAT_H__
#define PROC_STAT_H__

/* This is a Object File I/O API call */

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsGenQuery.h"

/* fake attri index for procStatOut */
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
    char addr[LONG_NAME_LEN];       /* if non empty, stat at this addr */
    char rodsZone[NAME_LEN];	    /* the zone */
    keyValPair_t condInput;
} procStatInp_t;

#define ProcStatInp_PI "str addr[LONG_NAME_LEN];str rodsZone[NAME_LEN];struct KeyValPair_PI;"

#define MAX_PROC_STAT_CNT	2000

#if defined(RODS_SERVER)
#define RS_PROC_STAT rsProcStat
#include "rodsConnect.h"
#include "rodsDef.h"
/* prototype for the server handler */
int
rsProcStat( rsComm_t *rsComm, procStatInp_t *procStatInp,
            genQueryOut_t **procStatOut );
int
_rsProcStat( rsComm_t *rsComm, procStatInp_t *procStatInp,
             genQueryOut_t **procStatOut );
int
_rsProcStatAll( rsComm_t *rsComm,
                genQueryOut_t **procStatOut );
int
localProcStat( procStatInp_t *procStatInp,
               genQueryOut_t **procStatOut );
int
remoteProcStat( rsComm_t *rsComm, procStatInp_t *procStatInp,
                genQueryOut_t **procStatOut, rodsServerHost_t *rodsServerHost );
int
initProcStatOut( genQueryOut_t **procStatOut, int numProc );
int
addProcToProcStatOut( procLog_t *procLog, genQueryOut_t *procStatOut );
#else
#define RS_PROC_STAT NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif

int
rcProcStat( rcComm_t *conn, procStatInp_t *procStatInp,
            genQueryOut_t **procStatOut );
#ifdef __cplusplus
}
#endif
#endif	// PROC_STAT_H__
