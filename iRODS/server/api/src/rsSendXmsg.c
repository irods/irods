/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sendXmsg.c
 */

#include "sendXmsg.h"
#include "xmsgLib.h"


extern ticketHashQue_t XmsgHashQue[];
extern xmsgQue_t XmsgQue;

int
rsSendXmsg (rsComm_t *rsComm, sendXmsgInp_t *sendXmsgInp)
{
  int status, i;
    ticketMsgStruct_t *ticketMsgStruct = NULL;
    irodsXmsg_t *irodsXmsg;
    char *miscInfo;

    status = getTicketMsgStructByTicket (sendXmsgInp->ticket.rcvTicket, 
      &ticketMsgStruct);

    if (status < 0) {
	clearSendXmsgInfo (&sendXmsgInp->sendXmsgInfo);
	return status;
    }

    /* match sendTicket */
    if (ticketMsgStruct->ticket.sendTicket != sendXmsgInp->ticket.sendTicket) {
	/* unmatched sendTicket */
	rodsLog (LOG_ERROR,
	  "rsSendXmsg: sendTicket mismatch, input %d, in cache %d",
	    sendXmsgInp->ticket.sendTicket, ticketMsgStruct->ticket.sendTicket);
	return (SYS_UNMATCHED_XMSG_TICKET);
    }

    /* added by Raja Jun 30, 2010 for dropping and clearing a messageStream */
    miscInfo = sendXmsgInp->sendXmsgInfo.miscInfo;
    if (miscInfo != NULL && strlen(miscInfo) > 0) {
      if(!strcmp(miscInfo,"CLEAR_STREAM")) {
	i = clearAllXMessages(ticketMsgStruct);
	return(i);
      }
      else if (!strcmp(miscInfo,"DROP_STREAM")) {
	if(sendXmsgInp->ticket.rcvTicket > 5) {
	  i = clearAllXMessages(ticketMsgStruct);
	  if (i < 0) return (i);
	  i = rmTicketMsgStructFromHQue (ticketMsgStruct,
					 (ticketHashQue_t *) ticketMsgStruct->ticketHashQue);
	  return(i);
	}
      }
      else if (!strcmp(miscInfo,"ERASE_MESSAGE")) {
	/* msgNumber actually is the sequence Number in the queue*/
	i = clearOneXMessage(ticketMsgStruct, sendXmsgInp->sendXmsgInfo.msgNumber);
	return(i);
      }
    }

    /* create a irodsXmsg_t */

    irodsXmsg = (irodsXmsg_t*)calloc (1, sizeof (irodsXmsg_t));
    irodsXmsg->sendXmsgInfo = (sendXmsgInfo_t*)calloc (1, sizeof (sendXmsgInfo_t));
    *irodsXmsg->sendXmsgInfo = sendXmsgInp->sendXmsgInfo;
    irodsXmsg->sendTime = time (0);
    /*    rstrcpy (irodsXmsg->sendUserName, rsComm->clientUser.userName, NAME_LEN);*/
    snprintf(irodsXmsg->sendUserName,NAME_LEN,"%s@%s",rsComm->clientUser.userName,rsComm->clientUser.rodsZone);
    rstrcpy (irodsXmsg->sendAddr,sendXmsgInp->sendAddr, NAME_LEN);
    /*** moved to xmsgLib.c RAJA Nov 29 2010 ***
    addXmsgToXmsgQue (irodsXmsg, &XmsgQue);
    status = addXmsgToTicketMsgStruct (irodsXmsg, ticketMsgStruct);
    ***  moved to xmsgLib.c RAJA Nov 29 2010 ***/
    status = addXmsgToQues(irodsXmsg,  ticketMsgStruct);
    return (status);
}


