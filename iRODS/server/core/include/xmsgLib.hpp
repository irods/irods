/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* xmsgLib.h - header file for xmsgLib
 */



#ifndef XMSG_LIB_H
#define XMSG_LIB_H

#include "rods.h"
#include "rsGlobalExtern.h"   /* server global */
#include "rcGlobalExtern.h"     /* client global */
#include "rsLog.h" 
#include "rodsLog.h"
#include "sockComm.h"
#include "rsMisc.h"
#include "getRodsEnv.h"
#include "rcConnect.h"
#include "initServer.h"
#include "rodsXmsg.h"

#define REQ_MSG_TIMEOUT_TIME	5	/* 5 sec timeout for req msg */

#define NUM_HASH_SLOT		47	/* number of slots for the ticket
					 * hash key */
#define NUM_XMSG_THR		40       /* used to be 10 */

int 
initThreadEnv ();
int
addXmsgToXmsgQue (irodsXmsg_t *xmsg, xmsgQue_t *xmsgQue);
int
rmXmsgFromXmsgQue (irodsXmsg_t *xmsg, xmsgQue_t *xmsgQue);
int
addTicketToHQue (xmsgTicketInfo_t *ticket, ticketHashQue_t *ticketHQue);
int
addTicketMsgStructToHQue (ticketMsgStruct_t *ticketMsgStruct, 
ticketHashQue_t *ticketHQue);
int
rmTicketMsgStructFromHQue (ticketMsgStruct_t *ticketMsgStruct,
ticketHashQue_t *ticketHQue);
int
addReqToQue (int sock);
xmsgReq_t *getReqFromQue ();
int
startXmsgThreads ();
void
procReqRoutine ();
int
ticketHashFunc (uint rcvTicket);
int
initXmsgHashQue ();
int
getTicketMsgStructByTicket (uint rcvTicket,
ticketMsgStruct_t **outTicketMsgStruct);
int
addXmsgToTicketMsgStruct (irodsXmsg_t *xmsg,
ticketMsgStruct_t *ticketMsgStruct);

int checkMsgCondition(irodsXmsg_t *irodsXmsg, char *msgCond);

int getIrodsXmsg (rcvXmsgInp_t *rcvXmsgInp, irodsXmsg_t **outIrodsXmsg);

int 
getIrodsXmsgByMsgNum (int rcvTicket, int msgNumber,
irodsXmsg_t **outIrodsXmsg);
int
_rsRcvXmsg (irodsXmsg_t *irodsXmsg, rcvXmsgOut_t *rcvXmsgOut);

int clearAllXMessages(ticketMsgStruct_t *ticketMsgStruct);
int clearOneXMessage(ticketMsgStruct_t *ticketMsgStruct, int seqNum);

int addXmsgToQues(irodsXmsg_t *irodsXmsg,  ticketMsgStruct_t *ticketMsgStruct); 
#endif	/* XMSG_LIB_H */

