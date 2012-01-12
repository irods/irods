/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See execCmd.h for a description of this API call.*/

#include "execCmd.h"

int
rcExecCmd (rcComm_t *conn, execCmd_t *execCmdInp, execCmdOut_t **execCmdOut)
{
    int status;
    status = procApiRequest (conn, EXEC_CMD_AN, execCmdInp, NULL, 
        (void **) execCmdOut, NULL);

    return (status);
}

int
rcExecCmd241 (rcComm_t *conn, execCmd241_t *execCmdInp,
execCmdOut_t **execCmdOut)
{
    int status;
    status = procApiRequest (conn, EXEC_CMD241_AN, execCmdInp, NULL,
        (void **) execCmdOut, NULL);

    return (status);
}

