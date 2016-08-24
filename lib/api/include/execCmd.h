#ifndef EXEC_CMD_H__
#define EXEC_CMD_H__

#include "rodsConnect.h"
#include "procApiRequest.h"
#include "dataObjInpOut.h"

#define CMD_DIR "../../var/lib/irods/msiExecCmd_bin"

typedef struct ExecCmd {
    char cmd[LONG_NAME_LEN];
    char cmdArgv[HUGE_NAME_LEN];
    char execAddr[LONG_NAME_LEN];       // if non empty, exec at this addr
    char hintPath[MAX_NAME_LEN];        // exec where is file is located
    int addPathToArgv;                  // whether to add the resolved phy path to the argv.
    int dummy;                          // 64 bit alignment
    keyValPair_t condInput;
} execCmd_t;

typedef struct ExecCmdOut {
    bytesBuf_t stdoutBuf;
    bytesBuf_t stderrBuf;
    int status;  // have to move status to back for 64 bit addr
} execCmdOut_t;

#define ExecCmd_PI "str cmd[LONG_NAME_LEN]; str cmdArgv[HUGE_NAME_LEN]; str execAddr[LONG_NAME_LEN]; str hintPath[MAX_NAME_LEN]; int addPathToArgv; int dummy; struct KeyValPair_PI;"
#define ExecCmdOut_PI "struct BinBytesBuf_PI; struct BinBytesBuf_PI; int status;"

#ifdef __cplusplus
extern "C"
#endif
int rcExecCmd( rcComm_t *conn, execCmd_t *execCmdInp, execCmdOut_t **execCmdOut );

#endif
