/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* xmsgLib.c - library routines for irodsXmsg
 */

// =-=-=-=-=-=-=-
// boost includes
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

// =-=-=-=-=-=-=-
// irods includes
#include "rsApiHandler.hpp"
#include "reGlobalsExtern.hpp"
#include "miscServerFunct.hpp"
#include "xmsgLib.hpp"
#include "reFuncDefs.hpp"
#include "irods_network_factory.hpp"
#include "initServer.hpp"

#include "sockCommNetworkInterface.hpp"

static boost::mutex			     ReqQueCondMutex;
static boost::mutex			     MessQueCondMutex;
static boost::thread*			 ProcReqThread[ NUM_XMSG_THR ];
static boost::condition_variable ReqQueCond;

static xmsgQue_t      XmsgQue;
static xmsgReq_t*     XmsgReqHead = NULL;
static xmsgReq_t*     XmsgReqTail = NULL; /* points to last item in queue */
static msParamArray_t XMsgMsParamArray;

// =-=-=-=-=-=-=-
// globally referenced variable
ticketHashQue_t XmsgHashQue[NUM_HASH_SLOT];



int
initThreadEnv() {
    return 0;
}


int
addXmsgToQues( irodsXmsg_t *irodsXmsg,  ticketMsgStruct_t *ticketMsgStruct ) {

    int status;

    MessQueCondMutex.lock();

    addXmsgToXmsgQue( irodsXmsg, &XmsgQue );
    status = addXmsgToTicketMsgStruct( irodsXmsg, ticketMsgStruct );

    MessQueCondMutex.unlock();

    return status;

}

int
addXmsgToXmsgQue( irodsXmsg_t *xmsg, xmsgQue_t *xmsgQue ) {

    if ( xmsg == NULL || xmsgQue == NULL ) {
        rodsLog( LOG_ERROR,
                 "addXmsgToQue: input xmsg or xmsgQue is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    xmsg->next = xmsg->prev = NULL;

    if ( xmsgQue->head == NULL ) {
        xmsgQue->head = xmsgQue->tail = xmsg;
    }
    else {
        /* que it on top */
        xmsgQue->head->prev = xmsg;
        xmsg->next = xmsgQue->head;
        xmsgQue->head = xmsg;
    }

    return 0;
}

int
rmXmsgFromXmsgQue( irodsXmsg_t *xmsg, xmsgQue_t *xmsgQue ) {
    if ( xmsg == NULL || xmsgQue == NULL ) {
        rodsLog( LOG_ERROR,
                 "addXmsgToQue: input xmsg or xmsgQue is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( xmsg->prev == NULL ) {
        /* at head */
        xmsgQue->head = xmsg->next;
    }
    else {
        xmsg->prev->next = xmsg->next;
    }

    if ( xmsg->next == NULL ) {
        /* at tail */
        xmsgQue->tail = xmsg->prev;
    }
    else {
        xmsg->next->prev = xmsg->prev;
    }

    xmsg->prev = xmsg->next = NULL;

    return 0;
}

int
rmXmsgFromXmsgTcketQue( irodsXmsg_t *xmsg, xmsgQue_t *xmsgQue ) {
    if ( xmsg == NULL || xmsgQue == NULL ) {
        rodsLog( LOG_ERROR,
                 "addXmsgToQue: input xmsg or xmsgQue is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( xmsg->tprev == NULL ) {
        /* at head */
        xmsgQue->head = xmsg->tnext;
    }
    else {
        xmsg->tprev->tnext = xmsg->tnext;
    }

    if ( xmsg->tnext == NULL ) {
        /* at tail */
        xmsgQue->tail = xmsg->tprev;
    }
    else {
        xmsg->tnext->tprev = xmsg->tprev;
    }

    xmsg->tprev = xmsg->tnext = NULL;

    return 0;
}

int
addXmsgToTicketMsgStruct( irodsXmsg_t *xmsg,
                          ticketMsgStruct_t *ticketMsgStruct ) {
    if ( xmsg == NULL || ticketMsgStruct == NULL ) {
        rodsLog( LOG_ERROR,
                 "addXmsgToTicketMsgStruct: input xmsg or ticketMsgStruct is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* up the expire time */
    if ( xmsg->sendTime + INC_EXPIRE_INT > ticketMsgStruct->ticket.expireTime ) {
        ticketMsgStruct->ticket.expireTime = xmsg->sendTime + INC_EXPIRE_INT;
    }

    if ( ticketMsgStruct->xmsgQue.head == NULL ) {
        ticketMsgStruct->xmsgQue.head = ticketMsgStruct->xmsgQue.tail = xmsg;
        xmsg->tnext = xmsg->tprev = NULL;
    }
    else {
        /* link it to the end */
        ticketMsgStruct->xmsgQue.tail->tnext = xmsg;
        xmsg->tprev = ticketMsgStruct->xmsgQue.tail;
        ticketMsgStruct->xmsgQue.tail = xmsg;
        xmsg->tnext = NULL;
    }
    xmsg->ticketMsgStruct = ticketMsgStruct;
    xmsg->seqNumber = ticketMsgStruct->nxtSeqNumber;
    ticketMsgStruct->nxtSeqNumber =  ticketMsgStruct->nxtSeqNumber + 1;

    return xmsg->seqNumber;
}

int checkMsgCondition( irodsXmsg_t *irodsXmsg, char *msgCond ) {
    char condStr[MAX_NAME_LEN * 2], res[MAX_NAME_LEN * 2];

    if ( msgCond == NULL || strlen( msgCond ) == 0 ) {
        return 0;
    }

    strcpy( condStr, msgCond );

    XMsgMsParamArray.msParam[0]->inOutStruct = ( char * ) irodsXmsg->sendXmsgInfo->msgType; /* *XHDR*/
    XMsgMsParamArray.msParam[1]->inOutStruct = ( char * ) irodsXmsg->sendUserName;        /* *XUSER*/
    XMsgMsParamArray.msParam[2]->inOutStruct = ( char * ) irodsXmsg->sendAddr;            /* *XADDR*/
    XMsgMsParamArray.msParam[3]->inOutStruct = ( char * ) irodsXmsg->sendXmsgInfo->miscInfo; /* *XMISC*/
    * ( int * ) XMsgMsParamArray.msParam[4]->inOutStruct = ( int ) irodsXmsg->sendXmsgInfo->msgNumber; /* *XMSGNUM*/
    * ( int * ) XMsgMsParamArray.msParam[5]->inOutStruct = ( int ) irodsXmsg->seqNumber;    /* *XSEQNUM*/
    * ( int * ) XMsgMsParamArray.msParam[6]->inOutStruct = ( int ) irodsXmsg->sendTime;     /* *XTIME*/

    if ( strcmp( condStr, "" ) == 0 ) {
        return 0;
    }
    int ret;
    int grdf[2];
    disableReDebugger( grdf );
    ret = !( computeExpression( condStr, &XMsgMsParamArray, NULL, 0, res ) == 0 );
    enableReDebugger( grdf );
    return ret;

}





int getIrodsXmsg( rcvXmsgInp_t *rcvXmsgInp, irodsXmsg_t **outIrodsXmsg ) {
    int rcvTicket = rcvXmsgInp->rcvTicket;
    char *msgCond = rcvXmsgInp->msgCondition;

    if ( outIrodsXmsg == NULL ) {
        rodsLog( LOG_ERROR,
                 "getIrodsXmsgByMsgNum: input outIrodsXmsg is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* locate the ticketMsgStruct_t */

    ticketMsgStruct_t *ticketMsgStruct;
    int status = getTicketMsgStructByTicket( rcvTicket, &ticketMsgStruct );

    if ( status < 0 ) {
        return status;
    }

    /* now locate the irodsXmsg_t */

    MessQueCondMutex.lock();
    irodsXmsg_t *tmpIrodsXmsg = ticketMsgStruct->xmsgQue.head;

    while ( tmpIrodsXmsg != NULL && checkMsgCondition( tmpIrodsXmsg, msgCond ) != 0 ) {
        tmpIrodsXmsg = tmpIrodsXmsg->tnext;
    }

    *outIrodsXmsg = tmpIrodsXmsg;
    MessQueCondMutex.unlock();
    if ( tmpIrodsXmsg == NULL ) {
        return SYS_NO_XMSG_FOR_MSG_NUMBER;
    }
    return 0;
}


#ifdef  AAAAA
int getIrodsXmsg( rcvXmsgInp_t *rcvXmsgInp, irodsXmsg_t **outIrodsXmsg ) {
    int status, i ;
    irodsXmsg_t *tmpIrodsXmsg, *prevIrodsXmsg;
    ticketMsgStruct_t *ticketMsgStruct;
    int rcvTicket;
    char *msgCond;

    rcvTicket = rcvXmsgInp->rcvTicket;
    msgCond = rcvXmsgInp->msgCondition;

    if ( outIrodsXmsg == NULL ) {
        rodsLog( LOG_ERROR,
                 "getIrodsXmsgByMsgNum: input outIrodsXmsg is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* locate the ticketMsgStruct_t */

    status = getTicketMsgStructByTicket( rcvTicket, &ticketMsgStruct );

    if ( status < 0 ) {
        return status;
    }

    /* now locate the irodsXmsg_t */


    tmpIrodsXmsg = ticketMsgStruct->xmsgQue.head;
    prevIrodsXmsg = NULL;

    while ( tmpIrodsXmsg != NULL ) {
        if ( ( i = checkMsgCondition( tmpIrodsXmsg, msgCond ) ) == 0 ) {
            /*** added to make it be part of a mutex ***/
#ifndef windows_platform
            pthread_mutex_lock( &MessQueCondMutex );
#endif
            if ( prevIrodsXmsg == NULL ) {
                if ( ticketMsgStruct->xmsgQue.head == tmpIrodsXmsg ) { /* message still there */
                    if ( ( i = checkMsgCondition( tmpIrodsXmsg, msgCond ) ) == 0 ) {
                        break;
                    }
                }
                else {
                    tmpIrodsXmsg = ticketMsgStruct->xmsgQue.head;
#ifndef windows_platform
                    pthread_mutex_unlock( &MessQueCondMutex );
#endif
                    continue;
                }
            } /* end of then of prevIrodsXmsg == NULL if */
            else if ( prevIrodsXmsg->tnext == tmpIrodsXmsg ) { /* message still there */
                if ( ( i = checkMsgCondition( tmpIrodsXmsg, msgCond ) ) == 0 ) {
                    break;
                }
            } /* end of else  of prevIrodsXmsg == NULL if */

#ifndef windows_platform
            pthread_mutex_unlock( &MessQueCondMutex );
#endif

        } /* end of main if */
        prevIrodsXmsg = tmpIrodsXmsg;
        tmpIrodsXmsg = prevIrodsXmsg->tnext;

    } /* end of while */

    *outIrodsXmsg = tmpIrodsXmsg;
    if ( tmpIrodsXmsg == NULL ) {
        return SYS_NO_XMSG_FOR_MSG_NUMBER;
    }
    else {
        return 0;
    }
}
#endif /*  AAAAA */

int
getIrodsXmsgByMsgNum( int rcvTicket, int msgNumber,
                      irodsXmsg_t **outIrodsXmsg ) {
    int status;
    irodsXmsg_t *tmpIrodsXmsg;
    ticketMsgStruct_t *ticketMsgStruct;

    if ( outIrodsXmsg == NULL ) {
        rodsLog( LOG_ERROR,
                 "getIrodsXmsgByMsgNum: input outIrodsXmsg is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* locate the ticketMsgStruct_t */

    status = getTicketMsgStructByTicket( rcvTicket, &ticketMsgStruct );

    if ( status < 0 ) {
        return status;
    }

    /* now locate the irodsXmsg_t */

    tmpIrodsXmsg = ticketMsgStruct->xmsgQue.head;

    if ( msgNumber != ANY_MSG_NUMBER ) {
        while ( tmpIrodsXmsg != NULL ) {
            if ( ( int ) tmpIrodsXmsg->sendXmsgInfo->msgNumber == msgNumber ) {
                break;
            }
            tmpIrodsXmsg = tmpIrodsXmsg->tnext;
        }
    }
    *outIrodsXmsg = tmpIrodsXmsg;
    if ( tmpIrodsXmsg == NULL ) {
        return SYS_NO_XMSG_FOR_MSG_NUMBER;
    }
    else {
        return 0;
    }
}

int
addTicketToHQue( xmsgTicketInfo_t *ticket, ticketHashQue_t *ticketHQue ) {
    int status;

    ticketMsgStruct_t *tmpTicketMsgStruct;

    if ( ticket == NULL || ticketHQue == NULL ) {
        rodsLog( LOG_ERROR,
                 "addTicketToHQue: input ticket or ticketHQue is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    tmpTicketMsgStruct = ( ticketMsgStruct_t* )calloc( 1, sizeof( ticketMsgStruct_t ) );

    /* copy the content of the ticket */

    tmpTicketMsgStruct->ticket = *ticket;
    status = addTicketMsgStructToHQue( tmpTicketMsgStruct, ticketHQue );

    if ( status < 0 ) {
        free( tmpTicketMsgStruct );
    }

    return status;
}

int
addTicketMsgStructToHQue( ticketMsgStruct_t *ticketMsgStruct,
                          ticketHashQue_t *ticketHQue ) {
    ticketMsgStruct_t *tmpTicketMsgStruct;

    if ( ticketMsgStruct == NULL || ticketHQue == NULL ) {
        rodsLog( LOG_ERROR,
                 "addTicketMsgStructToHQue: ticketMsgStruct or ticketHQue is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    ticketMsgStruct->hnext = ticketMsgStruct->hprev = NULL;
    ticketMsgStruct->nxtSeqNumber = 0;
    ticketMsgStruct->ticketHashQue = ticketHQue;

    if ( ticketHQue->head == NULL ) {
        ticketHQue->head = ticketHQue->tail = ticketMsgStruct;
        return 0;
    }


    /* que in descending order of rcvTicket */
    tmpTicketMsgStruct = ticketHQue->head;
    while ( tmpTicketMsgStruct != NULL ) {
        if ( ticketMsgStruct->ticket.rcvTicket ==
                tmpTicketMsgStruct->ticket.rcvTicket ) {
            return SYS_DUPLICATE_XMSG_TICKET;
        }
        else if ( ticketMsgStruct->ticket.rcvTicket >
                  tmpTicketMsgStruct->ticket.rcvTicket ) {
            break;
        }
        else {
            tmpTicketMsgStruct = tmpTicketMsgStruct->hnext;
        }
    }
    if ( tmpTicketMsgStruct == NULL ) {
        /* reached the end */
        ticketHQue->tail->hnext = ticketMsgStruct;
        ticketMsgStruct->hprev = ticketHQue->tail;
        ticketHQue->tail = ticketMsgStruct;
    }
    else if ( tmpTicketMsgStruct == ticketHQue->head ) {
        /* need to put ticketMsgStruct at the head */
        ticketHQue->head->hprev = ticketMsgStruct;
        ticketMsgStruct->hnext = ticketHQue->head;
        ticketHQue->head = ticketMsgStruct;
    }
    else {
        /* in the middle */
        ticketMsgStruct->hprev = tmpTicketMsgStruct->hprev;
        ticketMsgStruct->hnext = tmpTicketMsgStruct;
        tmpTicketMsgStruct->hprev->hnext = ticketMsgStruct;
        tmpTicketMsgStruct->hprev = tmpTicketMsgStruct;
    }

    return 0;
}

int
rmTicketMsgStructFromHQue( ticketMsgStruct_t *ticketMsgStruct,
                           ticketHashQue_t *ticketHQue ) {
    if ( ticketMsgStruct == NULL || ticketHQue == NULL ) {
        rodsLog( LOG_ERROR,
                 "rmTicketMsgStructFromHQue: ticketMsgStruct or ticketHQue is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( ticketMsgStruct->hprev == NULL ) {
        /* at head */
        ticketHQue->head = ticketMsgStruct->hnext;
    }
    else {
        ticketMsgStruct->hprev->hnext = ticketMsgStruct->hnext;
    }

    if ( ticketMsgStruct->hnext == NULL ) {
        /* at tail */
        ticketHQue->tail = ticketMsgStruct->hprev;
    }
    else {
        ticketMsgStruct->hnext->hprev = ticketMsgStruct->hprev;
    }

    ticketMsgStruct->hprev = ticketMsgStruct->hnext = NULL;

    return 0;
}

/* add incoming request to the bottom of the link list */

int
addReqToQue( int sock ) {
    xmsgReq_t *myXmsgReq;

    myXmsgReq = ( xmsgReq_t* )calloc( 1, sizeof( xmsgReq_t ) );

    myXmsgReq->sock = sock;

    ReqQueCondMutex.lock();

    if ( XmsgReqHead == NULL ) {
        XmsgReqHead = myXmsgReq;
        XmsgReqTail = myXmsgReq; /* points to last item in queue */
    }
    else {
        XmsgReqTail->next  = myXmsgReq;
        XmsgReqTail = myXmsgReq;
    }

    ReqQueCondMutex.unlock();

    ReqQueCond.notify_all();

    return 0;
}

xmsgReq_t *
getReqFromQue() {
    xmsgReq_t *myXmsgReq = NULL;

    while ( myXmsgReq == NULL ) {

        ReqQueCondMutex.lock();

        if ( XmsgReqHead != NULL ) {
            myXmsgReq = XmsgReqHead;
            XmsgReqHead = XmsgReqHead->next;
            ReqQueCondMutex.unlock();

            break;
        }

        ReqQueCondMutex.unlock();

        boost::unique_lock<boost::mutex> boost_lock( ReqQueCondMutex );
        ReqQueCond.wait( boost_lock );
        if ( XmsgReqHead == NULL ) {
            boost_lock.unlock();

            continue;
        }
        else {
            myXmsgReq = XmsgReqHead;
            XmsgReqHead = XmsgReqHead->next;
            boost_lock.unlock();

            break;
        }
    }

    return myXmsgReq;
}

int
startXmsgThreads() {
    for ( int i = 0; i < NUM_XMSG_THR; i++ ) {
        try {
            ProcReqThread[i] = new boost::thread( procReqRoutine );
        }
        catch ( const boost::thread_resource_error& ) {
            return SYS_THREAD_RESOURCE_ERR;
        }
    }

    return 0;
}

void
procReqRoutine() {
    struct timeval msgTimeout;
    memset( &msgTimeout, 0, sizeof( msgTimeout ) );
    msgTimeout.tv_sec = REQ_MSG_TIMEOUT_TIME;

    while ( 1 ) {
        xmsgReq_t * myXmsgReq = getReqFromQue();
        if ( myXmsgReq == NULL ) {
            /* someone else took care of it */
            continue;
        }

        rsComm_t rsComm;
        memset( &rsComm, 0, sizeof( rsComm ) );
        rsComm.sock = myXmsgReq->sock;

        // =-=-=-=-=-=-=-
        // manufacture a network object
        irods::network_object_ptr net_obj;
        irods::error ret = irods::network_factory( &rsComm, net_obj );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
        }

        startupPack_t *startupPack;
        int status = readStartupPack( net_obj, &startupPack, NULL );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "procReqRoutine: readStartupPack error, status = %d", status );
            free( myXmsgReq );
            continue;
        }
        initRsCommWithStartupPack( &rsComm, startupPack );
        free( startupPack );

        ret = sendVersion( net_obj, 0, 0, NULL, 0 );
        if ( !ret.ok() ) {
            sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
            free( myXmsgReq );
            continue;
        }
        fd_set sockMask;
        FD_ZERO( &sockMask );
        while ( 1 ) {
            int numSock;

            FD_SET( rsComm.sock, &sockMask );
            while ( ( numSock = select( rsComm.sock + 1, &sockMask,
                                        ( fd_set * ) NULL, ( fd_set * ) NULL, &msgTimeout ) ) <= 0 ) {
                if ( errno == EINTR ) {
                    rodsLog( LOG_NOTICE,
                             "procReqRoutine: select() interrupted" );
                    FD_SET( rsComm.sock, &sockMask );
                    continue;
                }
                else {
                    break;	/* timedout or something */
                }
            }
            if ( numSock < 0 ) {
                break;
            }
            status = readAndProcClientMsg( &rsComm, 0 );
            if ( status < 0 ) {
                break;
            }
        }
        close( rsComm.sock );
        free( myXmsgReq );
    }
}

/* The hash function which use rcvTicket as the key. It take the modulo of
 * rcvTicket/NUM_HASH_SLOT
 */

int
ticketHashFunc( uint rcvTicket ) {
    const int mySlot = rcvTicket % NUM_HASH_SLOT;
    return mySlot;
}

int
initXmsgHashQue() {

    xmsgTicketInfo_t *outXmsgTicketInfo;
    time_t thisTime;
    int hashSlotNum;

    memset( XmsgHashQue, 0, NUM_HASH_SLOT * sizeof( ticketHashQue_t ) );
    memset( &XmsgQue, 0, sizeof( XmsgQue ) );

    /***  have a permanent message queue with ticket-id =1,2,3,4,5***/

    thisTime = time( NULL );

    outXmsgTicketInfo = ( xmsgTicketInfo_t* )calloc( 1, sizeof( xmsgTicketInfo_t ) );
    outXmsgTicketInfo->expireTime = thisTime + ( MAX_EXPIRE_INT * 500 );
    outXmsgTicketInfo->rcvTicket = 1;
    outXmsgTicketInfo->sendTicket = 1;
    outXmsgTicketInfo->flag = 1;
    hashSlotNum = ticketHashFunc( outXmsgTicketInfo->rcvTicket );
    addTicketToHQue( outXmsgTicketInfo, &XmsgHashQue[hashSlotNum] );
    free( outXmsgTicketInfo ); // JMC cppcheck - leak
    outXmsgTicketInfo = ( xmsgTicketInfo_t* )calloc( 1, sizeof( xmsgTicketInfo_t ) );
    outXmsgTicketInfo->expireTime = thisTime + ( MAX_EXPIRE_INT * 500 );
    outXmsgTicketInfo->rcvTicket = 2;
    outXmsgTicketInfo->sendTicket = 2;
    outXmsgTicketInfo->flag = 1;
    hashSlotNum = ticketHashFunc( outXmsgTicketInfo->rcvTicket );
    addTicketToHQue( outXmsgTicketInfo, &XmsgHashQue[hashSlotNum] );
    free( outXmsgTicketInfo ); // JMC cppcheck - leak

    outXmsgTicketInfo = ( xmsgTicketInfo_t* )calloc( 1, sizeof( xmsgTicketInfo_t ) );
    outXmsgTicketInfo->expireTime = thisTime + ( MAX_EXPIRE_INT * 500 );
    outXmsgTicketInfo->rcvTicket = 3;
    outXmsgTicketInfo->sendTicket = 3;
    outXmsgTicketInfo->flag = 1;
    hashSlotNum = ticketHashFunc( outXmsgTicketInfo->rcvTicket );
    addTicketToHQue( outXmsgTicketInfo, &XmsgHashQue[hashSlotNum] );
    free( outXmsgTicketInfo ); // JMC cppcheck - leak

    outXmsgTicketInfo = ( xmsgTicketInfo_t* )calloc( 1, sizeof( xmsgTicketInfo_t ) );
    outXmsgTicketInfo->expireTime = thisTime + ( MAX_EXPIRE_INT * 500 );
    outXmsgTicketInfo->rcvTicket = 4;
    outXmsgTicketInfo->sendTicket = 4;
    outXmsgTicketInfo->flag = 1;
    hashSlotNum = ticketHashFunc( outXmsgTicketInfo->rcvTicket );
    addTicketToHQue( outXmsgTicketInfo, &XmsgHashQue[hashSlotNum] );
    free( outXmsgTicketInfo ); // JMC cppcheck - leak

    outXmsgTicketInfo = ( xmsgTicketInfo_t* )calloc( 1, sizeof( xmsgTicketInfo_t ) );
    outXmsgTicketInfo->expireTime = thisTime + ( MAX_EXPIRE_INT * 500 );
    outXmsgTicketInfo->rcvTicket = 5;
    outXmsgTicketInfo->sendTicket = 5;
    outXmsgTicketInfo->flag = 1;
    hashSlotNum = ticketHashFunc( outXmsgTicketInfo->rcvTicket );
    addTicketToHQue( outXmsgTicketInfo, &XmsgHashQue[hashSlotNum] );
    free( outXmsgTicketInfo ); // JMC cppcheck - leak

    addMsParam( &XMsgMsParamArray, "*XHDR", STR_MS_T, NULL, NULL );
    addMsParam( &XMsgMsParamArray, "*XUSER", STR_MS_T, NULL, NULL );
    addMsParam( &XMsgMsParamArray, "*XADDR", STR_MS_T, NULL, NULL );
    addMsParam( &XMsgMsParamArray, "*XMISC", STR_MS_T, NULL, NULL );
    addIntParamToArray( &XMsgMsParamArray, "*XMSGNUM", 0 );
    addIntParamToArray( &XMsgMsParamArray, "*XSEQNUM", 0 );
    addIntParamToArray( &XMsgMsParamArray, "*XTIME", 0 );

    /*** have a permanent message queue with ticket-id = 1,2,3,4,5***/

    return 0;
}

int
getTicketMsgStructByTicket( uint rcvTicket,
                            ticketMsgStruct_t **outTicketMsgStruct ) {
    int hashSlotNum;
    ticketMsgStruct_t *tmpTicketMsgStruct;

    hashSlotNum = ticketHashFunc( rcvTicket );

    tmpTicketMsgStruct = XmsgHashQue[hashSlotNum].head;

    while ( tmpTicketMsgStruct != NULL ) {
        if ( rcvTicket == tmpTicketMsgStruct->ticket.rcvTicket ) {
            *outTicketMsgStruct = tmpTicketMsgStruct;
            return 0;
        }
        else if ( rcvTicket > tmpTicketMsgStruct->ticket.rcvTicket ) {
            *outTicketMsgStruct = NULL;
            return SYS_UNMATCHED_XMSG_TICKET;
        }
        else {
            tmpTicketMsgStruct = tmpTicketMsgStruct->hnext;
        }
    }

    /* no match */
    *outTicketMsgStruct = NULL;
    return SYS_UNMATCHED_XMSG_TICKET;
}

int
_rsRcvXmsg( irodsXmsg_t *irodsXmsg, rcvXmsgOut_t *rcvXmsgOut ) {
    sendXmsgInfo_t *sendXmsgInfo;
    ticketMsgStruct_t *ticketMsgStruct;

    if ( irodsXmsg == NULL || rcvXmsgOut == NULL ) {
        rodsLog( LOG_ERROR,
                 "_rsRcvXmsg: input irodsXmsg or rcvXmsgOut is NULL" );
        MessQueCondMutex.unlock();
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    sendXmsgInfo = irodsXmsg->sendXmsgInfo;
    ticketMsgStruct = ( ticketMsgStruct_t* )irodsXmsg->ticketMsgStruct;

    /* rodsLog (LOG_NOTICE,
         "_rsRcvXmsg: SEQNum=%d, numRcv=%d", irodsXmsg->seqNumber,
         sendXmsgInfo->numRcv); */
    sendXmsgInfo = irodsXmsg->sendXmsgInfo;

    sendXmsgInfo->numRcv--;

    if ( sendXmsgInfo->numRcv <= 0 && sendXmsgInfo->numDeli <= 0 ) {
        /* done with this msg */
        rcvXmsgOut->msg = sendXmsgInfo->msg;
        rcvXmsgOut->seqNumber  = irodsXmsg->seqNumber;
        rcvXmsgOut->msgNumber  = sendXmsgInfo->msgNumber;
        sendXmsgInfo->msg = NULL;
        rstrcpy( rcvXmsgOut->msgType, sendXmsgInfo->msgType, HEADER_TYPE_LEN );
        rstrcpy( rcvXmsgOut->sendUserName, irodsXmsg->sendUserName,
                 NAME_LEN );
        rstrcpy( rcvXmsgOut->sendAddr, irodsXmsg->sendAddr,
                 NAME_LEN );
        rmXmsgFromXmsgQue( irodsXmsg, &XmsgQue );
        rmXmsgFromXmsgTcketQue( irodsXmsg, &ticketMsgStruct->xmsgQue );
        clearSendXmsgInfo( sendXmsgInfo );
        free( sendXmsgInfo );
        free( irodsXmsg );
    }
    else {
        rcvXmsgOut->msg = strdup( sendXmsgInfo->msg );
        rcvXmsgOut->seqNumber  = irodsXmsg->seqNumber;
        rcvXmsgOut->msgNumber  = sendXmsgInfo->msgNumber;
        rstrcpy( rcvXmsgOut->msgType, sendXmsgInfo->msgType, HEADER_TYPE_LEN );
        rstrcpy( rcvXmsgOut->sendUserName, irodsXmsg->sendUserName,
                 NAME_LEN );
        rstrcpy( rcvXmsgOut->sendAddr, irodsXmsg->sendAddr,
                 NAME_LEN );
    }
    MessQueCondMutex.unlock();
    return 0;
}

int
clearOneXMessage( ticketMsgStruct_t *ticketMsgStruct, int seqNum ) {


    irodsXmsg_t *tmpIrodsXmsg;

    tmpIrodsXmsg = ticketMsgStruct->xmsgQue.head;
    while ( tmpIrodsXmsg != NULL ) {
        if ( ( int ) tmpIrodsXmsg->seqNumber == seqNum ) {
            rmXmsgFromXmsgQue( tmpIrodsXmsg, &XmsgQue );
            rmXmsgFromXmsgTcketQue( tmpIrodsXmsg, &ticketMsgStruct->xmsgQue );
            clearSendXmsgInfo( tmpIrodsXmsg->sendXmsgInfo );
            free( tmpIrodsXmsg->sendXmsgInfo );
            free( tmpIrodsXmsg );
            return 0;
        }
        tmpIrodsXmsg = tmpIrodsXmsg->tnext;
    }


    return 0;
}

int
clearAllXMessages( ticketMsgStruct_t *ticketMsgStruct ) {

    irodsXmsg_t *tmpIrodsXmsg, *tmpIrodsXmsg2;

    tmpIrodsXmsg = ticketMsgStruct->xmsgQue.head;
    while ( tmpIrodsXmsg != NULL ) {
        tmpIrodsXmsg2 = tmpIrodsXmsg->tnext;
        rmXmsgFromXmsgQue( tmpIrodsXmsg, &XmsgQue );
        clearSendXmsgInfo( tmpIrodsXmsg->sendXmsgInfo );
        free( tmpIrodsXmsg->sendXmsgInfo );
        free( tmpIrodsXmsg );
        tmpIrodsXmsg = tmpIrodsXmsg2;
    }

    ticketMsgStruct->xmsgQue.head = NULL;
    ticketMsgStruct->xmsgQue.tail = NULL;
    return 0;
}

