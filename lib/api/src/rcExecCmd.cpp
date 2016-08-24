/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "execCmd.h"
#include "apiNumber.h"

/**
 * \fn rcExecCmd( rcComm_t *conn, execCmd_t *execCmdInp, execCmdOut_t **execCmdOut )
 *
 * \brief Execute a command on the server.
 *
 * \user client
 *
 * \ingroup miscellaneous
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] execCmdInp - the execCmd input
 * \param[out] execCmdOut - The stdout and stderr of the command is stored int this bytesBuf.
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcExecCmd( rcComm_t *conn, execCmd_t *execCmdInp, execCmdOut_t **execCmdOut ) {
    int status;
    status = procApiRequest( conn, EXEC_CMD_AN, execCmdInp, NULL,
                             ( void ** ) execCmdOut, NULL );

    return status;
}
