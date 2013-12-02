/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* xmsgTest.c - test the high level api */

#include "rodsClient.h" 

int
main(int argc, char **argv)
{
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    int status;
    rErrMsg_t errMsg;
    getXmsgTicketInp_t getXmsgTicketInp;
    xmsgTicketInfo_t *outXmsgTicketInfo = NULL;
    sendXmsgInp_t sendXmsgInp;
    rcvXmsgInp_t rcvXmsgInp;
    rcvXmsgOut_t *rcvXmsgOut = NULL;

    status = getRodsEnv (&myRodsEnv);

    if (status < 0) {
	fprintf (stderr, "getRodsEnv error, status = %d\n", status);
	exit (1);
    }

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

    memset (&getXmsgTicketInp, 0, sizeof (getXmsgTicketInp));

    status = rcGetXmsgTicket (conn, &getXmsgTicketInp, &outXmsgTicketInfo);
    if (status != 0) {
        fprintf (stderr, "rcGetXmsgTicket error. status = %d\n", status);
        rcDisconnect(conn);
        exit (8);
    }

    memset (&sendXmsgInp, 0, sizeof (sendXmsgInp));

    sendXmsgInp.ticket = *outXmsgTicketInfo;

    sendXmsgInp.sendXmsgInfo.msgNumber = 1;
    sendXmsgInp.sendXmsgInfo.numRcv = 1;
    rstrcpy (sendXmsgInp.sendXmsgInfo.msgType, "testMsg", HEADER_TYPE_LEN);
    sendXmsgInp.sendXmsgInfo.msg = "This is test msg 1";

    status = rcSendXmsg (conn, &sendXmsgInp);
    if (status < 0) {
        fprintf (stderr, "rsSendXmsg error. status = %d\n", status);
        rcDisconnect(conn);
        exit (9);
    }

    sendXmsgInp.sendXmsgInfo.msgNumber = 2;
    rstrcpy (sendXmsgInp.sendXmsgInfo.msgType, "testMsg", HEADER_TYPE_LEN);
    sendXmsgInp.sendXmsgInfo.msg = "This is test msg 2";

    status = rcSendXmsg (conn, &sendXmsgInp);
    if (status < 0) {
        fprintf (stderr, "rsSendXmsg error. status = %d\n", status);
        rcDisconnect(conn);
        exit (9);
    }

    rcDisconnect(conn);

    conn = rcConnectXmsg (&myRodsEnv, &errMsg);

    if (conn == NULL) {
        fprintf (stderr, "rcConnect error\n");
        exit (1);
    }

    /* receive the 2nd msg */

    memset (&rcvXmsgInp, 0, sizeof (rcvXmsgInp));

    rcvXmsgInp.rcvTicket = outXmsgTicketInfo->rcvTicket;
    rcvXmsgInp.msgNumber = 2;

    status = rcRcvXmsg (conn, &rcvXmsgInp, &rcvXmsgOut);
    if (status < 0) {
        fprintf (stderr, "rsRcvXmsg error. status = %d\n", status);
        rcDisconnect(conn);
        exit (9);
    }

    printf ("Received Xmsg: type = %s, msg = %s\n", rcvXmsgOut->msgType,
      rcvXmsgOut->msg);

    /* receive the 1st msg */

    rcvXmsgInp.msgNumber = 1;

    status = rcRcvXmsg (conn, &rcvXmsgInp, &rcvXmsgOut);
    if (status < 0) {
        fprintf (stderr, "rsRcvXmsg error. status = %d\n", status);
        rcDisconnect(conn);
        exit (9);
    }

    printf ("Received Xmsg: type = %s, msg = %s\n", rcvXmsgOut->msgType,
      rcvXmsgOut->msg);

    free (outXmsgTicketInfo);

    rcDisconnect(conn);

    exit (0);
}
