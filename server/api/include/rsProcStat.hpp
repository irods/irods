#ifndef RS_PROC_STAT_HPP
#define RS_PROC_STAT_HPP

#include "rodsConnect.h"
#include "rodsDef.h"
#include "procStat.h"

int rsProcStat( rsComm_t *rsComm, procStatInp_t *procStatInp, genQueryOut_t **procStatOut );
int _rsProcStat( rsComm_t *rsComm, procStatInp_t *procStatInp, genQueryOut_t **procStatOut );
int _rsProcStatAll( rsComm_t *rsComm, genQueryOut_t **procStatOut );
int localProcStat( procStatInp_t *procStatInp, genQueryOut_t **procStatOut );
int remoteProcStat( rsComm_t *rsComm, procStatInp_t *procStatInp, genQueryOut_t **procStatOut, rodsServerHost_t *rodsServerHost );
int initProcStatOut( genQueryOut_t **procStatOut, int numProc );
int addProcToProcStatOut( procLog_t *procLog, genQueryOut_t *procStatOut );

#endif
