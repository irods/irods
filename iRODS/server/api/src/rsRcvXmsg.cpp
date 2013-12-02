/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rcvXmsg.c
 */

#include "rcvXmsg.h"
#include "xmsgLib.h"

extern ticketHashQue_t XmsgHashQue[];
extern xmsgQue_t XmsgQue;

int
rsRcvXmsg (rsComm_t *rsComm, rcvXmsgInp_t *rcvXmsgInp, 
rcvXmsgOut_t **rcvXmsgOut)
{
    int status;
    irodsXmsg_t *irodsXmsg = NULL;
    /*
    status = getIrodsXmsgByMsgNum (rcvXmsgInp->rcvTicket,
      rcvXmsgInp->msgNumber, &irodsXmsg);
    */
    status = getIrodsXmsg (rcvXmsgInp, &irodsXmsg);
    
    if (status < 0) {
	return status;
    }

    /* get the msg */
 
    *rcvXmsgOut = (rcvXmsgOut_t*)calloc (1, sizeof (rcvXmsgOut_t));

    status = _rsRcvXmsg (irodsXmsg, *rcvXmsgOut);

    return (status);
}

