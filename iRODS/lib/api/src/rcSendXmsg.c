/* This is script-generated code.  */ 
/* See sendXmsg.h for a description of this API call.*/

#include "sendXmsg.h"

int
rcSendXmsg (rcComm_t *conn, sendXmsgInp_t *sendXmsgInp)
{
    int status;
    status = procApiRequest (conn, SEND_XMSG_AN, sendXmsgInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
