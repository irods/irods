
/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* This is a test program that operates like an i-command to test the
   general-query calls.  We have other general-query tests that link
   with the ICAT library, but this performs a few other tests and uses
   the rc calls.  To build, one can move this to icommands/src and
   update the icommands/Makefile or replace one of the icommands.c
   (ipwd.c, for example), with this.

   Current tests include two queries on ACLs and one that gets
   object and sorted resource information.
*/
#include "rods.h"
#include "rodsClient.h"

void usage (char *prog);


int
printGenQOut( genQueryOut_t *genQueryOut) {
   int i;
   printf("genQueryOut->rowCnt=%d\n", genQueryOut->rowCnt);
   printf("genQueryOut->attriCnt=%d\n", genQueryOut->attriCnt);
   printf("genQueryOut->totalRowCount=%d\n", genQueryOut->totalRowCount);
   for (i=0;i<genQueryOut->attriCnt;i++) {
      int j;
      printf("genQueryOut->SqlResult[%d].attriInx=%d\n",i,
	     genQueryOut->sqlResult[i].attriInx);
      printf("genQueryOut->SqlResult[%d].len=%d\n",i,
	     genQueryOut->sqlResult[i].len);
      for (j=0;j<genQueryOut->rowCnt;j++) {
	 char *tResult;
	 tResult = genQueryOut->sqlResult[i].value;
	 tResult += j*genQueryOut->sqlResult[i].len;
	 printf("genQueryOut->SqlResult[%d].value=%s\n",i, tResult);
      }
      printf("genQueryOut->continueInx=%d\n",genQueryOut->continueInx);
   }
   return(0);
}


int
doTest1(rcComm_t *Conn,
	 char *userName, char *rodsZone, char *accessPerm, char *collection) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    char condStr[MAX_NAME_LEN];
    char condStr2[MAX_NAME_LEN];
    int status;
    char accStr[LONG_NAME_LEN];
    int doAccessControlToQuery=0;

    printf("dotest1\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    if (doAccessControlToQuery) {
       snprintf (accStr, LONG_NAME_LEN, "%s", userName);
       addKeyVal (&genQueryInp.condInput, USER_NAME_CLIENT_KW, accStr);

       snprintf (accStr, LONG_NAME_LEN, "%s", rodsZone);
       addKeyVal (&genQueryInp.condInput, RODS_ZONE_CLIENT_KW, accStr);

       snprintf (accStr, LONG_NAME_LEN, "%s", accessPerm);
       addKeyVal (&genQueryInp.condInput, ACCESS_PERMISSION_KW, accStr);
    }

    snprintf (condStr, MAX_NAME_LEN, "='%s'", collection);
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

    addInxIval (&genQueryInp.selectInp, COL_COLL_ID, 1);

    snprintf (condStr2, LONG_NAME_LEN, "='%s'", userName);
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_USER_NAME, condStr2);

    snprintf (condStr2, LONG_NAME_LEN, "='%s'", rodsZone);
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_USER_ZONE, condStr2);

    addInxIval (&genQueryInp.selectInp, COL_COLL_ACCESS_NAME, ORDER_BY);

#if 0
    addInxIval (&genQueryInp.selectInp, COL_COLL_ACCESS_USER_ID, 1);
#endif

    genQueryInp.maxRows = 10;

    status =  rcGenQuery (Conn, &genQueryInp, &genQueryOut);

    printf("GenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut->totalRowCount);

    if (status == 0) {
       printGenQOut(genQueryOut);
    }

    return(0);
}

int
doTest2(rcComm_t *Conn,
	 char *userName, char *rodsZone, char *accessPerm, char *collection,
	 char *fileName) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    char condStr[MAX_NAME_LEN];
    char condStr2[MAX_NAME_LEN];
    int status;
    char accStr[LONG_NAME_LEN];
    int doAccessControlToQuery=0;

    printf("dotest2\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    if (doAccessControlToQuery) {
       snprintf (accStr, LONG_NAME_LEN, "%s", userName);
       addKeyVal (&genQueryInp.condInput, USER_NAME_CLIENT_KW, accStr);

       snprintf (accStr, LONG_NAME_LEN, "%s", rodsZone);
       addKeyVal (&genQueryInp.condInput, RODS_ZONE_CLIENT_KW, accStr);

       snprintf (accStr, LONG_NAME_LEN, "%s", accessPerm);
       addKeyVal (&genQueryInp.condInput, ACCESS_PERMISSION_KW, accStr);
    }

    snprintf (condStr, MAX_NAME_LEN, "='%s'", collection);
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

    snprintf (condStr, MAX_NAME_LEN, "='%s'", fileName);
    addInxVal (&genQueryInp.sqlCondInp, COL_DATA_NAME, condStr);

    addInxIval (&genQueryInp.selectInp, COL_D_DATA_ID, 1);

    snprintf (condStr2, LONG_NAME_LEN, "='%s'", userName);
    addInxVal (&genQueryInp.sqlCondInp, COL_DATA_USER_NAME, condStr2);

    snprintf (condStr2, LONG_NAME_LEN, "='%s'", rodsZone);
    addInxVal (&genQueryInp.sqlCondInp, COL_DATA_USER_ZONE, condStr2);

    addInxIval (&genQueryInp.selectInp, COL_DATA_ACCESS_NAME, ORDER_BY);

#if 0
    addInxIval (&genQueryInp.selectInp, COL_DATA_ACCESS_USER_ID, 1);
#endif

    genQueryInp.maxRows = 10;

    status =  rcGenQuery (Conn, &genQueryInp, &genQueryOut); 

    printf("GenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut->totalRowCount);

    if (status == 0) {
       printGenQOut(genQueryOut);
    }

    return(0);
}

int
doTest3(rcComm_t *Conn,
	 char *userName, char *rodsZone,  char *collection,
	char *fileName, char *option) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    char condStr[MAX_NAME_LEN];
#if 0
    char condStr2[MAX_NAME_LEN];
#endif
    int status;
    char accStr[LONG_NAME_LEN];
    int doAccessControlToQuery=0;

    printf("dotest3\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    if (doAccessControlToQuery) {
       snprintf (accStr, LONG_NAME_LEN, "%s", userName);
       addKeyVal (&genQueryInp.condInput, USER_NAME_CLIENT_KW, accStr);

       snprintf (accStr, LONG_NAME_LEN, "%s", rodsZone);
       addKeyVal (&genQueryInp.condInput, RODS_ZONE_CLIENT_KW, accStr);

    }

    snprintf (condStr, MAX_NAME_LEN, "='%s'", collection);
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

    snprintf (condStr, MAX_NAME_LEN, "='%s'", fileName);
    addInxVal (&genQueryInp.sqlCondInp, COL_DATA_NAME, condStr);

    addInxIval (&genQueryInp.selectInp, COL_D_DATA_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_REPL_NUM, 1);
    if (strcmp(option, "desc")==0) {
       addInxIval (&genQueryInp.selectInp, COL_R_CLASS_NAME, ORDER_BY);
    }
    else {
       addInxIval (&genQueryInp.selectInp, COL_R_CLASS_NAME, ORDER_BY_DESC);
    }

#if 0
    snprintf (condStr2, LONG_NAME_LEN, "='%s'", userName);
    addInxVal (&genQueryInp.sqlCondInp, COL_DATA_USER_NAME, condStr2);

    snprintf (condStr2, LONG_NAME_LEN, "='%s'", rodsZone);
    addInxVal (&genQueryInp.sqlCondInp, COL_DATA_USER_ZONE, condStr2);
#endif

    genQueryInp.maxRows = 10;

    status =  rcGenQuery (Conn, &genQueryInp, &genQueryOut); 

    printf("GenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut->totalRowCount);

    if (status == 0) {
       printGenQOut(genQueryOut);
    }

    return(0);
}

int
doTest4(rcComm_t *Conn,
	 char *userName, char *userDN) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int status;
    char condition1[MAX_NAME_LEN];
    char condition2[MAX_NAME_LEN];

    printf("dotest4\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    snprintf (condition1, MAX_NAME_LEN, "='%s'", userDN);
    addInxVal (&genQueryInp.sqlCondInp, COL_USER_DN, condition1);
    printf("condition1 %s\n", condition1);

    if (strlen(userName) > 0) {
       snprintf (condition2, MAX_NAME_LEN, "='%s'", 
		 userName);
       addInxVal (&genQueryInp.sqlCondInp, COL_USER_NAME, condition2);
       printf("adding condition2 %s\n", condition2);
    }

    addInxIval (&genQueryInp.selectInp, COL_USER_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_USER_TYPE, 1);
    addInxIval (&genQueryInp.selectInp, COL_USER_ZONE, 1);

    genQueryInp.maxRows = 2;

    status =  rcGenQuery (Conn, &genQueryInp, &genQueryOut); 

    printf("GenQuery status=%d\n",status);

    if (status == 0) {
       printf("genQueryOut->totalRowCount=%d\n", genQueryOut->totalRowCount);

       printGenQOut(genQueryOut);

    }

    return(0);
}

/*
Find data-objects with AVUs of the input name, ordered by the value
of that AVU.
 */
int
doTest5(rcComm_t *Conn,
	char *AvuName, char *order) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int status;
    char condition1[MAX_NAME_LEN];

    printf("dotest5\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    snprintf (condition1, MAX_NAME_LEN, "='%s'", AvuName);
    addInxVal (&genQueryInp.sqlCondInp, COL_META_DATA_ATTR_NAME, condition1);
    printf("condition1 %s\n", condition1);

    addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 0);
    addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 0);

    if (order == NULL) {
	  addInxIval (&genQueryInp.selectInp, COL_META_DATA_ATTR_VALUE, 
		      ORDER_BY_DESC);    /* regular, descending order */
    }
    else {
       if (strcmp(order, "up")==0) {     /* ascending order */
	  addInxIval (&genQueryInp.selectInp, COL_META_DATA_ATTR_VALUE, 
		      ORDER_BY);
       }
       else {                           /* regular, descending order */
	  addInxIval (&genQueryInp.selectInp, COL_META_DATA_ATTR_VALUE, 
		      ORDER_BY_DESC);
       }
    }

    genQueryInp.maxRows = 2;       /* get two rows to better show the results,
				      but could just get 1 */

    status =  rcGenQuery (Conn, &genQueryInp, &genQueryOut); 

    printf("GenQuery status=%d\n",status);
    if (status) printError(Conn, status, "rcGenQuery");

    if (status == 0) {
       printf("genQueryOut->totalRowCount=%d\n", genQueryOut->totalRowCount);

       printGenQOut(genQueryOut);

       if (genQueryOut->continueInx > 0) { /* there are more rows available */
	  genQueryInp.continueInx=genQueryOut->continueInx; 
	  genQueryInp.maxRows = 0; /* just close it out */
	  status = rcGenQuery (Conn, &genQueryInp, &genQueryOut); 
	  printf ("close out rcGenQuery status=%d\n",status);
       }
    }
    return(0);
}


/* A test function from Jean-Yves that is triggering a bug */
int
chkObjExist (rcComm_t *conn, char *inpPath, char *hostname)
{
   int status;
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut = NULL;
   char condStr[MAX_NAME_LEN];

   memset (&genQueryInp, 0, sizeof (genQueryInp));
   addInxIval(&genQueryInp.selectInp, COL_D_DATA_ID, 1);
   genQueryInp.maxRows = MAX_SQL_ROWS;
 
   snprintf (condStr, MAX_NAME_LEN, "='%s'", inpPath);
   addInxVal (&genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr);
   snprintf (condStr, MAX_NAME_LEN, "like '%s%s' || ='%s'", hostname, 
	     "%", hostname);
   addInxVal (&genQueryInp.sqlCondInp, COL_R_LOC, condStr);
 
   status = rcGenQuery (conn, &genQueryInp, &genQueryOut);
   if (status == CAT_NO_ROWS_FOUND) {
      printf ("fichier %s est orphelin\n", inpPath);
   }
   else {
      /*char *dirMPath;
 for (int i = 0; i < genQueryOut->rowCnt; i++) {
 dirMPath = genQueryOut->sqlResult[0].value;
 dirMPath += i*genQueryOut->sqlResult[0].len;
 } */
      printf ("fichier %s est bien %i dans irods\n", inpPath, status); 
/* NEEIIIINNN !!! */
   }
//   sleep(1);
 
   clearGenQueryInp(&genQueryInp);
   freeGenQueryOut(&genQueryOut);
 
   return (status);
 
}

int
doTest6(rcComm_t *Conn,
	char *inpPath, char *hostname, char *repeatCount) {
   int status;
   int rep, i;
   rep = atoi(repeatCount);
   for (i=0;i<rep;i++) {
      printf("%d ",i);
      if (i==94) {
//	 printf("sleeping\n");
//	 sleep(20);
      }
      status = chkObjExist(Conn, inpPath, hostname);
      if (status==CAT_NO_ROWS_FOUND) status=0;
      if (status) break;
   }
   return(status);
}

int
doTest7(rcComm_t *Conn,
	char *userName, char *rescName) {
   int status;
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut = NULL;
   char condition1[MAX_NAME_LEN];
   char condition2[MAX_NAME_LEN];

   memset (&genQueryInp, 0, sizeof (genQueryInp));

   genQueryInp.options = QUOTA_QUERY;

   snprintf (condition1, MAX_NAME_LEN, "%s", 
	     userName);
   addInxVal (&genQueryInp.sqlCondInp, COL_USER_NAME, condition1);

   if (rescName != NULL && strlen(rescName)>0) {
      snprintf (condition2, MAX_NAME_LEN, "%s", 
		rescName);
      addInxVal (&genQueryInp.sqlCondInp, COL_R_RESC_NAME, condition2);
   }

   genQueryInp.maxRows = 10;
 
   status = rcGenQuery (Conn, &genQueryInp, &genQueryOut);
   
   printf("GenQuery status=%d\n",status);
   if (status) printError(Conn, status, "rcGenQuery");

   if (status == 0) {
      printf("genQueryOut->totalRowCount=%d\n", genQueryOut->totalRowCount);

      printGenQOut(genQueryOut);

      if (genQueryOut->continueInx > 0) { /* there are more rows available */
	 genQueryInp.continueInx=genQueryOut->continueInx; 
	 genQueryInp.maxRows = 0; /* just close it out */
	 status = rcGenQuery (Conn, &genQueryInp, &genQueryOut); 
	 printf ("close out rcGenQuery status=%d\n",status);
      }
   }

   clearGenQueryInp(&genQueryInp);
   freeGenQueryOut(&genQueryOut);
 
   return (status);
}


int
main(int argc, char **argv)
{
    int ix, status;
    int echoFlag=0;
    rodsEnv myEnv;
    rcComm_t *Conn;
    rErrMsg_t errMsg;
    rodsArguments_t myRodsArgs;

#if 0
    struct stat statbuf;
    int doStty=0;
    char newPw[MAX_PASSWORD_LEN+10];
    int len, lcopy;

    char buf0[MAX_PASSWORD_LEN+10];
    char buf1[MAX_PASSWORD_LEN+10];
    char buf2[MAX_PASSWORD_LEN+10];
#endif


    status = parseCmdLineOpt(argc, argv, "ehvVl", 0, &myRodsArgs);
    if (status != 0) {
       printf("Use -h for help.\n");
       exit(1);
    }

    if (myRodsArgs.echo==True) {
       echoFlag=1;
    }
    if (myRodsArgs.help==True) {
       usage(argv[0]);
       exit(0);
    }

    if (myRodsArgs.longOption==True) {
	rodsLogLevel(LOG_NOTICE);
    }
 
    ix = myRodsArgs.optind;

    status = getRodsEnv (&myEnv);  /* Need to get irodsAuthFileName (if set) */
    if (status < 0) {
       rodsLog (LOG_ERROR, "main: getRodsEnv error. status = %d",
		status);
       exit (1);
    }


    /* Connect... */ 
    Conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
      myEnv.rodsZone, 0, &errMsg);
    if (Conn == NULL) {
       rodsLog(LOG_ERROR, 
		    "Saved password, but failed to connect to server %s",
	       myEnv.rodsHost);
       exit(2);
    }


    status = clientLogin(Conn);
    if (status != 0) {
       rcDisconnect(Conn);
       exit (7);
    }

    printf("argc=%d\n",argc);
    if (argc >=3) {
       if (strcmp(argv[1],"acl1")==0) {
	  doTest1(Conn, "rods", "tempZone", argv[2], argv[3]);
       }
       if (strcmp(argv[1],"acl2")==0) {
	  doTest2(Conn, "rods", "tempZone", argv[2], argv[3], argv[4]);
       }
       if (strcmp(argv[1],"resc")==0) {
	  doTest3(Conn, "rods", "tempZone", argv[2], argv[3], argv[4]);
       }
       if (strcmp(argv[1],"DN")==0) {
	  doTest4(Conn, argv[2], argv[3]);
       }
       if (strcmp(argv[1],"AVU")==0) {
	  doTest5(Conn, argv[2], argv[3]);
       }
       if (strcmp(argv[1],"jy")==0) {
	  doTest6(Conn, argv[2], argv[3], argv[4]);
       }
       if (strcmp(argv[1],"quota")==0) {
	  doTest7(Conn, argv[2], argv[3]);
       }
    }

    rcDisconnect(Conn);

    exit (0);
}


void usage (char *prog)
{
   char *msgs[]={
      "Test program for developers debug/test",
      "See source code",
      ""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("iTestGenQuery.c");
}
