/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* l1rm.c - test the high level api */

#include "rodsClient.h" 

#define USER_NAME	"rods"
#define RODS_ZONE	"temp"

/* NOTE : You have to change FILE_NAME, PAR_DIR_NAME, DIR_NAME and ADDR
 * for your own env */


#define DEST_RESC_NAME  "demoResc"
#define RESC_NAME   	"demoResc"

#define MY_MODE          0750
#define OUT_FILE_NAME       "foo1"

#define BUFSIZE	(1024*1024)

int
main(int argc, char **argv)
{
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    rErrMsg_t errMsg;
    int status;
    dataObjInp_t dataObjOpenInp;
    ruleExecDelInp_t ruleExecDelInp;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s rods_dataObj\n",argv[0]);
        exit(1);
    }

    memset (&errMsg, 0, sizeof (rErrMsg_t));

    status = getRodsEnv (&myRodsEnv);

    if (status < 0) {
	fprintf (stderr, "getRodsEnv error, status = %d\n", status);
	exit (1);
    }

    conn = rcConnect (myRodsEnv.rodsHost, myRodsEnv.rodsPort, USER_NAME,
      RODS_ZONE, 0, &errMsg);

    if (conn == NULL) {
        fprintf (stderr, "rcConnect error\n");
        exit (1);
    }

    status = clientLogin(conn);
    if (status != 0) {
        rcDisconnect(conn);
        exit (7);
    }

    /* test rcRuleExecDel call */

    memset (&ruleExecDelInp, 0, sizeof (ruleExecDelInp));
    snprintf (ruleExecDelInp.ruleExecId, NAME_LEN, "%s", argv[1]);

    status = rcRuleExecDel (conn, &ruleExecDelInp);

    if (status < 0) {
        fprintf (stderr, "ruleExecDelInp of %s error. status = %d\n", 
	 argv[1], status);
        rcDisconnect (conn);
        exit (1);
    } else {
        printf ("ruleExecDelInp: status = %d\n", status);
    }


    /* test rcDataObjUnlink call */

    memset (&dataObjOpenInp, 0, sizeof (dataObjOpenInp));

    snprintf (dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
      myRodsEnv.rodsCwd, argv[1]);

    status = rcDataObjUnlink (conn, &dataObjOpenInp);

    if (status < 0) {
        fprintf (stderr, "rcDataObjUnlink error. status = %d\n", status);
        rcDisconnect (conn);
        exit (1);
    } else {
        printf ("rcDataObjUnlink: status = %d\n", status);
    }


    rcDisconnect (conn);
} 

