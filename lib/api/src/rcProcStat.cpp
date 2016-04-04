#include "procStat.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcProcStat( rcComm_t *conn, procStatInp_t *procStatInp, genQueryOut_t **procStatOut )
 *
 * \brief Get the status of a process.
 *
 * \user client
 *
 * \ingroup miscellaneous
 *
 * \since 1.0
 *
 *
 * \remark Get the connection stat of iRODS agents running in the
 * iRODS federation. By default, the stat of the iRODS agents on the iCAT
 * enabled server (IES) is listed. Other servers can be specified using
 * the "addr" field of procStatInp or using the RESC_NAME_KW keyword.
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] procStatInp
 *      \n     \b addr - the IP address of the server where the stat should be done.
 *                      A zero length addr means no input.
 *      \n     \b rodsZone - the zone name for this stat.  A zero length rodsZone means
 *                      the stat is to be done in the local zone.
 *      \n     \b condInput - conditional Input
 *      \n      -- RESC_NAME_KW - "value" - do the stat on the server where the Resource is located.
 *      \n      -- ALL_KW (and zero len value) - stat for all servers in the federation.
 * \param[in] procStatOut - The procStatOut contains 9 attributes and value arrays with the
 *              attriInx defined above. i.e.:
 *      \n      -- PID_INX - pid of the agent process
 *      \n      -- STARTTIME_INX - The connection start time in secs since Epoch.
 *      \n      -- CLIENT_NAME_INX - client user name
 *      \n      -- CLIENT_ZONE_INX - client user zone
 *      \n      -- PROXY_NAME_INX - proxy user name
 *      \n      -- PROXY_ZONE_INX - proxy user zone
 *      \n      -- REMOTE_ADDR_INX - the from address of the connection
 *      \n      -- SERVER_ADDR_INX - the server address of the connection
 *      \n      -- PROG_NAME_INX - the client program name
 *
 *      \n A row will be given for each running irods agent. If a server is
 *  completely idle, one row will still be given with all the attribute
 *  values empty (zero length string) except for the value associated
 *  with the SERVER_ADDR_INX.
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcProcStat( rcComm_t *conn, procStatInp_t *procStatInp,
            genQueryOut_t **procStatOut ) {
    int status;
    status = procApiRequest( conn, PROC_STAT_AN, procStatInp,
                             NULL, ( void ** ) procStatOut, NULL );

    return status;
}
