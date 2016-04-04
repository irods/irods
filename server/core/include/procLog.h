#ifndef PROC_LOG_H
#define PROC_LOG_H

#include "rodsConnect.h"

int
initAndClearProcLog();
int
initProcLog();
int
logAgentProc( rsComm_t* );
int
readProcLog( int pid, procLog_t *procLog );
int
rmProcLog( int pid );

#endif //PROC_LOG_H
