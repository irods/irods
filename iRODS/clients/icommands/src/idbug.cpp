/*** Copyright (c), The University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* idbug.c - debug rule execution with single stepping across micro-services */

#include <errno.h>
#include "rodsClient.hpp"
#include <signal.h>


rcComm_t *conn = NULL;
rodsEnv myRodsEnv;
rErrMsg_t errMsg;
char myHostName[MAX_NAME_LEN];
int streamId = -1;
int myMNum;

char lastSent[HEADER_TYPE_LEN];
char *sendAddr[100];
int sendAddrInx = 0;

/* localStatus maintains what is being expectd */
/* 1 = waiting for iRODS */
/* 2 = waiting for user */
int  localStatus = 1;

void signalIdebugExit();

int
printCommandSummary() {
    printf( "Command Prompt Summary:\n" );
    printf( " Each of the commands can be appended with the string ' for all' or ' for <num>'\n" );
    printf( "  then the message is sent to all irodsAgents involved with this debugging session or to just only one\n" );
    printf( "  <num> is the index number of the host-address:pid of the irodsAgent and is given by command prompt 'a'\n\n" );
    printf( "  a : list all iRODS agents host-address:pid involved currently in the session.\n" );
    printf( "  n|next : execute next action.\n" );
    printf( "  s|step : step into next action.\n" );
    printf( "  f|finish : step out of current action.\n" );
    printf( "  c|continue : continue running rules/micro-services.\n" );
    printf( "  C|Continue : same as 'c' but with steps shown.\n" );
    printf( "  d|discontinue : discontinue. stop at next rule/micro-service. Useful in 'c' mode.\n" );
    printf( "  b|break <action> [<qualifier>] : set breakpoint at a rule/micro-service.\n" );
    printf( "  del|delete <num> : delete breakpoint <num>.\n" );
    printf( "  l|list (r|rule <name>)|<name> : list rule, $-variables, *-variables, or breakpoints.\n" );
    printf( "  p|print <expression> : print expression.\n" );
    printf( "  w|where [<num>] : display <num> layers of rule/micro-service call stack. If <num> is not specified, display default number of steps.\n" );
    printf( "  W|Where [<num>] : display <num> layers of full rule/micro-service call stack. If <num> is not specified, display default number of steps.\n" );
    printf( "  q : cleanup and quit.\n" );
    /*  hibernate (sleep X and then continue) Hibernate (sleep X every step)
        step (step/skip throuh a  rule)
        Step (step/skip throuh a  rule in verbose mode)
        [C]continue X ([C]continue for X steps and stop)
    */
    return 0;
}

void  printIdbugHelp( char *cmd ) {
    printf( "idbug: icommand for rule debugging \n" );
    printf( "usage: %s [-h][-v n] [-c|C] \n" , cmd );
    printf( "   -h : prints this  help message \n" );
    printf( "   -v : verbose mode 1,2 or 3 \n" );
    printf( "   -c : starts debugging in continue mode for all processes\n" );
    printf( "   -C : same as -c but with steps shown\n" );

    printCommandSummary();
    printReleaseInfo( "idbug" );
}


int connectToX() {
    int status;
    int sleepSec = 1;

    if ( conn != NULL ) {
        return 0;
    }

    status = getRodsEnv( &myRodsEnv );
    if ( status < 0 ) {
        fprintf( stderr, "getRodsEnv error, status = %d\n", status );
        exit( 1 );
    }
    while ( conn == NULL ) {
        conn = rcConnectXmsg( &myRodsEnv, &errMsg );
        if ( conn == NULL ) {
            fprintf( stderr, "rcConnectXmsg error...Will try again\n" );
            sleep( sleepSec );
            sleepSec = 2 * sleepSec;
            if ( sleepSec > 10 ) {
                sleepSec = 10;
            }
            continue;
        }
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            conn = NULL;
            fprintf( stderr, "clientLogin error...Will try again\n" );
            sleep( sleepSec );
            sleepSec = 2 * sleepSec;
            if ( sleepSec > 10 ) {
                sleepSec = 10;
            }
            continue;
        }
    }
    return 0;
}

int sendIDebugCommand( char *buf, char *hdr ) {
    int status, ii;
    static int mNum = 1;
    sendXmsgInp_t sendXmsgInp;
    xmsgTicketInfo_t xmsgTicketInfo;

    memset( &xmsgTicketInfo, 0, sizeof( xmsgTicketInfo ) );
    memset( &sendXmsgInp, 0, sizeof( sendXmsgInp ) );

    xmsgTicketInfo.sendTicket = streamId;
    xmsgTicketInfo.rcvTicket = streamId;
    xmsgTicketInfo.flag = 1;
    sendXmsgInp.ticket = xmsgTicketInfo;
    if ( strcmp( buf, "quit" ) == 0 ) {
        sendXmsgInp.sendXmsgInfo.miscInfo = strdup( "DROP_STREAM" );
    }
    snprintf( sendXmsgInp.sendAddr, NAME_LEN, "%s:%i", myHostName, getpid() );
    /***
    if (strstr(hdr, "ALL") != NULL)
      sendXmsgInp.sendXmsgInfo.numRcv = sendAddrInx;
    else
      sendXmsgInp.sendXmsgInfo.numRcv = 1;
    ***/
    sendXmsgInp.sendXmsgInfo.numRcv = 1;
    sendXmsgInp.sendXmsgInfo.msgNumber = mNum;
    sendXmsgInp.sendXmsgInfo.msg = buf;
    if ( strstr( hdr, "BEGIN" ) != NULL ) {
        sendXmsgInp.sendXmsgInfo.numRcv = 0;
        strcpy( sendXmsgInp.sendXmsgInfo.msgType, "CMSG:ALL" );
        /*	printf("*** Sending:%s::%s::%i\n",sendXmsgInp.sendXmsgInfo.msgType,sendXmsgInp.sendXmsgInfo.msg,sendXmsgInp.sendXmsgInfo.numRcv);*/
        status = rcSendXmsg( conn, &sendXmsgInp );
        if ( status < 0 ) {
            fprintf( stderr, "rsSendXmsg error for %i. status = %d\n",
                     streamId, status );
            return status;
        }
        mNum++;
    }
    else if ( strstr( hdr, "ALL" ) == NULL ) {
        snprintf( sendXmsgInp.sendXmsgInfo.msgType, sizeof( sendXmsgInp.sendXmsgInfo.msgType ), "%s", hdr );
        /*	printf("*** Sending:%s::%s::%i\n",sendXmsgInp.sendXmsgInfo.msgType,sendXmsgInp.sendXmsgInfo.msg,sendXmsgInp.sendXmsgInfo.numRcv);*/
        status = rcSendXmsg( conn, &sendXmsgInp );
        if ( status < 0 ) {
            fprintf( stderr, "rsSendXmsg error for %i. status = %d\n",
                     streamId, status );
            return status;
        }
        mNum++;
    }
    else {
        for ( ii = 0; ii < sendAddrInx ; ii++ ) {
            if ( sendAddr[ii] != NULL ) {
                snprintf( hdr, HEADER_TYPE_LEN, "CMSG:%s", sendAddr[ii] );
                snprintf( sendXmsgInp.sendXmsgInfo.msgType, sizeof( sendXmsgInp.sendXmsgInfo.msgType ), "%s", hdr );
                /*	printf("*** Sending:%s::%s::%i\n",sendXmsgInp.sendXmsgInfo.msgType,sendXmsgInp.sendXmsgInfo.msg,sendXmsgInp.sendXmsgInfo.numRcv);*/
                status = rcSendXmsg( conn, &sendXmsgInp );
                if ( status < 0 ) {
                    fprintf( stderr, "rsSendXmsg error for %i. status = %d\n",
                             streamId, status );
                    return status;
                }
                mNum++;
            }
        }
    }
    return 0;
}

int getIDebugReply( rcvXmsgInp_t *rcvXmsgInp, rcvXmsgOut_t **rcvXmsgOut, int waitFlag ) {
    int status;
    int sleepNum = 1;

    while ( 1 ) {
        status = rcRcvXmsg( conn, rcvXmsgInp, rcvXmsgOut );
        if ( status >= 0 || waitFlag == 0 ) {
            return status;
        }
        sleep( sleepNum );
        if ( sleepNum < 10 ) {
            sleepNum++;
        }
    }
}

int
cleanUpAndExit() {
    sendXmsgInp_t sendXmsgInp;
    xmsgTicketInfo_t xmsgTicketInfo;
    int status;
    char buf[] = "QUIT";

    memset( &xmsgTicketInfo, 0, sizeof( xmsgTicketInfo ) );
    memset( &sendXmsgInp, 0, sizeof( sendXmsgInp ) );
    xmsgTicketInfo.sendTicket = 4;
    xmsgTicketInfo.rcvTicket = 4;
    xmsgTicketInfo.flag = 1;
    sendXmsgInp.ticket = xmsgTicketInfo;
    snprintf( sendXmsgInp.sendAddr, NAME_LEN, "%s:%i", myHostName, getpid() );
    sendXmsgInp.sendXmsgInfo.numRcv = 1;
    strcpy( sendXmsgInp.sendXmsgInfo.msgType, "QUIT" );
    sendXmsgInp.sendXmsgInfo.msg = buf;
    sendXmsgInp.sendXmsgInfo.msgNumber = myMNum;
    sendXmsgInp.sendXmsgInfo.miscInfo = strdup( "ERASE_MESSAGE" );
    status = rcSendXmsg( conn, &sendXmsgInp );
    if ( status < 0 ) {
        fprintf( stderr, "rsSendXmsg error for 4. status = %d\n", status );
    }

    sendIDebugCommand( "quit", "CMSG:QUIT" );

    rcDisconnect( conn );
    exit( 0 );
}

int
storeSendAddr( char *addr ) {
    int i;
    int j = -1;
    for ( i = 0 ; i < sendAddrInx ; i++ ) {
        if ( sendAddr[i] == NULL ) {
            if ( j < 0 ) {
                j = i;
            }
        }
        else if ( strcmp( addr, sendAddr[i] ) == 0 ) {
            return 0;
        }
    }
    if ( j < 0 ) {
        sendAddr[sendAddrInx] = strdup( addr );
        sendAddrInx++;
    }
    else {
        sendAddr[j] = strdup( addr );    /* filling holes */
    }
    /*  printf("+++added addr:%s:j=%i:sendAddrInx=%i\n",addr,j,sendAddrInx);*/
    return 0;
}
/***
int
unstoreSendAddr(char *addr) {
  int i;
  int j = 0;
  for (i = 0 ; i < sendAddrInx ; i++ ) {
    if (j == 0) {
      if (strcmp(addr, sendAddr[i]) == 0) {
	free(sendAddr[i]);
	j=1;
      }
    }
    else {
      sendAddr[i-1] = sendAddr[i];
    }
  }
  if (j == 1)
    sendAddrInx--;
  return 0;
}
**/
int
unstoreSendAddr( char *addr ) {
    int i;
    for ( i = 0 ; i < sendAddrInx ; i++ ) {
        if ( sendAddr[i] != NULL  && strcmp( addr, sendAddr[i] ) == 0 ) {
            free( sendAddr[i] );
            sendAddr[i] = NULL;
            /*      printf("+++del addr: %s:::%i\n",addr, i);*/
            break;
        }
    }
    return 0;
}

int processUserInput( char *buf ) {
    char c;
    char hdr[HEADER_TYPE_LEN];
    int i;
    char *t;
    c = buf[0];


    /*
    if (localStatus == 1 && c != 'q') {
      printf("Waiting on iRODS. User input ignored: %s\n", buf);
      return 0;
    }
    */
    if ( ( t = strstr( buf, " for " ) ) != NULL ) {
        *t = '\0';
        t = t + 5;
        while ( *t == ' ' ) {
            t++;
        }
        if ( ( strstr( t, "all" ) == t ) ) {
            snprintf( hdr, HEADER_TYPE_LEN, "CMSG:ALL" );
        }
        else {
            if ( t[strlen( t ) - 1] == '\n' ) {
                t[strlen( t ) - 1] = '\0';
            }
            if ( sendAddr[( int )atoi( t )] != NULL ) {
                snprintf( hdr, HEADER_TYPE_LEN, "CMSG:%s", sendAddr[( int )atoi( t )] );
            }
            else {
                printf( "Wrong Server: No server found for %s\n", t );
                return 0;
            }
        }
    }
    else {
        if ( sendAddrInx == 1 ) {
            snprintf( hdr, HEADER_TYPE_LEN, "CMSG:%s", sendAddr[0] );
        }
        else {
            snprintf( hdr, HEADER_TYPE_LEN, "%s", lastSent );
        }
    }
    strcpy( lastSent, hdr );

    switch ( c ) {
    case '\n':
    case '\0':
    case ' ':
        break;
    case 'n': /* next */
    case 's': /* step in */
    case 'f': /* step out */
    case 'd': /* discontinue. stop now */
    case 'w': /* where */
    case 'W': /* where */
    case 'c': /* continue */
    case 'C': /* continue */
        sendIDebugCommand( buf, hdr );
        localStatus = 1;
        break;
    case 'b': /* set breakpoint at micro-service or rule-name*/
    case 'e': /* print *,$ parameters */
    case 'p': /* print *,$ parameters */
    case 'l': /* list  $ * variables*/
        if ( strchr( buf, ' ' ) == NULL ) {
            fprintf( stderr, "Error: Unknown Option\n" );
        }
        else {
            localStatus = 1;
            sendIDebugCommand( buf, hdr );
        }
        break;
    case 'q': /* quit debugging */
        cleanUpAndExit();
        break;
    case 'h': /* help */
        printCommandSummary();
        break;
    case 'a': /* list Addresses of Processes */
        for ( i = 0; i < sendAddrInx; i++ ) {
            if ( sendAddr[i] != NULL ) {
                printf( "%i: %s\n", i, sendAddr[i] );
            }
        }
        break;
    default:
        fprintf( stderr, "Error: Unknown Option\n" );
        break;
    }
    return 0;

}

void
#if defined(linux_platform) || defined(aix_platform) || defined(solaris_platform) || defined(linux_platform) || defined(osx_platform)
signalIdbugExit( int )
#else
signalIdbugExit()
#endif
{
    cleanUpAndExit();
}


int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    int continueAllFlag = 0;
    int sleepSec = 1;
    int rNum = 1;
    int stdinflags;
    int verbose = 0;
    int opt;
    getXmsgTicketInp_t getXmsgTicketInp;
    xmsgTicketInfo_t xmsgTicketInfo;
    xmsgTicketInfo_t *outXmsgTicketInfo;
    sendXmsgInp_t sendXmsgInp;
    rcvXmsgInp_t rcvXmsgInp;
    rcvXmsgOut_t *rcvXmsgOut = NULL;
    char  ubuf[4000];

    /* set up signals */

#ifndef _WIN32
    signal( SIGINT, signalIdbugExit );
    signal( SIGHUP, signalIdbugExit );
    signal( SIGTERM, signalIdbugExit );
    signal( SIGUSR1, signalIdbugExit );
    /* XXXXX switched to SIG_DFL for embedded python. child process
     * went away. But probably have to call waitpid.
     * signal(SIGCHLD, SIG_IGN); */
    signal( SIGCHLD, SIG_DFL );
#endif


    while ( ( opt = getopt( argc, argv, "cChv:" ) ) != EOF ) {
        switch ( opt ) {
        case 'v':
            verbose = atoi( optarg );
            break;
        case 'c':
            continueAllFlag = 1;
            break;
        case 'C':
            continueAllFlag = 2;
            break;
        case 'h':
            printIdbugHelp( argv[0] );
            exit( 0 );
            break;
        default:
            fprintf( stderr, "Error: Unknown Option\n" );
            printIdbugHelp( argv[0] );
            exit( 1 );
            break;
        }
    }


    /* initialize and connect */
    snprintf( lastSent, HEADER_TYPE_LEN, "CMSG:BEGIN" );
    myHostName[0] = '\0';
    gethostname( myHostName, MAX_NAME_LEN );
    connectToX();

    memset( &xmsgTicketInfo, 0, sizeof( xmsgTicketInfo ) );
    memset( &getXmsgTicketInp, 0, sizeof( getXmsgTicketInp ) );

    /* get Ticket */
    getXmsgTicketInp.flag = 1;
    status = rcGetXmsgTicket( conn, &getXmsgTicketInp, &outXmsgTicketInfo );
    if ( status != 0 ) {
        fprintf( stderr, "Unable to get Xmsg Ticket. status = %d\n", status );
        exit( 8 );
    }
    fprintf( stdout, "Debug XMsg Stream= %i\n", outXmsgTicketInfo->sendTicket );
    streamId = outXmsgTicketInfo->sendTicket;


    /* Send STOP message on newly created Debug XMsg Stream */
    if ( continueAllFlag == 0 ) {
        status = sendIDebugCommand( "discontinue", "CMSG:BEGIN" );
    }
    else if ( continueAllFlag == 1 ) {
        status = sendIDebugCommand( "continue", "CMSG:BEGIN" );
    }
    else {
        status = sendIDebugCommand( "Continue", "CMSG:BEGIN" );
    }
    if ( status < 0 ) {
        fprintf( stderr, "Error sending Message to Debug Stream %i = %d\n",
                 streamId, status );
        exit( -1 );
    }


    /*
    memset (&sendXmsgInp, 0, sizeof (sendXmsgInp));
    xmsgTicketInfo.sendTicket = streamId;
    xmsgTicketInfo.rcvTicket = streamId;
    xmsgTicketInfo.flag = 1;
    sendXmsgInp.ticket = xmsgTicketInfo;
    snprintf(sendXmsgInp.sendAddr, NAME_LEN, "%s:%i", myHostName, getpid ());
    sendXmsgInp.sendXmsgInfo.numRcv = 100;
    sendXmsgInp.sendXmsgInfo.msgNumber = mNum;
    strcpy(sendXmsgInp.sendXmsgInfo.msgType, "idbug:client");
    snprintf(ubuf,3999, "stop");
    sendXmsgInp.sendXmsgInfo.msg = ubuf;
    status = sendIDebugCommand(&sendXmsgInp);
    mNum++;
    */

    /* Send Startup messages on Stream 4 */
    memset( &sendXmsgInp, 0, sizeof( sendXmsgInp ) );
    xmsgTicketInfo.sendTicket = 4;
    xmsgTicketInfo.rcvTicket = 4;
    xmsgTicketInfo.flag = 1;
    sendXmsgInp.ticket = xmsgTicketInfo;
    snprintf( sendXmsgInp.sendAddr, NAME_LEN, "%s:%i", myHostName, getpid() );
    sendXmsgInp.sendXmsgInfo.numRcv = 100;
    sendXmsgInp.sendXmsgInfo.msgNumber = 1;
    strcpy( sendXmsgInp.sendXmsgInfo.msgType, "STARTDEBUG" );
    snprintf( ubuf, 3999, "%i", outXmsgTicketInfo->sendTicket );
    sendXmsgInp.sendXmsgInfo.msg = ubuf;
    status = rcSendXmsg( conn, &sendXmsgInp );
    if ( status < 0 ) {
        fprintf( stderr, "Error sending Message to Stream 4 = %d\n", status );
        exit( -1 );
    }
    myMNum = status;

    /* switch off blocking for stdin */
    stdinflags = fcntl( 0, F_GETFL, 0 ); /* get current file status flags */
    stdinflags |= O_NONBLOCK;/* turn off blocking flag */
    if ( int status = fcntl( 0, F_SETFL, stdinflags ) ) { /* set up non-blocking read */
        fprintf( stderr, "fcntl failed with status: %d", status );
    }

    /* print to stdout */
    /*    printf("idbug> ");*/

    while ( 1 ) {
        /* check for user  input */
        ubuf[0] = '\0';
        if ( fgets( ubuf, 3999, stdin ) == NULL ) {
            if ( errno !=  EWOULDBLOCK ) {
                printf( "Exiting idbug\n" );
                cleanUpAndExit();
            }
        }
        else { /* got some user input */
            processUserInput( ubuf );
        }
        /* check for messages */
        ubuf[0] = '\0';
        memset( &rcvXmsgInp, 0, sizeof( rcvXmsgInp ) );
        rcvXmsgInp.rcvTicket = streamId;
        sprintf( rcvXmsgInp.msgCondition, "(*XSEQNUM >= %d) && (*XADDR != \"%s:%i\") ", rNum, myHostName, getpid() );

        status = getIDebugReply( &rcvXmsgInp, &rcvXmsgOut, 0 );
        if ( status == 0 ) {
            if ( verbose == 3 ) {
                printf( "%s:%s#%i::%s: %s",
                        rcvXmsgOut->sendUserName, rcvXmsgOut->sendAddr,
                        rcvXmsgOut->seqNumber, rcvXmsgOut->msgType, rcvXmsgOut->msg );
            }
            else if ( verbose == 2 ) {
                printf( "%s#%i::%s: %s",
                        rcvXmsgOut->sendAddr,
                        rcvXmsgOut->seqNumber, rcvXmsgOut->msgType, rcvXmsgOut->msg );
            }
            else if ( verbose == 1 ) {
                printf( "%i::%s: %s", rcvXmsgOut->seqNumber, rcvXmsgOut->msgType, rcvXmsgOut->msg );
            }
            else {
                printf( "%s: %s", rcvXmsgOut->msgType, rcvXmsgOut->msg );
            }
            if ( strstr( rcvXmsgOut->msg, "PROCESS BEGIN" ) != NULL ) {
                /*  printf(" FROM %s ", rcvXmsgOut->sendAddr); */
                storeSendAddr( rcvXmsgOut->sendAddr );
            }
            if ( strstr( rcvXmsgOut->msg, "PROCESS END" ) != NULL ) {
                printf( " FROM %s ", rcvXmsgOut->sendAddr );
                unstoreSendAddr( rcvXmsgOut->sendAddr );
            }
            if ( rcvXmsgOut->msg[strlen( rcvXmsgOut->msg ) - 1] != '\n' ) {
                printf( "\n" );
            }
            rNum  = rcvXmsgOut->seqNumber + 1;
            sleepSec = 1;
            free( rcvXmsgOut->msg );
            free( rcvXmsgOut );
            rcvXmsgOut = NULL;
            localStatus = 2;
        }
        else {
            sleep( sleepSec );
            sleepSec = 2 * sleepSec;
            /* if (sleepSec > 10) sleepSec = 10; */
            if ( sleepSec > 1 ) {
                sleepSec = 1;
            }
        }
    }
}
