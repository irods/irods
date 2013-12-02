/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getXmsgTicket.c
 */

#include "getXmsgTicket.h"
#include "xmsgLib.h"

extern ticketHashQue_t XmsgHashQue[];

int
rsGetXmsgTicket (rsComm_t *rsComm, getXmsgTicketInp_t *getXmsgTicketInp,
xmsgTicketInfo_t **outXmsgTicketInfo)
{
    int status;
    int hashSlotNum;
    time_t thisTime;
    time_t inpExpireTime;

    *outXmsgTicketInfo = (xmsgTicketInfo_t*)calloc (1, sizeof (xmsgTicketInfo_t));

    /**    (*outXmsgTicketInfo)->sendTicket = random(); **/
    thisTime = time (NULL);
    inpExpireTime = getXmsgTicketInp->expireTime;
    if (inpExpireTime > 0) {
	if (inpExpireTime - thisTime > MAX_EXPIRE_INT) {
	    (*outXmsgTicketInfo)->expireTime = thisTime + MAX_EXPIRE_INT;
	} else if (inpExpireTime - thisTime <= 0) {
	    (*outXmsgTicketInfo)->expireTime = thisTime + DEF_EXPIRE_INT;
	} else {
	    (*outXmsgTicketInfo)->expireTime = inpExpireTime;
	}
    } else {
        (*outXmsgTicketInfo)->expireTime = thisTime + DEF_EXPIRE_INT;
    }
    (*outXmsgTicketInfo)->flag = getXmsgTicketInp->flag;
    while (1) {
        (*outXmsgTicketInfo)->rcvTicket = random();
	(*outXmsgTicketInfo)->sendTicket = (*outXmsgTicketInfo)->rcvTicket;
        hashSlotNum = ticketHashFunc ((*outXmsgTicketInfo)->rcvTicket);
        status = addTicketToHQue (
	  *outXmsgTicketInfo, &XmsgHashQue[hashSlotNum]);
	if (status != SYS_DUPLICATE_XMSG_TICKET) {
	    break;
	}
    }

    if (status < 0) {
	free (*outXmsgTicketInfo);
	*outXmsgTicketInfo = NULL;
    }

    return (status);
}

