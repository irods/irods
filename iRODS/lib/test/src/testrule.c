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


int
main(int argc, char **argv)
{
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    rErrMsg_t errMsg;
    int status;
    msParamArray_t msParamArray;
    msParam_t *myParam;
    dataObjInp_t dataObjOpenInp, dataObjCreateInp, dataObjCopyInp;
    dataObjCloseInp_t dataObjCloseInp1, dataObjCloseInp2;
    fileLseekInp_t dataObjLseekInp;
    execMyRuleInp_t execMyRuleInp;
    msParamArray_t *outParamArray = NULL;


    if (argc != 4) {
        fprintf(stderr, "Usage: %s rods_dataObj1, rods_dataObj2\n",argv[0]);
        exit(1);
    }

    status = getRodsEnv (&myRodsEnv);

    if (status < 0) {
	fprintf (stderr, "getRodsEnv error, status = %d\n", status);
	exit (1);
    }

    memset (&msParamArray, 0, sizeof (msParamArray));

    memset (&dataObjOpenInp, 0, sizeof (dataObjOpenInp));
    snprintf (dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
      myRodsEnv.rodsCwd, argv[1]);

    memset (&dataObjCreateInp, 0, sizeof (dataObjCreateInp));
    snprintf (dataObjCreateInp.objPath, MAX_NAME_LEN, "%s/%s",
      myRodsEnv.rodsCwd, argv[2]);

    memset (&dataObjCopyInp, 0, sizeof (dataObjCopyInp));
    snprintf (dataObjCopyInp.objPath, MAX_NAME_LEN, "%s/%s",
      myRodsEnv.rodsCwd, argv[3]);

    addMsParam (&msParamArray, "*A", "DataObjInp_PI", &dataObjOpenInp, NULL);
    addMsParam (&msParamArray, "*B", "DataObjInp_PI", &dataObjCreateInp, NULL);
    addMsParam (&msParamArray, "*C", "DataObjInp_PI", &dataObjCopyInp, NULL);

#if 0
    memset (&dataObjLseekInp, 0, sizeof (dataObjLseekInp));
    dataObjLseekInp.fileInx = 101;
    dataObjLseekInp.offset = 1000;
    dataObjLseekInp.whence = SEEK_SET;
    addMsParam (&msParamArray, "C", "fileLseekInp_PI", 
      &dataObjLseekInp, NULL);
#endif

    memset (&errMsg, 0, sizeof (rErrMsg_t));

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

    memset (&execMyRuleInp, 0, sizeof (execMyRuleInp));
#if 0
    rstrcpy (execMyRuleInp.myRule, "myTestRule||msiDataObjOpen(*A,*X)##|msiDataObjLseek(*X,*B,*Y)##msiDataObjClose(*C,*Z)", META_STR_LEN);
    rstrcpy (execMyRuleInp.myRule, "myTestRule||msiDataObjOpen(*A,*X)##msiDataObjCreate(*B,null,*Y)##msiDataObjClose(*X,*Z1)##msiDataObjClose(*Y,*Z2)", META_STR_LEN);
    rstrcpy (execMyRuleInp.outParamDesc, "*X%*Y%*Z1%*Z2", LONG_NAME_LEN);
#endif
    rstrcpy (execMyRuleInp.myRule, "myTestRule||msiDataObjOpen(*A,*S_FD)##msiDataObjCreate(*B,null,*D_FD)##msiDataObjLseek(*S_FD,10,SEEK_SET,*junk1)##msiDataObjRead(*S_FD,10000,*R_BUF)##msiDataObjWrite(*D_FD,*R_BUF,*W_LEN)##msiDataObjClose(*S_FD,*junk2)##msiDataObjClose(*D_FD,*junk3)##msiDataObjCopy(*B,*C,null,*junk4)##delayExec(msiDataObjRepl(*C,demoResc8,*junk5),<A></A>)##msiDataObjUnlink(*B,*junk6)", META_STR_LEN);
    rstrcpy (execMyRuleInp.outParamDesc, "*R_BUF%*W_LEN", LONG_NAME_LEN);

    execMyRuleInp.inpParamArray = &msParamArray;  
    status = rcExecMyRule (conn, &execMyRuleInp, &outParamArray);

    if (status < 0) {
        rError_t *err;
        rErrMsg_t *errMsg;
	int i, len;

	printf ("rcExecMyRule error, status = %d\n", status);
        if ((err = conn->rError) != NULL) {
            len = err->len;
            for (i=0;i<len;i++) {
                errMsg = err->errMsg[i];
                printf("Level %d: %s\n",i, errMsg->msg);
	    }
        }
    } else {
	 printf ("rcExecMyRule success\n");
    }
    rcDisconnect (conn);
} 

