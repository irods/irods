/*** Copyright (c), The University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ixmsg.c - Xmessage communicator */

#include "rodsClient.h" 

rodsEnv myRodsEnv;
rErrMsg_t errMsg;
int  connectFlag = 0;
int 
printIxmsgHelp(char *cmd) {

  printf("usage: %s s [-t ticketNum] [-n startingMessageNumber] [-r numOfReceivers] [-H header] [-M message] \n" , cmd);
  printf("usage: %s r [-n NumberOfMessages] [-t tickefStreamtNum] [-s startingSequenceNumber] [-c conditionString]\n" , cmd);
  printf("usage: %s t \n" , cmd);
  printf("usage: %s d -t ticketNum \n" , cmd);
  printf("usage: %s c -t ticketNum \n" , cmd);
  printf("usage: %s c -t ticketNum -s sequenceNum \n" , cmd);
  printf("    s: send messages. If no ticketNum is given, 1 is used \n");
  printf("    r: receive messages. If no ticketNum is given, 1 is used \n");
  printf("    t: create new message stream and get a new ticketNum \n");
  printf("    d: drop message Stream \n");
  printf("    c: clear message Stream \n");
  printf("    e: erase a message \n");
  printReleaseInfo("ixmsg"); 
  exit(1);
}


int 
sendIxmsg( rcComm_t **inconn, sendXmsgInp_t *sendXmsgInp){
  int status;
  int sleepSec = 1;
  rcComm_t *conn;

  conn = *inconn;

  while (connectFlag == 0) {
    conn = rcConnectXmsg (&myRodsEnv, &errMsg);
    if  (conn == NULL) {
      sleep(sleepSec);
      sleepSec = 2 * sleepSec;
      if (sleepSec > 10) sleepSec = 10;
      continue;
    }
    status = clientLogin(conn);
    if (status != 0) {
      rcDisconnect(conn);
      fprintf (stderr, "clientLogin error...Will try again\n");
      sleep(sleepSec);
      sleepSec = 2 * sleepSec;
      if (sleepSec > 10) sleepSec = 10;
      continue;
    }
    *inconn = conn;
    connectFlag = 1;
  }
  status = rcSendXmsg (conn, sendXmsgInp);
  /*  rcDisconnect(conn); **/
  if (status < 0) {
    fprintf (stderr, "rsSendXmsg error. status = %d\n", status);
    exit (9);
  }
  return(status);
}

int
main(int argc, char **argv)
{
    rcComm_t *conn = NULL;
    int status;
    int mNum = 0;
    int tNum = 1;
    int opt;
    int sNum = 0;
    int sleepSec = 1;
    int rNum = 1;
    char  msgBuf[4000];
    char  msgHdr[HEADER_TYPE_LEN];
    char  condStr[NAME_LEN];
    char myHostName[MAX_NAME_LEN];
    char cmd[10];

    getXmsgTicketInp_t getXmsgTicketInp;
    xmsgTicketInfo_t xmsgTicketInfo;
    xmsgTicketInfo_t *outXmsgTicketInfo;
    sendXmsgInp_t sendXmsgInp;
    rcvXmsgInp_t rcvXmsgInp;
    rcvXmsgOut_t *rcvXmsgOut = NULL;

    msgBuf[0] ='\0';
    strcpy(msgHdr,"ixmsg");;
    myHostName[0] = '\0';
    condStr[0] ='\0';

    if (argc < 2) {
      printIxmsgHelp(argv[0]);
      exit(1);
    }      

    strncpy(cmd,argv[1],9);
    status = getRodsEnv (&myRodsEnv);
    if (status < 0) {
	fprintf (stderr, "getRodsEnv error, status = %d\n", status);
	exit (1);
    }

    while ((opt = getopt(argc, argv, "ht:n:r:H:M:c:s:")) != (char)EOF) {
      switch(opt) {
      case 't':
	tNum = atoi(optarg);
	break;
      case 'n':
	mNum = atoi(optarg);
	break;
      case 'r':
	rNum = atoi(optarg);
	break;
      case 's':
	sNum = atoi(optarg);
	break;
      case 'H':
	strncpy(msgHdr, optarg, HEADER_TYPE_LEN - 1);
	break;
      case 'M':
	strncpy(msgBuf, optarg, 3999);
	break;
      case 'c':
	strncpy(condStr, optarg, NAME_LEN - 1);
	break;
      case 'h':
	printIxmsgHelp(argv[0]);
	exit(0);
	break;
      default:
	fprintf(stderr,"Error: Unknown Option\n");
	exit (1);
	break;
      }
    }

    gethostname (myHostName, MAX_NAME_LEN);
    memset (&xmsgTicketInfo, 0, sizeof (xmsgTicketInfo));
    
    if (!strcmp(cmd, "s")) {
      memset (&sendXmsgInp, 0, sizeof (sendXmsgInp));
      xmsgTicketInfo.sendTicket = tNum;
      xmsgTicketInfo.rcvTicket = tNum;
      xmsgTicketInfo.flag = 1;
      sendXmsgInp.ticket = xmsgTicketInfo;
      snprintf(sendXmsgInp.sendAddr, NAME_LEN, "%s:%i", myHostName, getpid ());
      sendXmsgInp.sendXmsgInfo.numRcv = rNum;
      sendXmsgInp.sendXmsgInfo.msgNumber = mNum;
      strcpy(sendXmsgInp.sendXmsgInfo.msgType, msgHdr);
      sendXmsgInp.sendXmsgInfo.msg = msgBuf;

      if (strlen(msgBuf) > 0) {
	status = sendIxmsg(&conn, &sendXmsgInp);
	if (connectFlag == 1) {
	  rcDisconnect(conn); 
	}
	if (status < 0) 
	  exit(8);
	exit(0);
      }
      printf("Message Header : %s\n", msgHdr);
      printf("Message Address: %s\n",sendXmsgInp.sendAddr);
      while (fgets (msgBuf, 3999, stdin) != NULL) {
        if (strstr(msgBuf,"/EOM") == msgBuf) {
	  if (connectFlag == 1) {
	    rcDisconnect(conn); 
	  }
	  exit(0);
	}
	sendXmsgInp.sendXmsgInfo.msgNumber = mNum;
	if (mNum != 0) mNum++;
	sendXmsgInp.sendXmsgInfo.msg = msgBuf;
	status = sendIxmsg(&conn, &sendXmsgInp);
	if (status < 0) {
	  if (connectFlag == 1) {
	    rcDisconnect(conn); 
	  }
	  exit(8);
	}
      }
      if (connectFlag == 1) {
	rcDisconnect(conn); 
      }
    }
    else if (!strcmp(cmd, "r")) {
      memset (&rcvXmsgInp, 0, sizeof (rcvXmsgInp));
      rcvXmsgInp.rcvTicket = tNum;
      /*      rcvXmsgInp.msgNumber = mNum; */

      if (mNum == 0) mNum--;

      while ( mNum != 0 ) {
	if (connectFlag == 0) {
	  conn = rcConnectXmsg (&myRodsEnv, &errMsg);
	  if (conn == NULL) {
	    sleep(sleepSec);
	    sleepSec = 2 * sleepSec;
	    if (sleepSec > 10) sleepSec = 10;
	    continue;
	  }
	  status = clientLogin(conn);
	  if (status != 0) {
	    rcDisconnect(conn);
	    sleep(sleepSec);
	    sleepSec = 2 * sleepSec;
	    if (sleepSec > 10) sleepSec = 10;
	    continue;
	  }
	  connectFlag = 1;
	}
	if (strlen(condStr) > 0)
	  sprintf(rcvXmsgInp.msgCondition, "(*XSEQNUM  >= %d) && (%s)", sNum, condStr);
	else
	  sprintf(rcvXmsgInp.msgCondition, "*XSEQNUM >= %d ", sNum);

	status = rcRcvXmsg (conn, &rcvXmsgInp, &rcvXmsgOut);
	/*        rcDisconnect(conn); */
 	if (status  >= 0) {
	  printf ("%s:%s#%i::%s: %s", 
		  rcvXmsgOut->sendUserName,rcvXmsgOut->sendAddr,
		  rcvXmsgOut->seqNumber, rcvXmsgOut->msgType, rcvXmsgOut->msg);
	  if (rcvXmsgOut->msg[strlen(rcvXmsgOut->msg)-1] != '\n')
	    printf("\n");
	  sleepSec = 1;
	  mNum--;
	  sNum = rcvXmsgOut->seqNumber + 1;
	  free(rcvXmsgOut->msg);
	  free(rcvXmsgOut);
	  rcvXmsgOut = NULL;
	}
	else {
	  sleep(sleepSec);
	  sleepSec = 2 * sleepSec;
	  if (sleepSec > 10) sleepSec = 10;
	}

      }
      if (connectFlag == 1) {
	rcDisconnect(conn); 
      }
    }
    else if (!strcmp(cmd, "t")) {
      memset (&getXmsgTicketInp, 0, sizeof (getXmsgTicketInp));
      getXmsgTicketInp.flag = 1;
      conn = rcConnectXmsg (&myRodsEnv, &errMsg);
      if (conn == NULL) {
	fprintf (stderr, "rcConnect error\n");
	exit (1);
      }
      status = clientLogin(conn);
      if (status != 0) {
	fprintf (stderr, "clientLogin error\n");
	rcDisconnect(conn);
	exit (7);
      }
      status = rcGetXmsgTicket (conn, &getXmsgTicketInp, &outXmsgTicketInfo);
      rcDisconnect(conn);
      if (status != 0) {
        fprintf (stderr, "rcGetXmsgTicket error. status = %d\n", status);
        exit (8);
      }
      printf("Send Ticket Number= %i\n",outXmsgTicketInfo->sendTicket);
      printf("Recv Ticket Number= %i\n",outXmsgTicketInfo->rcvTicket);
      printf("Ticket Expiry Time= %i\n",outXmsgTicketInfo->expireTime);
      printf("Ticket Flag       = %i\n",outXmsgTicketInfo->flag);
      free (outXmsgTicketInfo);
    }
    else if (!strcmp(cmd, "c") || !strcmp(cmd, "d") || !strcmp(cmd, "e") ) {
      memset (&sendXmsgInp, 0, sizeof (sendXmsgInp));
      xmsgTicketInfo.sendTicket = tNum;
      xmsgTicketInfo.rcvTicket = tNum;
      xmsgTicketInfo.flag = 1;
      sendXmsgInp.ticket = xmsgTicketInfo;
      snprintf(sendXmsgInp.sendAddr, NAME_LEN, "%s:%i", myHostName, getpid ());
      sendXmsgInp.sendXmsgInfo.numRcv = rNum;
      if (!strcmp(cmd, "e")) {
	sendXmsgInp.sendXmsgInfo.msgNumber = sNum;
      } 
      else {
	sendXmsgInp.sendXmsgInfo.msgNumber = mNum;
      }
      strcpy(sendXmsgInp.sendXmsgInfo.msgType, msgHdr);
      sendXmsgInp.sendXmsgInfo.msg = msgBuf;
      if (!strcmp(cmd, "c")) {
	sendXmsgInp.sendXmsgInfo.miscInfo = strdup("CLEAR_STREAM");
      }
      if (!strcmp(cmd, "e")) {
        sendXmsgInp.sendXmsgInfo.miscInfo = strdup("ERASE_MESSAGE");
      }
      else {
        sendXmsgInp.sendXmsgInfo.miscInfo = strdup("DROP_STREAM");
      }
      status = sendIxmsg(&conn, &sendXmsgInp);
      if (connectFlag == 1) {
	rcDisconnect(conn);
      }
      if (status < 0)
	exit(8);
      exit(0);
    }
    else {
      fprintf(stderr,"wrong option. Check with -h\n");
      exit(9);
    }

    exit (0);
}
