/* This is script-generated code.  */ 
/* See rcvXmsg.h for a description of this API call.*/

#include "rcvXmsg.h"

int
rcRcvXmsg (rcComm_t *conn, rcvXmsgInp_t *rcvXmsgInp, 
rcvXmsgOut_t **rcvXmsgOut)
{
    int status;
    status = procApiRequest (conn, RCV_XMSG_AN, rcvXmsgInp, NULL, 
        (void **) rcvXmsgOut, NULL);

    return (status);
}
