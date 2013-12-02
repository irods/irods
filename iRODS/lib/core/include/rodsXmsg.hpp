/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rodsXmsg.h - Header for for xmsg  */

#ifndef RODS_XMSG_H
#define RODS_XMSG_H

#define DEF_EXPIRE_INT	(3600 * 4)	/* default expire interval */
#define MAX_EXPIRE_INT  (3600 * 48)	/* max expire interval */
#define INC_EXPIRE_INT  (3600 * 2)	/* minimum inteval after new inteval
					 * was sent */ 
/* definition for msgNumber */
#define ANY_MSG_NUMBER	0		/* get the msg with the smallest 
					 * msgNumber */
typedef struct {
    uint expireTime;                    /* unix time of ticket expiration */
    uint flag;                          /* flag set to  XmsgTicketInfo.flag */
} getXmsgTicketInp_t;

/* definition for XmsgTicketInfo.flag */
#define SINGLE_MSG_TICKET  0	/* ticket for just a single msg. ticket will
				 * be deleted after the msg is received */
#define MULTI_MSG_TICKET   1	/* ticket for MULTIPLE MSG */
typedef struct XmsgTicketInfo {
    uint sendTicket;
    uint rcvTicket;
    uint expireTime;                    /* unix time of ticket expiration */
    uint flag;
} xmsgTicketInfo_t;

typedef struct SendXmsgInfo {
    uint msgNumber;
    char msgType[HEADER_TYPE_LEN];      /* msg type, 16 char */
    uint numRcv;                        /* number of receiver */
    int flag;				/* not used by client */
    char *msg;                          /* the msg */
    int numDeli;                         /* number of msg to deliver */
    char **deliAddress;                  /* array of str pointer of addr */
    uint *deliPort;                      /* array of port number to deliver */
    char *miscInfo;                     /* for expiration, etc */
} sendXmsgInfo_t;

typedef struct {
    xmsgTicketInfo_t ticket;            /* xmsgTicketInfo from getXmsgTicket */
    char sendAddr[NAME_LEN];                  /* send Addr can contain pid after: */
    sendXmsgInfo_t sendXmsgInfo;
} sendXmsgInp_t;

/* xmsg struct */

typedef struct IrodsXmsg {
    sendXmsgInfo_t *sendXmsgInfo;
    uint sendTime;                      /* unix time of the send */
    char sendUserName[NAME_LEN];        /* userName@zone of clientUser */
    char sendAddr[NAME_LEN];            /* sender's network address*/
    struct IrodsXmsg *prev;		/* the link list with XmsgReqHead */
    struct IrodsXmsg *next;
    struct IrodsXmsg *tnext;		/* the link list within the same
					 * ticket (diff msgNumber) */
    struct IrodsXmsg *tprev;
    uint seqNumber;
    void *ticketMsgStruct;		/* points to the ticketMsgStruct_t this
					 * xmsg belongs */
} irodsXmsg_t;

typedef struct XmsgQue {
    irodsXmsg_t *head;
    irodsXmsg_t *tail;
} xmsgQue_t;

typedef struct TicketMsgStruct {
    xmsgTicketInfo_t ticket;
    xmsgQue_t xmsgQue;		/* the link list of msg with the same ticket */
    struct TicketMsgStruct *hprev;    /* the hash link list. sort by rcvTicket 
				       */
    struct TicketMsgStruct *hnext;
    void *ticketHashQue;	/* points to the ticketHashQue_t this ticket
				 * belongs */
    uint nxtSeqNumber;
} ticketMsgStruct_t;

/* queue of msg hashed to the same slot */
typedef struct TicketHashQue {
    ticketMsgStruct_t *head;
    ticketMsgStruct_t *tail;
} ticketHashQue_t;

typedef struct XmsgReq {
    int sock;
    struct XmsgReq *next;
} xmsgReq_t;

typedef struct RcvXmsgInp {
    uint rcvTicket;
    uint msgNumber;
    uint seqNumber;
    char msgCondition[MAX_NAME_LEN];
} rcvXmsgInp_t;

typedef struct RcvXmsgOut {
    char msgType[HEADER_TYPE_LEN];      /* msg type, 16 char */
    char sendUserName[NAME_LEN];        /* userName@zone of clientUser */
    char sendAddr[NAME_LEN];            /* sender's network address*/
    uint msgNumber;                     /* msgNumber as set by sender */
    uint seqNumber;                     /* internal sequence number */
    char *msg;                          /* the msg */
} rcvXmsgOut_t;


#endif	/* RODS_XMSG_H */
