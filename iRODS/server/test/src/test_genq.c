/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* 
  ICAT test program for the GeneralQuery.
*/

#include "rodsClient.h"
#include "readServerConfig.h"

#include "icatHighLevelRoutines.h"

int sTest(int i1, int i2);
int sTest2(int i1, int i2, int i3);
int findCycles(int startTable);

#define BIG_STR 200
int doLs()
{
   genQueryInp_t genQueryInp;
   genQueryOut_t genQueryOut;
   int i1a[10];
   int i1b[10];
   int i2a[10];
   int status = 0;
   char *condVal[2];
   char v1[BIG_STR];
   int i;
   char cwd[]="/";

   i1a[0]=COL_DATA_NAME;
   i1a[1]=COL_COLL_NAME;
   i1b[0]=0;
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = 2;

   i2a[0]=COL_COLL_PARENT_NAME;
   genQueryInp.sqlCondInp.inx = i2a;
   sprintf(v1,"='%s'",cwd);
   condVal[0]=v1;
   genQueryInp.sqlCondInp.value = condVal;
   genQueryInp.sqlCondInp.len=1;

   genQueryInp.maxRows=10;
   genQueryInp.continueInx=0;
   i = chlGenQuery(genQueryInp, &genQueryOut);
   printf("chlGenQuery status=%d\n",i);
   printf("genQueryOut.rowCnt=%d\n", genQueryOut.rowCnt);
   printf("genQueryOut.attriCnt=%d\n", genQueryOut.attriCnt);
   if (genQueryOut.rowCnt > 0) {
      int i, j;
      for (i=0;i<genQueryOut.attriCnt;i++) {
	 printf("genQueryOut.SqlResult[%d].attriInx=%d\n",i,
		genQueryOut.sqlResult[i].attriInx);
	 printf("genQueryOut.SqlResult[%d].len=%d\n",i,
		genQueryOut.sqlResult[i].len);
	 for (j=0;j<genQueryOut.rowCnt;j++) {
	    char *tResult;
	    tResult = genQueryOut.sqlResult[i].value;
	    tResult += j*genQueryOut.sqlResult[i].len;
	    printf("genQueryOut.SqlResult[%d].value=%s\n",i, tResult);
	 }
      }
   }
   return(status);
}

/* revised version of test that is similar to ls */
int doLs2()
{
   genQueryInp_t genQueryInp;
   genQueryOut_t genQueryOut;
   int i1a[10];
   int i1b[10];
   int i2a[10];
   char *condVal[2];
   char v1[BIG_STR];
   int i;
   char cwd[]="/tempZone/home/rods";

   memset ((char*)&genQueryInp, 0, sizeof (genQueryInp));

   i1a[0]=COL_DATA_NAME;
   i1b[0]=0;
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = 1;

   i2a[0]=COL_COLL_NAME;
   genQueryInp.sqlCondInp.inx = i2a;
   sprintf(v1,"='%s'",cwd);
   condVal[0]=v1;

   genQueryInp.sqlCondInp.value = condVal;
   genQueryInp.sqlCondInp.len=1;

   genQueryInp.maxRows=10;
   genQueryInp.continueInx=0;
   i = chlGenQuery(genQueryInp, &genQueryOut);
   printf("chlGenQuery status=%d\n",i);
   printf("genQueryOut.rowCnt=%d\n", genQueryOut.rowCnt);
   printf("genQueryOut.attriCnt=%d\n", genQueryOut.attriCnt);
   if (genQueryOut.rowCnt > 0) {
      int i, j;
      for (i=0;i<genQueryOut.attriCnt;i++) {
	 printf("genQueryOut.SqlResult[%d].attriInx=%d\n",i,
		genQueryOut.sqlResult[i].attriInx);
	 printf("genQueryOut.SqlResult[%d].len=%d\n",i,
		genQueryOut.sqlResult[i].len);
	 for (j=0;j<genQueryOut.rowCnt;j++) {
	    char *tResult;
	    tResult = genQueryOut.sqlResult[i].value;
	    tResult += j*genQueryOut.sqlResult[i].len;
	    printf("genQueryOut.SqlResult[%d].value=%s\n",i, tResult);
	 }
      }
   }
   return(i);
}


/* similar to ls2 but doesn't print much, called by doLs3 */
int doLs3a() 

{
   genQueryInp_t genQueryInp;
   genQueryOut_t genQueryOut;
   int i1a[10];
   int i1b[10];
   int i2a[10];
   char *condVal[2];
   char v1[BIG_STR];
   int i;
   char cwd[]="/newZone/home/rods";

   memset ((char*)&genQueryInp, 0, sizeof (genQueryInp));

   i1a[0]=COL_DATA_NAME;
   i1b[0]=0;
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = 1;

   i2a[0]=COL_COLL_NAME;
   genQueryInp.sqlCondInp.inx = i2a;
   sprintf(v1,"='%s'",cwd);
   condVal[0]=v1;

   genQueryInp.sqlCondInp.value = condVal;
   genQueryInp.sqlCondInp.len=1;

   genQueryInp.maxRows=10;
   genQueryInp.continueInx=0;
   i = chlGenQuery(genQueryInp, &genQueryOut);
   printf("chlGenQuery status=%d, genQueryOut.rowCnt=%d\n", i, 
	  genQueryOut.rowCnt);
   if (genQueryOut.continueInx>0) {
      int status2;
      genQueryInp.maxRows=-1;
      genQueryInp.continueInx = genQueryOut.continueInx;
      status2  = chlGenQuery(genQueryInp, &genQueryOut);
      printf("chlGenQuery second call to close out status=%d\n",status2);
   }
   return(i);
}

/* similar to ls2 but will do it repeatedly */
int doLs3(char *repCount) {
   int iRepCount, i, status;
   rodsLogSqlReq(0);  /* less verbosity */
   iRepCount = atoi(repCount);
   for (i=0;i<iRepCount;i++) {
      status = doLs3a();
      if (status) break;
   }
   return(status);
}


int
doTest2() {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    int status;

    rodsLogSqlReq(1);

    memset ((char*)&genQueryInp, 0, sizeof (genQueryInp));

    addInxIval (&genQueryInp.selectInp, COL_R_RESC_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_ZONE_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_TYPE_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_CLASS_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_LOC, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_VAULT_PATH, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_FREE_SPACE, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_INFO, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_COMMENT, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_CREATE_TIME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_MODIFY_TIME, 1);

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);
    return(status);
}

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
doTest3() {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr[MAX_NAME_LEN];
    int status;

    printf("dotest3\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    addInxIval (&genQueryInp.selectInp, COL_R_RESC_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_ZONE_NAME, 1);

    /*    snprintf (condStr, MAX_NAME_LEN, "='%s'", "e");  */
    /*    snprintf (condStr, MAX_NAME_LEN, "like '%s'", "e"); */

    /* compound condition test: */
    snprintf (condStr, MAX_NAME_LEN, "like '%s' || like 's'", "e");
    addInxVal (&genQueryInp.sqlCondInp, COL_R_RESC_NAME, condStr);

    genQueryInp.maxRows=2;

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);
    if (status == 0) {
       printGenQOut(&genQueryOut);
    }
    return(status);
}

int
doTest4() {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr[MAX_NAME_LEN];
    int status;

    printf("dotest4\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    addInxIval (&genQueryInp.selectInp, COL_RESC_GROUP_RESC_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_NAME, 1);

    /* compound condition test: */
    snprintf (condStr, MAX_NAME_LEN, "like 'abc'");
    addInxVal (&genQueryInp.sqlCondInp,  COL_RESC_GROUP_NAME, condStr);

    genQueryInp.maxRows=2;

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);
    if (status == 0) {
       printGenQOut(&genQueryOut);
    }
    return(status);
}

int
doTest5() {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr[MAX_NAME_LEN];
    int status;

    printf("dotest5\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_REI_FILE_PATH, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_USER_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_ADDRESS, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_TIME, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_FREQUENCY, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_PRIORITY, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_ESTIMATED_EXE_TIME, 1);
    addInxIval (&genQueryInp.selectInp, COL_RULE_EXEC_NOTIFICATION_ADDR, 1);

    /* compound condition test: */
    snprintf (condStr, MAX_NAME_LEN, "= 'Wayne'");
    addInxVal (&genQueryInp.sqlCondInp,  COL_RULE_EXEC_USER_NAME, condStr);

    genQueryInp.maxRows=2;

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);
    if (status == 0) {
       printGenQOut(&genQueryOut);
    }
    return(status);
}

int
doTest6(char *testString) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr1[MAX_NAME_LEN];
    char condStr2[MAX_NAME_LEN];
    int status;

    printf("dotest6\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    addInxIval (&genQueryInp.selectInp, COL_TOKEN_NAME, 1);

    snprintf (condStr1, MAX_NAME_LEN, "= 'data_type'");
    addInxVal (&genQueryInp.sqlCondInp,  COL_TOKEN_NAMESPACE, condStr1);

    /*    snprintf (condStr2, MAX_NAME_LEN, "like '%s|%s|%s'", "%", 
	  testString,"%"); */
    snprintf (condStr2, MAX_NAME_LEN, "like '%s%s%s'", "%", 
	      testString,"%");
    addInxVal (&genQueryInp.sqlCondInp,  COL_TOKEN_VALUE2, condStr2);

    genQueryInp.maxRows=1;

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);
    if (status == 0) {
       printGenQOut(&genQueryOut);
    }
    return(status);
}

int
doTest7(char *testString, char *testString2, char *testString3, 
	char *minTotalRows) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr1[MAX_NAME_LEN];
    char condStr2[MAX_NAME_LEN];
    int status, status2;

    printf("dotest7\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    addInxIval (&genQueryInp.selectInp, COL_TOKEN_NAME, 1);

    snprintf (condStr1, MAX_NAME_LEN, "= 'data_type'");
    addInxVal (&genQueryInp.sqlCondInp,  COL_TOKEN_NAMESPACE, condStr1);

    snprintf (condStr2, MAX_NAME_LEN, "like '%s%s%s'", "%", 
	      testString,"%");
    addInxVal (&genQueryInp.sqlCondInp,  COL_TOKEN_VALUE2, condStr2);

    genQueryInp.options=RETURN_TOTAL_ROW_COUNT;
    genQueryInp.maxRows=5;
    if (testString3 != NULL && *testString3!='\0') {
       genQueryInp.maxRows = atoi(testString3);
    }

    if (testString2 != NULL && *testString2!='\0') {
       genQueryInp.rowOffset = atoi(testString2);
    }

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut.totalRowCount);

    if (status == 0) {
       printGenQOut(&genQueryOut);
    }

    if (minTotalRows != NULL && *minTotalRows!='\0') {
       int minTotalRowsInt;
       minTotalRowsInt  = atoi(minTotalRows);
       if (genQueryOut.totalRowCount < minTotalRowsInt) {
	  rodsLog (LOG_ERROR, "doTest7 chlGenQuery totalRowCount(%d) is less than minimum expected (%d)\n",
		   genQueryOut.totalRowCount, minTotalRowsInt);
	  return(-1);
       }
    }

    if (genQueryOut.continueInx>0) {
       genQueryInp.continueInx = genQueryOut.continueInx;
       printf("Second call to chlGenQuery\n");
       status2  = chlGenQuery(genQueryInp, &genQueryOut);
       printf("chlGenQuery status=%d\n",status2);
       if (status2 == 0) {
	  printGenQOut(&genQueryOut);
       }
    }
    return(status);
}

int
doTest8(char *testString, char *testString2, char *testString3) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    /*    char condStr1[MAX_NAME_LEN]; */
    char condStr2[MAX_NAME_LEN];
    int status, status2;

    printf("dotest8\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

#if 0
    /* for this one: test_genq gen8 "|.o|" 0 10 */
    addInxIval (&genQueryInp.selectInp, COL_TOKEN_NAME, 1);

    snprintf (condStr1, MAX_NAME_LEN, "= 'data_type'");
    addInxVal (&genQueryInp.sqlCondInp,  COL_TOKEN_NAMESPACE, condStr1);

    snprintf (condStr2, MAX_NAME_LEN, "begin_of '%s'", testString);
    addInxVal (&genQueryInp.sqlCondInp,  COL_TOKEN_VALUE2, condStr2);
#endif

#if 1
    /* for : test_genq gen8 / 0 10          (shoule find 1)
       or :  test_genq gen8 /tempZone 0 10  (should find 2)  
       or :  test_genq gen8 abc 0 10        (should find 0)  */ 
    addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);

    snprintf (condStr2, MAX_NAME_LEN, "begin_of '%s'", testString);
    addInxVal (&genQueryInp.sqlCondInp,  COL_COLL_NAME, condStr2);
#endif

    genQueryInp.options=RETURN_TOTAL_ROW_COUNT;
    genQueryInp.maxRows=5;
    if (testString3 != NULL && *testString3!='\0') {
       genQueryInp.maxRows = atoi(testString3);
    }

    if (testString2 != NULL && *testString2!='\0') {
       genQueryInp.rowOffset = atoi(testString2);
    }

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut.totalRowCount);

    if (status == 0) {
       printGenQOut(&genQueryOut);
    }

    if (genQueryOut.continueInx>0) {
       genQueryInp.continueInx = genQueryOut.continueInx;
       printf("Second call to chlGenQuery\n");
       status2  = chlGenQuery(genQueryInp, &genQueryOut);
       printf("chlGenQuery status=%d\n",status2);
       if (status2 == 0) {
	  printGenQOut(&genQueryOut);
       }
    }
    return(status);
}

int
doTest9(char *testString, char *testString2) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr1[MAX_NAME_LEN];
    int status, status2;
    int type;

    printf("dotest9\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    type=0;
    if (testString2 != NULL) {
      if (strcmp(testString2,"min")==0) type=1;
      if (strcmp(testString2,"max")==0) type=2;
      if (strcmp(testString2,"sum")==0) type=3;
      if (strcmp(testString2,"avg")==0) type=4;
      if (strcmp(testString2,"count")==0) type=5;
    }

    if (type==0) 
       addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, 0);
    if (type==1) 
       addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, SELECT_MIN);
    if (type==2) 
       addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, SELECT_MAX);
    if (type==3) 
       addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, SELECT_SUM);
    if (type==4) 
       addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, SELECT_AVG);
    if (type==5) 
       addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, SELECT_COUNT);

    snprintf (condStr1, MAX_NAME_LEN, "= '%s'", testString);

    addInxVal (&genQueryInp.sqlCondInp,  COL_R_RESC_NAME, condStr1);

    genQueryInp.maxRows=5;

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut.totalRowCount);

    if (status == 0) {
       printGenQOut(&genQueryOut);
    }

    if (genQueryOut.continueInx>0) {
       genQueryInp.continueInx = genQueryOut.continueInx;
       printf("Second call to chlGenQuery\n");
       status2  = chlGenQuery(genQueryInp, &genQueryOut);
       printf("chlGenQuery status=%d\n",status2);
       if (status2 == 0) {
	  printGenQOut(&genQueryOut);
       }
    }
    return(status);
}

int
doTest10(char *userName, char *rodsZone, char *accessPerm, char *collection) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr[MAX_NAME_LEN];
    int status;
    char accStr[LONG_NAME_LEN];

    printf("dotest10\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    snprintf (accStr, LONG_NAME_LEN, "%s", userName);
    addKeyVal (&genQueryInp.condInput, USER_NAME_CLIENT_KW, accStr);

    snprintf (accStr, LONG_NAME_LEN, "%s", rodsZone);
    addKeyVal (&genQueryInp.condInput, RODS_ZONE_CLIENT_KW, accStr);

    snprintf (accStr, LONG_NAME_LEN, "%s", accessPerm);
    addKeyVal (&genQueryInp.condInput, ACCESS_PERMISSION_KW, accStr);

    snprintf (condStr, MAX_NAME_LEN, "='%s'", collection);
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

    addInxIval (&genQueryInp.selectInp, COL_COLL_ID, 1);

    genQueryInp.maxRows = 10;

    /*  status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut); */

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut.totalRowCount);

    if (status == 0) {
       printGenQOut(&genQueryOut);
    }

    return(0);
}

int
doTest11(char *userName, char *rodsZone, char *accessPerm, char *collection, 
	 char *dataObj) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr[MAX_NAME_LEN];
    int status;
    char accStr[LONG_NAME_LEN];

    printf("dotest11\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    snprintf (accStr, LONG_NAME_LEN, "%s", userName);
    addKeyVal (&genQueryInp.condInput, USER_NAME_CLIENT_KW, accStr);

    snprintf (accStr, LONG_NAME_LEN, "%s", rodsZone);
    addKeyVal (&genQueryInp.condInput, RODS_ZONE_CLIENT_KW, accStr);

    snprintf (accStr, LONG_NAME_LEN, "%s", accessPerm);
    addKeyVal (&genQueryInp.condInput, ACCESS_PERMISSION_KW, accStr);

    snprintf (condStr, MAX_NAME_LEN, "='%s'", collection);
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

    snprintf (condStr, MAX_NAME_LEN, "='%s'", dataObj);
    addInxVal (&genQueryInp.sqlCondInp, COL_DATA_NAME, condStr);

    addInxIval (&genQueryInp.selectInp,  COL_D_DATA_ID, 1);
 
    genQueryInp.maxRows = 10;

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut.totalRowCount);

    if (status == 0) {
       printGenQOut(&genQueryOut);
    }

    return(0);
}

int
doTest12(char *userName, char *rodsZone, char *accessPerm, char *collection) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr[MAX_NAME_LEN];
    char condStr2[MAX_NAME_LEN];
    int status;
    char accStr[LONG_NAME_LEN];
    int doAccessControlToQuery=0;

    printf("dotest12\n");
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

    /*  status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut); */

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut.totalRowCount);

    if (status == 0) {
       printGenQOut(&genQueryOut);
    }

    return(0);
}

int
doTest13(char *userName, char *rodsZone, char *accessPerm, char *collection,
	 char *fileName) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr[MAX_NAME_LEN];
    char condStr2[MAX_NAME_LEN];
    int status;
    char accStr[LONG_NAME_LEN];
    int doAccessControlToQuery=0;

    printf("dotest13\n");
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

    /*  status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut); */

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);

    printf("genQueryOut->totalRowCount=%d\n", genQueryOut.totalRowCount);

    if (status == 0) {
       printGenQOut(&genQueryOut);
    }

    return(0);
}

/* Based on doTest7, but tests AUTO_CLOSE */
int
doTest15(char *testString, char *testString2, char *testString3) {
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    char condStr1[MAX_NAME_LEN];
    char condStr2[MAX_NAME_LEN];
    int status;

    printf("dotest15\n");
    rodsLogSqlReq(1);

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    addInxIval (&genQueryInp.selectInp, COL_TOKEN_NAME, 1);

    snprintf (condStr1, MAX_NAME_LEN, "= 'data_type'");
    addInxVal (&genQueryInp.sqlCondInp,  COL_TOKEN_NAMESPACE, condStr1);

    snprintf (condStr2, MAX_NAME_LEN, "like '%s%s%s'", "%", 
	      testString,"%");
    addInxVal (&genQueryInp.sqlCondInp,  COL_TOKEN_VALUE2, condStr2);

    genQueryInp.options=AUTO_CLOSE;
    genQueryInp.maxRows=1;
    if (testString3 != NULL && *testString3!='\0') {
       genQueryInp.maxRows = atoi(testString3);
    }

    if (testString2 != NULL && *testString2!='\0') {
       genQueryInp.rowOffset = atoi(testString2);
    }

    status  = chlGenQuery(genQueryInp, &genQueryOut);
    printf("chlGenQuery status=%d\n",status);

    if (status == 0) {
       printGenQOut(&genQueryOut);
    }
    return(status);
}


int
main(int argc, char **argv) {
   int i1, i2, i3, i;
   genQueryInp_t genQueryInp;
   int i1a[10];
   int i2a[10];
   int done;
   int mode;
   char *condVal[2];
   char v1[20];
   rodsServerConfig_t serverConfig;

   /* remove this call or change to LOG_NOTICE for more verbosity */
   rodsLogLevel(LOG_ERROR);

   /* this will cause the sql to be printed, comment this out to skip it  */
   rodsLogSqlReq(1);

   i1=7;
   i2=5;

   done=0;
   mode = 0;
   if (argc >= 2) {
      if (strcmp(argv[1],"gen")==0) mode=1;
      if (strcmp(argv[1],"ls")==0) mode=2;
      if (strcmp(argv[1],"gen2")==0) mode=3;
      if (strcmp(argv[1],"gen3")==0) mode=4;
      if (strcmp(argv[1],"gen4")==0) mode=5;
      if (strcmp(argv[1],"gen5")==0) mode=6;
      if (strcmp(argv[1],"gen6")==0) mode=7;
      if (strcmp(argv[1],"gen7")==0) mode=8;
      if (strcmp(argv[1],"gen8")==0) mode=9;
      if (strcmp(argv[1],"gen9")==0) mode=10;
      if (strcmp(argv[1],"gen10")==0) mode=11;
      if (strcmp(argv[1],"gen11")==0) mode=12;
      if (strcmp(argv[1],"gen12")==0) mode=13;
      if (strcmp(argv[1],"gen13")==0) mode=14;
      if (strcmp(argv[1],"lsr")==0) mode=15;
      if (strcmp(argv[1],"gen15")==0) mode=16;
   }

   if (argc ==3 && mode==0) {
      i1 = atoi(argv[1]);
      i2 = atoi(argv[2]);
      sTest(i1, i2);
      done++;
   }

   if (argc == 4 && mode==0) {
      i1 = atoi(argv[1]);
      i2 = atoi(argv[2]);
      i3 = atoi(argv[3]);
      sTest2(i1, i2, i3);
      done++;
   }

   if (argc==2 && mode==0 ) {
      int j;
      j = atoi(argv[1]);
      if (j >= 0) {
	 printf("finding cycles starting with table %d\n",j);
	 i = findCycles(j);
	 printf("status = %d\n", i);
      }
      else {
	 printf("finding cycles for all tables\n");
	 printf("last test should be -816000 if last table finished\n");
	 i=0;
	 for (j=0;i==0;j++) {
	    i = findCycles(j);
	    printf("starting with table %d status = %d\n", j, i);
	 }
	 if (j > 70 && i == CAT_INVALID_ARGUMENT) {
	    printf(
	       "Success: Searched all tables (0 to %d) and no cycles found\n",
	       j-1);
	    exit(0);
	 } else {
	    exit(5);
	 }
      }
      done++;
   }

   memset((char*)&genQueryInp, 0, sizeof(genQueryInp));

   genQueryInp.maxRows = 10;
   genQueryInp.continueInx = 0;
   /*
     (another test case that could be used)
   i1a[0]=COL_R_RESC_NAME;
   i1a[1]=COL_R_ZONE_NAME;
   i1a[2]=COL_R_TYPE_NAME;
   i1a[3]=COL_R_CLASS_NAME;
   */
   /*   i1a[0]=COL_COLL_INHERITANCE; */
   i1a[0]=COL_COLL_NAME;

   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.len = 1;

   i2a[0]=COL_D_DATA_PATH;
   genQueryInp.sqlCondInp.inx = i2a;
   strcpy(v1, "='b'");
   condVal[0]=v1;
   genQueryInp.sqlCondInp.value = condVal;
   genQueryInp.sqlCondInp.len=1;

   if (mode==1) {
      if (argc == 3) {
	 i1a[0]=atoi(argv[2]);
      }
   }
   if (done==0) {
      int status;   
      genQueryOut_t result;
      rodsEnv myEnv;

      memset((char *)&result, 0, sizeof(result));
      memset((char *)&myEnv, 0, sizeof(myEnv));
      status = getRodsEnv (&myEnv);
      if (status < 0) {
	 rodsLog (LOG_ERROR, "main: getRodsEnv error. status = %d",
		  status);
	 exit (1);
      }


      /* This is no longer ifdef'ed GEN_QUERY_AC (since msiAclPolicy
	 now used), so just do it.   */
      chlGenQueryAccessControlSetup(myEnv.rodsUserName,
				myEnv.rodsZone,
				LOCAL_PRIV_USER_AUTH, 1);

      if (strstr(myEnv.rodsDebug, "CAT") != NULL) {
	 chlDebug(myEnv.rodsDebug);
      }

      memset(&serverConfig, 0, sizeof(serverConfig));
      status = readServerConfig(&serverConfig);
      if (status) {
         printf("Error %d from readServerConfig\n", status);
      }

      if ((status = chlOpen(serverConfig.DBUsername,
			    serverConfig.DBPassword)) != 0) {
	 rodsLog (LOG_SYS_FATAL,
		  "chlopen Error. Status = %d",
		  status);
	 return (status);
      }
      if (mode==2) {
	 /*	 doLs(); */
	 doLs2();
	 exit(0);
      }
      if (mode==3) {
	 doTest2();
	 exit(0);
      }
      if (mode==4) {
	 doTest3();
	 exit(0);
      }
      if (mode==5) {
	 doTest4();
	 exit(0);
      }
      if (mode==6) {
	 doTest5();
	 exit(0);
      }
      if (mode==7) {
	 doTest6(argv[2]);
	 exit(0);
      }

      if (mode==8) {
	 status = doTest7(argv[2], argv[3], argv[4], argv[5]);
	 if (status <0) exit(1);
	 exit(0);
      }

      if (mode==9) {
	 status = doTest8(argv[2], argv[3], argv[4]);
	 if (status <0) exit(2);
	 exit(0);
      }

      if (mode==10) {
	 status = doTest9(argv[2], argv[3]);
	 if (status <0) exit(2);
	 exit(0);
      }

      if (mode==11) {
	 status = doTest10(argv[2], argv[3], argv[4], argv[5]);
	 if (status <0) exit(2);
	 exit(0);
      }

      if (mode==12) {
	 status = doTest11(argv[2], argv[3], argv[4], argv[5], argv[6]);
	 if (status <0) exit(2);
	 exit(0);
      }
      if (mode==13) {
	 status = doTest12(argv[2], argv[3], argv[4], argv[5]);
	 if (status <0) exit(2);
	 exit(0);
      }
      if (mode==14) {
	 status = doTest13(argv[2], argv[3], argv[4], argv[5], argv[6]);
	 if (status <0) exit(2);
	 exit(0);
      }
      if (mode==15) {
	 status = doLs3(argv[2]);
	 if (status <0) exit(2);
	 exit(0);
      }
      if (mode==16) {
	 status = doTest15(argv[2], argv[3], argv[4]);
	 if (status <0) exit(2);
	 exit(0);
      }

      genQueryInp.maxRows=2;
      i = chlGenQuery(genQueryInp, &result);
      printf("chlGenQuery status=%d\n",i);
      printf("result.rowCnt=%d\n", result.rowCnt);
      if (result.rowCnt > 0) {
	 int i;
	 for (i=0;i<result.rowCnt;i++) {
	    printf("result.SqlResult[%d].value=%s\n",i,
		   result.sqlResult[i].value);
	 }
      }
   }
   exit(0);
}
