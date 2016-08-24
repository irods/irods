#ifndef RS_EXEC_CMD_HPP
#define RS_EXEC_CMD_HPP

#include "rodsConnect.h"
#include "execCmd.h"

int rsExecCmd( rsComm_t *rsComm, execCmd_t *execCmdInp, execCmdOut_t **execCmdOut );
int initExecCmdMutex();
int _rsExecCmd( execCmd_t *execCmdInp, execCmdOut_t **execCmdOut );
int remoteExecCmd( rsComm_t *rsComm, execCmd_t *execCmdInp, execCmdOut_t **execCmdOut, rodsServerHost_t *rodsServerHost );
int execCmd( execCmd_t *execCmdInp, int stdOutFd, int stdErrFd );
int initCmdArg( char *av[], char *cmdArgv, char *cmdPath );

#endif
