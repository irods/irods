/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* l1test.c - test the high level api */

#include "rodsClient.h" 

#define USER_NAME	"rods"
#define RODS_ZONE	"tempZone"

/* NOTE : You have to change FILE_NAME, PAR_DIR_NAME, DIR_NAME and ADDR
 * for your own env */

#define DEST_RESC_NAME  "demoResc"
#define RESC_NAME   	"demoResc"
#define REPL_RESC_NAME   	"demoResc8"
#define REPL_RESC_NAME1   	"demoResc7"
#define MY_HOME	"/tempZone/home/rods"

#define MY_MODE          0750
#define OUT_FILE_NAME       "foo15"

#define BUFSIZE	(1024*1024)

int
main(int argc, char **argv)
{
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    int status;
    rErrMsg_t errMsg;
    int l1descInx1;
    dataObjInp_t dataObjOpenInp;
    openedDataObjInp_t dataObjCloseInp;
#if 0
    dataObjInp_t dataObjCreateInp;
    openedDataObjInp_t dataObjWriteInp;
    bytesBuf_t dataObjWriteInpBBuf;
    openedDataObjInp_t dataObjReadInp;
    bytesBuf_t dataObjReadOutBBuf;
    dataObjInp_t dataObjOprInp;
    bytesBuf_t dataObjInpBBuf;
    struct stat statbuf;
    int bytesWritten, bytesRead, total;
    int in_fd, out_fd;
    openedDataObjInp_t dataObjLseekInp;
    fileLseekOut_t *dataObjLseekOut = NULL;
    dataObjCopyInp_t dataObjCopyInp;
    collInp_t collCreateInp;
    execCmd_t execCmd;
    execCmdOut_t *execCmdOut = NULL;
    char *chksumStr;
    char myPath[MAX_NAME_LEN], myCwd[MAX_NAME_LEN];
#endif

    if (argc != 2) {
        fprintf(stderr, "Usage: %s rods_dataObj\n",argv[0]);
        exit(1);
    }

    status = getRodsEnv (&myRodsEnv);

    if (status < 0) {
	fprintf (stderr, "getRodsEnv error, status = %d\n", status);
	exit (1);
    }


    conn = rcConnect (myRodsEnv.rodsHost, myRodsEnv.rodsPort, 
      myRodsEnv.rodsUserName, myRodsEnv.rodsZone, 0, &errMsg);

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

		printf("--------------open file for read -----------\n");
		
    memset (&dataObjOpenInp, 0, sizeof (dataObjOpenInp));

    snprintf (dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
      MY_HOME, argv[1]);
    dataObjOpenInp.openFlags = O_RDONLY;

    l1descInx1 = rcDataObjOpen (conn, &dataObjOpenInp);

    if (l1descInx1 < 0) {
        fprintf (stderr, "rcDataObjOpen error. status = %d\n", l1descInx1);
        rcDisconnect (conn);
        exit (1);
    } else {
        printf ("rcDataObjOpen: l1descInx1 = %d\n", l1descInx1);
    }

		printf("--------------end of open file Read -----------\n");

    /* close the files */
    memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
    dataObjCloseInp.l1descInx = l1descInx1;

    status = rcDataObjClose (conn, &dataObjCloseInp);
    if (status < 0 ) {
        fprintf (stderr, "rcDataObjClose of %d error, status = %d\n",
          l1descInx1, status);
        exit (1);
    } else {
        printf ("rcDataObjClose: status = %d\n", status);
    }


                printf("--------------open file for write ----------\n");

    memset (&dataObjOpenInp, 0, sizeof (dataObjOpenInp));

    snprintf (dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
      MY_HOME, argv[1]);
    dataObjOpenInp.openFlags = O_WRONLY;

    l1descInx1 = rcDataObjOpen (conn, &dataObjOpenInp);

    if (l1descInx1 < 0) {
        fprintf (stderr, "rcDataObjOpen error. status = %d\n", l1descInx1);
        rcDisconnect (conn);
        exit (1);
    } else {
        printf ("rcDataObjOpen: l1descInx1 = %d\n", l1descInx1);
    }

                printf("--------------end of open file for write ---------\n");

    status = rcDataObjClose (conn, &dataObjCloseInp);
    if (status < 0 ) {
        fprintf (stderr, "rcDataObjClose of %d error, status = %d\n",
          l1descInx1, status);
        exit (1);
    } else {
        printf ("rcDataObjClose: status = %d\n", status);
    }

    rcDisconnect (conn);
    exit (0);
} 

