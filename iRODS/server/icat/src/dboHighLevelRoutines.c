/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**************************************************************************

  This file contains the DBO (database objects)/DBR (database
  resource) high level functions.  These constitute the API between
  the rest of iRODS and the DBR databases, accessed via DBOs.

  See the 'Database Resources' page on the irods web site for more
  information.

  These routines, like the icatHighLevelRoutines, layer on top of
  either icatLowLevelOdbc or icatLowLevelOracle.  DBR is not ICAT,
  but they both use the shared low level interface to the databases.
  DBR can be built as part of either an ICAT-enabled server or a
  non-ICAT-Enabled server.

**************************************************************************/

#include "rods.h"
#include "rcMisc.h"

#include "dboHighLevelRoutines.h"
#include "icatLowLevel.h"

#include "dataObjOpen.h"
#include "dataObjRead.h"
#include "dataObjClose.h"
#include "dataObjCreate.h"
#include "dataObjWrite.h"

#define DBR_CONFIG_FILE "dbr.config"
#define DBR_ODBC_ENTRY_PREFIX "IRODS_DBR_"
#define MAX_ODBC_ENTRY_NAME 100

#define DBR_ACCESS_READ  1
#define DBR_ACCESS_WRITE 2
#define DBR_ACCESS_OWN   3

#define BUF_LEN 500
#define MAX_SQL 4000

#define MAX_DBR_NAME_LEN 200
#define BIG_STR 200

#define LOCAL_BUFFER_SIZE 1000000
#define MAX_SESSIONS 10
static char openDbrName[MAX_SESSIONS][MAX_DBR_NAME_LEN+2]={"","","","","","","","","",""};

int dboLogSQL=0;

int readDbrConfig(char *dbrname, char *DBUser, char *DBPasswd,char *DBType);

icatSessionStruct dbr_icss[MAX_SESSIONS]={{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}};

int dboDebug(char *debugMode) {
   if (strstr(debugMode, "SQL")) {
      dboLogSQL=1;
   }
   else {
      dboLogSQL=0;
   }
   return(0);
}

int
getOpenDbrIndex(char *dbrName) {
   int ix, i;
   ix=-1;
   for (i=0;i<MAX_SESSIONS;i++) {
      if (strcmp(openDbrName[i], dbrName)==0) {
	 ix = i;
	 break;
      }
   }
   return(ix);
}


int dbrOpen(char *dbrName) {
#if defined(DBR) 
   int i;
   int status;
   char DBType[DB_TYPENAME_LEN];
   int icss_index;
   char odbcEntryName[MAX_ODBC_ENTRY_NAME+10];

   i = getOpenDbrIndex(dbrName);
   if (i>=0) return(DBR_ALREADY_OPEN);

   icss_index = -1;
   for (i=0;i<MAX_SESSIONS;i++) {
      if (dbr_icss[i].status==0) {
	 icss_index=i;
	 break;
      }
   }
   if (icss_index==-1) return (DBR_MAX_SESSIONS_REACHED);

   status =  readDbrConfig(dbrName, 
			   dbr_icss[icss_index].databaseUsername,
			   dbr_icss[icss_index].databasePassword,
			   DBType);
   if (status) return(status);

   dbr_icss[icss_index].databaseType = DB_TYPE_POSTGRES;
   if (strcmp(DBType, "oracle")==0) {
      dbr_icss[icss_index].databaseType = DB_TYPE_ORACLE;
   }
   if (strcmp(DBType, "mysql")==0) {
      dbr_icss[icss_index].databaseType = DB_TYPE_MYSQL;
   }

   /* Initialize the dbr_icss statement pointers */
   for (i=0; i<MAX_NUM_OF_CONCURRENT_STMTS; i++) {
      dbr_icss[icss_index].stmtPtr[i]=0;
   }

   /* Open Environment */
   i = cllOpenEnv(&dbr_icss[icss_index]);
   if (i != 0) {
      rodsLog(LOG_NOTICE, "dbrOpen cllOpen failure %d",i);
      return(DBR_ENV_ERR);
   }

   /* Connect to the DBMS */
   rstrcpy((char *)&odbcEntryName, DBR_ODBC_ENTRY_PREFIX, 
	   MAX_ODBC_ENTRY_NAME);
   rstrcat((char *)&odbcEntryName, dbrName,
	   MAX_ODBC_ENTRY_NAME);
   i = cllConnectDbr(&dbr_icss[icss_index], odbcEntryName);
   if (i != 0) {
      rodsLog(LOG_NOTICE, "dbrOpen cllConnectDbr failure %d",i);
      return(DBR_CONNECT_ERR);
   }

   dbr_icss[icss_index].status=1;
   rstrcpy(openDbrName[icss_index], dbrName, MAX_DBR_NAME_LEN);

   return(icss_index);
#else
   openDbrName[0][0]='\0'; /* avoid warning */
   return(DBR_NOT_COMPILED_IN);
#endif
}


int _dbrClose(int icss_index) {
#if defined(DBR) 
   int status, stat2;

   status = cllDisconnect(&dbr_icss[icss_index]);

   stat2 = cllCloseEnv(&dbr_icss[icss_index]);

   openDbrName[icss_index][0]='\0';

   if (status) {
      return(DBR_DISCONNECT_ERR);
   }
   if (stat2) {
      return(DBR_CLOSE_ENV_ERR);
   }
   dbr_icss[icss_index].status=0;
   openDbrName[icss_index][0] = '\0';
   return(0);
#else
   return(DBR_NOT_COMPILED_IN);
#endif
}

int dbrClose(char *dbrName) {
#if defined(DBR) 
   int i;
   for (i=0;i<MAX_SESSIONS;i++) {
      if (strcmp(openDbrName[i], dbrName)==0) {
	 return(_dbrClose(i));
      }
   }
   return(DBR_NOT_OPEN);
#else
   return(DBR_NOT_COMPILED_IN);
#endif
}

int dbrCommit(rsComm_t *rsComm, char *dbrName) {
#if defined(DBR) 
   int status, ix;

   if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
      return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
   }

   ix = getOpenDbrIndex(dbrName);

   if (ix<0) return(DBR_NOT_OPEN);

   status = cllExecSqlNoResult(&dbr_icss[ix], "commit");
   return(status);
#else
   return(DBR_NOT_COMPILED_IN);
#endif
}

int dbrRollback(rsComm_t *rsComm, char *dbrName) {
#if defined(DBR) 
   int status, ix;

   if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
      return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
   }

   ix = getOpenDbrIndex(dbrName);

   if (ix<0) return(DBR_NOT_OPEN);

   status = cllExecSqlNoResult(&dbr_icss[ix], "rollback");
   return(status);
#else
   return(DBR_NOT_COMPILED_IN);
#endif
}

int dboIsConnected(int icss_index) {
#if defined(DBR) 
   if (dboLogSQL) rodsLog(LOG_SQL, "dboIsConnected");
   return(dbr_icss[icss_index].status);
#else
   return(DBR_NOT_COMPILED_IN);
#endif
}

/*
Via a general query, get the user's access to the DBR
*/
int
checkAccessToDBR(rsComm_t *rsComm, char *dbrName)
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int iAttr[10];
   int iAttrVal[10]={0,0,0,0,0};
   int iCond[10];
   char *condVal[10];
   char v1[BIG_STR];
   char v2[BIG_STR];
   char v3[BIG_STR];
   int status, i, j;

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   i=0;
   iAttr[i++]=COL_RESC_ACCESS_NAME;
   
   genQueryInp.selectInp.inx = iAttr;
   genQueryInp.selectInp.value = iAttrVal;
   genQueryInp.selectInp.len = i;

   genQueryInp.sqlCondInp.inx = iCond;
   genQueryInp.sqlCondInp.value = condVal;

   i=0;
   iCond[i]=COL_R_RESC_NAME;
   sprintf(v1,"='%s'",dbrName);
   condVal[i++]=v1;

   iCond[i]=COL_RESC_USER_NAME;
   sprintf(v2,"='%s'", rsComm->clientUser.userName);
   condVal[i++]=v2;

   iCond[i]=COL_RESC_USER_ZONE;
   sprintf(v3,"='%s'", rsComm->clientUser.rodsZone);
   condVal[i++]=v3;

   genQueryInp.sqlCondInp.len=i;

   genQueryInp.maxRows=50;
   genQueryInp.continueInx=0;
   status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
   if (status == CAT_NO_ROWS_FOUND) return(0);
   if (status) return(status);

   for (i=0;i<genQueryOut->rowCnt;i++) {
      for (j=0;j<genQueryOut->attriCnt;j++) {
	 char *tResult;
	 tResult = genQueryOut->sqlResult[j].value;
	 tResult += i*genQueryOut->sqlResult[j].len;
	 printf("%s ",tResult);
	 if (strcmp(tResult,"own")==0) return(DBR_ACCESS_OWN);
	 if (strcmp(tResult,"modify object")==0) return(DBR_ACCESS_WRITE);
	 if (strcmp(tResult,"read object")==0) return(DBR_ACCESS_READ);
      }
      printf("\n");
   }
   return (0);
}

/*
Via a general query, check that the DBO is of the right type (access
will be checked when the data-object is opened for reading)
*/
int
checkDBOType(rsComm_t *rsComm, char *dboName)
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int iAttr[10];
   int iAttrVal[10]={0,0,0,0,0};
   int iCond[10];
   char *condVal[10];
   char v1[BIG_STR];
   char v2[BIG_STR];
   char v3[BIG_STR];
   int status, i;
   char logicalFileName[MAX_NAME_LEN];
   char logicalDirName[MAX_NAME_LEN];

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   i=0;
   iAttr[i++]=COL_D_DATA_ID;
   
   genQueryInp.selectInp.inx = iAttr;
   genQueryInp.selectInp.value = iAttrVal;
   genQueryInp.selectInp.len = i;

   genQueryInp.sqlCondInp.inx = iCond;
   genQueryInp.sqlCondInp.value = condVal;

   status = splitPathByKey(dboName,
			   logicalDirName, logicalFileName, '/');

   i=0;
   iCond[i]=COL_DATA_TYPE_NAME;
   sprintf(v1,"='%s'","database object");
   condVal[i++]=v1;

   iCond[i]=COL_DATA_NAME;
   sprintf(v2,"='%s'", logicalFileName);
   condVal[i++]=v2;

   iCond[i]=COL_COLL_NAME;
   sprintf(v3,"='%s'", logicalDirName);
   condVal[i++]=v3;

   genQueryInp.sqlCondInp.len=i;

   genQueryInp.maxRows=50;
   genQueryInp.continueInx=0;
   status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
   if (status == CAT_NO_ROWS_FOUND) return(DBO_NOT_VALID_DATATYPE);
   return(status);
}


/*
  Check the access permissions to the DBR and DBO.
 */
int checkDBOOwner(rsComm_t *rsComm, char *dbrName, char *dboName) {
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   genQueryInp_t genQueryInp2;
   genQueryOut_t *genQueryOut2;
   int iAttr[10];
   int iAttrVal[10]={0,0,0,0,0};
   int iCond[10];
   char *condVal[10];
   char v1[BIG_STR];
   char v2[BIG_STR];
   char v3[BIG_STR];
   int status, i, j;
   char logicalFileName[MAX_NAME_LEN];
   char logicalDirName[MAX_NAME_LEN];

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   i=0;
   iAttr[i++]=COL_DATA_ACCESS_USER_ID;
   
   genQueryInp.selectInp.inx = iAttr;
   genQueryInp.selectInp.value = iAttrVal;
   genQueryInp.selectInp.len = i;

   status = splitPathByKey(dboName,
			   logicalDirName, logicalFileName, '/');

   i=0;
   iCond[i]=COL_DATA_ACCESS_NAME;
   sprintf(v1,"= '%s' || = '%s'","own", "modify object");
   condVal[i++]=v1;

   iCond[i]=COL_DATA_NAME;
   sprintf(v2,"='%s'", logicalFileName);
   condVal[i++]=v2;

   iCond[i]=COL_COLL_NAME;
   sprintf(v3,"='%s'", logicalDirName);
   condVal[i++]=v3;

   genQueryInp.sqlCondInp.len=i;
   genQueryInp.sqlCondInp.inx = iCond;
   genQueryInp.sqlCondInp.value = condVal;

   genQueryInp.maxRows=20;
   genQueryInp.continueInx=0;
   genQueryInp.condInput.len=0;
   status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
   if (status) return(status);
   if (genQueryOut->continueInx != 0) return(DBR_WRITABLE_BY_TOO_MANY);

   for (i=0;i<genQueryOut->rowCnt;i++) {
       printf("DEBUGy");
      for (j=0;j<genQueryOut->attriCnt;j++) {
	 char *tResult;
	 tResult = genQueryOut->sqlResult[j].value;
	 tResult += i*genQueryOut->sqlResult[j].len;
	 printf("%s ",tResult);
      }
      printf("\n");
   }


/* Now find all users with write access to the DBR */
   memset (&genQueryInp2, 0, sizeof (genQueryInp_t));

   i=0;
   iAttr[i++]=COL_RESC_ACCESS_USER_ID;
   
   genQueryInp2.selectInp.inx = iAttr;
   genQueryInp2.selectInp.value = iAttrVal;
   genQueryInp2.selectInp.len = i;

   i=0;
   iCond[i]=COL_RESC_ACCESS_NAME;
   sprintf(v1,"= '%s'", "modify object");
   condVal[i++]=v1;

   iCond[i]=COL_R_RESC_NAME;
   sprintf(v2,"='%s'", dbrName);
   condVal[i++]=v2;

   genQueryInp2.sqlCondInp.len=i;
   genQueryInp2.sqlCondInp.inx = iCond;
   genQueryInp2.sqlCondInp.value = condVal;

   genQueryInp2.maxRows=20;
   genQueryInp2.continueInx=0;
   genQueryInp2.condInput.len=0;
   status = rsGenQuery(rsComm, &genQueryInp2, &genQueryOut2);

   if (status) return(status);
   if (genQueryOut2->continueInx != 0) return(DBR_WRITABLE_BY_TOO_MANY);

#if 0
   for (i=0;i<genQueryOut2->rowCnt;i++) {
      for (j=0;j<genQueryOut->attriCnt;j++) {
	 char *tResult;
	 tResult = genQueryOut->sqlResult[j].value;
	 tResult += i*genQueryOut->sqlResult[j].len;
	 printf("DEBUG2: %s ",tResult);
      }
      printf("\n");
   }
#endif

   /* if more users can write to the DBO than can write to the DBR
      then the DBO access is clearly too open */
   if (genQueryOut->rowCnt > genQueryOut2->rowCnt) 
      return(DBO_WRITABLE_BY_NON_PRIVILEGED);

   /* Each user that can write to the DBO must be in the list of those
      who can write to the DBR */
   for (i=0;i<genQueryOut->rowCnt;i++) {
      int OK=0;
      for (j=0;j<genQueryOut2->rowCnt;j++) {
	 if (strcmp(genQueryOut->sqlResult[i].value,
		    genQueryOut2->sqlResult[j].value) == 0) OK=1;
      }
      if (OK==0) return(DBO_WRITABLE_BY_NON_PRIVILEGED);
   }
   return (status);  /* any error, no access */
}


int dboSqlNoResults(char *sql, char *parm[], int nParms) {
#if defined(DBO_NOTYET) 
   int i;
   for (i=0;i<nParms;i++) {
      cllBindVars[i]=parm[i];
   }
   cllBindVarCount=nParms;
   i = cllExecSqlNoResult(&dbr_icss, sql);
   /*   if (i <= CAT_ENV_ERR) return(i); ? already an iRODS error code */
   printf("i=%d\n",i);
   if (i==CAT_SUCCESS_BUT_WITH_NO_INFO) return(0);
   if (i) return(DBO_SQL_ERR);
   return(0);
#else
   return(DBR_NOT_COMPILED_IN);
#endif
}

int dboSqlWithResults(int fd, char *sql, char *sqlFormat, char *args[10],
		      char *outBuf, int maxOutBuf) {
#if defined(DBR) 
   int i, ii;
   int statement;
   int rowCount, nCols;
   int csvMode=0;

   csvMode=0;
   if (strcmp(sqlFormat, "csv")==0) csvMode=1;
   if (strcmp(sqlFormat, "CSV")==0) csvMode=1;

   for (i=0;i<10;i++) {
      if (args[i]==NULL || strlen(args[i])==0) break;
      cllBindVars[i]=args[i];
   }
   cllBindVarCount=i;

   i = cllExecSqlWithResult(&dbr_icss[fd], &statement, sql);
   if (i==CAT_SUCCESS_BUT_WITH_NO_INFO) return(CAT_SUCCESS_BUT_WITH_NO_INFO);
   if (i) {
      cllGetLastErrorMessage(outBuf, maxOutBuf);
      if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
      return (CAT_SQL_ERR);
   }

   for (rowCount=0;;rowCount++) {
      i = cllGetRow(&dbr_icss[fd], statement);
      if (i != 0)  {
	 ii = cllFreeStatement(&dbr_icss[fd], statement);
	 if (rowCount==0) return(CAT_GET_ROW_ERR);
	 return(0);
      }

      if (dbr_icss[fd].stmtPtr[statement]->numOfCols == 0) {
	 i = cllFreeStatement(&dbr_icss[fd],statement);
	 if (rowCount==0) return(CAT_NO_ROWS_FOUND);
	 if (!csvMode) {
	    rstrcat(outBuf, "\n<\\rows>\n", maxOutBuf);
	 }
	 else {
	    rstrcat(outBuf, "\n", maxOutBuf);	    
	 }
	 return(0);
      }

      nCols = dbr_icss[fd].stmtPtr[statement]->numOfCols;
      if (rowCount==0) {
	 if (!csvMode) rstrcat(outBuf, "<column_descriptions>\n", maxOutBuf);
	 for (i=0; i<nCols ; i++ ) {
	    rstrcat(outBuf, dbr_icss[fd].stmtPtr[statement]->resultColName[i],
		    maxOutBuf);
	    if (csvMode) {
	       if (i<nCols-1) {
		  rstrcat(outBuf, ",", maxOutBuf);
	       }
	       else {
		  rstrcat(outBuf, "\n", maxOutBuf);
	       }
	    }
	    else {
	       rstrcat(outBuf, "|", maxOutBuf);
	    }
	 }
	 if (!csvMode) {
	    rstrcat(outBuf, "\n<\\column_descriptions>\n<rows>\n", maxOutBuf);
	 }
      }
      if (rowCount>0) rstrcat(outBuf, "\n", maxOutBuf);
      if (nCols>0 && nCols<11 && sqlFormat!=NULL && *sqlFormat!='\0'
	 && csvMode==0 ) {
	 char line1[1000];
	 if (nCols==1) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0]);
	 }
	 if (nCols==2) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1]);
	 }
	 if (nCols==3) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[2]);
	 }
	 if (nCols==4) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[2],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[3]);
	 }
	 if (nCols==5) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[2],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[3],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[4]);
	 }
	 if (nCols==6) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[2],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[3],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[4],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[5]);
	 }
	 if (nCols==7) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[2],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[3],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[4],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[5],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[6]);
	 }
	 if (nCols==8) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[2],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[3],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[4],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[5],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[6],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[7]);
	 }
	 if (nCols==9) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[2],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[3],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[4],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[5],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[6],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[7],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[8]);
	 }
	 if (nCols==10) {
	    snprintf(line1, sizeof(line1), sqlFormat, 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[0], 
		     dbr_icss[fd].stmtPtr[statement]->resultValue[1],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[2],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[3],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[4],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[5],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[6],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[7],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[8],
		     dbr_icss[fd].stmtPtr[statement]->resultValue[9]);
	 }
	 rstrcat(outBuf, line1, maxOutBuf);
      }
      else {
	 for (i=0; i<nCols ; i++ ) {
	    rstrcat(outBuf, dbr_icss[fd].stmtPtr[statement]->resultValue[i], 
		    maxOutBuf);
	    if (csvMode) {
	       if (i<nCols-1) {
		  rstrcat(outBuf, ",", maxOutBuf);
	       }
	    }
	    else {
	       rstrcat(outBuf, "|", maxOutBuf);
	    }
	 }
      }
   }

   return(0);  /* never reached */
#else
   return(DBR_NOT_COMPILED_IN);
#endif
}

char *
getDbrConfigDir()
{
    char *myDir;

    if ((myDir = (char *) getenv("irodsConfigDir")) != (char *) NULL) {
        return (myDir);
    }
    return (DEF_CONFIG_DIR);
}

int 
readDbrConfig(char *dboName, char *DBUser, char *DBPasswd, char *DBType) {
   FILE *fptr;
   char buf[BUF_LEN];
   char *fchar;
   char *key;
   char *dboConfigFile;
   char foundLine[BUF_LEN];
   int state, i;
   char *DBKey=0;
   int f1=0, f2=0, f3=0, f4=0;

   dboConfigFile =  (char *) malloc((strlen (getDbrConfigDir()) +
				    strlen(DBR_CONFIG_FILE) + 24));

   sprintf (dboConfigFile, "%s/%s", getDbrConfigDir(), 
	    DBR_CONFIG_FILE);

   fptr = fopen (dboConfigFile, "r");

   if (fptr == NULL) {
      rodsLog (LOG_NOTICE, 
	       "Cannot open DBR_CONFIG_FILE file %s. errno = %d\n",
          dboConfigFile, errno);
      free (dboConfigFile);
      return (DBR_CONFIG_FILE_ERR);
   }
   free (dboConfigFile);

   buf[BUF_LEN-1]='\0';
   fchar = fgets(buf, BUF_LEN-1, fptr);
   for(;fchar!='\0';) {
      if (buf[0]=='#' || buf[0]=='/') {
	 buf[0]='\0'; /* Comment line, ignore */
      }
      key=strstr(buf, dboName);
      if (key == buf) {
	 rstrcpy(foundLine, buf, BUF_LEN);
	 state=0;
	 for (i=0;i<BUF_LEN;i++) {
	    if (foundLine[i]==' ' || foundLine[i]=='\n') {
	       int endOfLine;
	       endOfLine=0;
	       if (foundLine[i]=='\n') endOfLine=1;
	       foundLine[i]='\0';
	       if (endOfLine && state<8) break;
	       if (state==0) state=1;
	       if (state==2) state=3;
	       if (state==4) state=5;
	       if (state==6) state=7;
	       if (state==8) {
		  break;
	       }
	    }
	    else {
	       if (state==1) {
		  state=2;
		  f1 = i;
	       }
	       if (state==3) {
		  state=4;
		  f2 = i;
	       }
	       if (state==5) {
		  state=6;
		  f3 = i;
	       }
	       if (state==7) {
		  state=8;
		  f4 = i;
	       }
	    }
	 }
	 if (f4>0) {
	    static char unscrambledPw[DB_PASSWORD_LEN];
	    DBKey=&foundLine[f4];
	    obfDecodeByKey(DBPasswd, DBKey, unscrambledPw);
	    strncpy(DBPasswd,unscrambledPw,DB_PASSWORD_LEN);
	 }
	 if (f3>0) {
	    strncpy(DBType,&foundLine[i],DB_TYPENAME_LEN);
	 }
	 if (f2>0) {
	    strncpy(DBPasswd,&foundLine[f2],DB_PASSWORD_LEN);
	 }
	 if (f1>0) {
	    strncpy(DBUser,&foundLine[f1],DB_USERNAME_LEN);
	    fclose (fptr);
	    return(0);
	 }
      }
      fchar = fgets(buf, BUF_LEN-1, fptr);
   }
   fclose (fptr);

   return(DBR_NAME_NOT_FOUND);
}

int
getDboSql( rsComm_t *rsComm, char *fullName, char *dboSQL, char *dboFormat) {
   openedDataObjInp_t dataObjReadInp;
   openedDataObjInp_t dataObjCloseInp;
   bytesBuf_t *readBuf;
   int status;
   int objFD;
   char *cp1, *cp2, *cp3;
   int bytesRead;
   dataObjInp_t dataObjInp;

   memset (&dataObjInp, 0, sizeof(dataObjInp_t));
   rstrcpy(dataObjInp.objPath, fullName, MAX_NAME_LEN);

   if ((objFD=rsDataObjOpen(rsComm, &dataObjInp)) < 0) {
      int i;
      if (objFD == CAT_NO_ROWS_FOUND) return (DBO_DOES_NOT_EXIST);
      if (objFD==CAT_NO_ACCESS_PERMISSION) {
	 i = addRErrorMsg (&rsComm->rError, 0, 
			   "You do not have read access to the DBO.");
      }
      return (objFD);
   }

   /* read buffer init */
   readBuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
   readBuf->len = 5*1024;	        /* just 5K should do it */
   readBuf->buf = (char *)malloc(readBuf->len);
   memset (readBuf->buf, '\0', readBuf->len);

  /* read SQL data */
   memset (&dataObjReadInp, 0, sizeof (dataObjReadInp));
   dataObjReadInp.l1descInx = objFD;
   dataObjReadInp.len = readBuf->len;

   bytesRead = rsDataObjRead (rsComm, &dataObjReadInp, readBuf);
   if (bytesRead < 0) return(bytesRead);
   

   cp1 = (char*)readBuf->buf;

   while (*cp1 == '!' || *cp1 == '#') {
      cp1++;
      while (*cp1 != '\n') cp1++;
      cp1++;
   }
   cp2=cp1;
   while (*cp2 != '\n' && *cp2 != '\0') cp2++;
   if (*cp2=='\0') {
      rstrcpy(dboSQL, cp1, MAX_SQL);
      *dboFormat='\0';
   }
   else {
      *cp2='\0';
      rstrcpy(dboSQL, cp1, MAX_SQL);
      cp2++;
      cp3=cp2;
      while (*cp3 != '\n' && *cp3 != '\0') cp3++;
      *cp3='\0';
      rstrcpy(dboFormat, cp2, MAX_SQL);
   }

   memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
   dataObjCloseInp.l1descInx = objFD;
	
   free(readBuf);

   status = rsDataObjClose (rsComm, &dataObjCloseInp);
   if (status) return(status);

   return(0);
}

void
printSqlAndArgsEtc(char *dbrName, 
		   char *dboSQL, char *args[10], char *outBuf, int maxOutBuf) {
   int i;
   rstrcpy(outBuf, "<dbr>", maxOutBuf);
   rstrcat(outBuf, dbrName, maxOutBuf);
   rstrcat(outBuf, "</dbr>\n", maxOutBuf);
   rstrcat(outBuf, "<sql>", maxOutBuf);
   rstrcat(outBuf, dboSQL, maxOutBuf);
   rstrcat(outBuf, "</sql>\n", maxOutBuf);
   for (i=0;i<10;i++) {
      if (args[i]==NULL || strlen(args[i])==0) break;
      rstrcat(outBuf, "<arg>", maxOutBuf);
      rstrcat(outBuf, args[i], maxOutBuf);
      rstrcat(outBuf, "</arg>\n", maxOutBuf);
   }
}

int
openDBOR(rsComm_t *rsComm, char *dborName, int forceOption) {
    dataObjInp_t myDataObjInp;
    int status;

    memset (&myDataObjInp, 0, sizeof(dataObjInp_t));
    rstrcpy(myDataObjInp.objPath, dborName, MAX_NAME_LEN);
    if (forceOption) {
       addKeyVal (&myDataObjInp.condInput, FORCE_FLAG_KW, "");
    }
    status = rsDataObjCreate (rsComm, &myDataObjInp);
    return(status);
}

int
dboExecute(rsComm_t *rsComm, char *dbrName, char *dboName, 
	   char *dborName, int dborOption, 
	   char *outBuf,
	   int maxOutBuf, char *args[10]) {
   int status;
   char dboSQL[MAX_SQL];
   char dboFormat[MAX_SQL];
   int i, ix;
   int outBufStrLen;
   int didOpen=0;
   int dbrAccess;
   int outDesc;
   char *myOutBuf;
   int myMaxOutBuf;
   int csvMode;

   myOutBuf = outBuf;
   myMaxOutBuf = maxOutBuf;

   status = checkDBOType(rsComm, dboName);
   if (status==DBO_NOT_VALID_DATATYPE) {
      int i;
      i = addRErrorMsg (&rsComm->rError, 0, 
          "Either the DBO does not exist or does not have a data-type\nof 'database object'.  The owner of the DBO can set the datatype via 'isysmeta'.\nSee 'isysmeta -h'.");
   }
   if (status) return(status);

   dbrAccess = checkAccessToDBR(rsComm, dbrName);
   if (dbrAccess < 0) return(dbrAccess);

   if (dbrAccess < DBR_ACCESS_WRITE) {
      if (dbrAccess == DBR_ACCESS_READ) {
	 status = checkDBOOwner(rsComm, dbrName, dboName);
	 if (status==DBO_WRITABLE_BY_NON_PRIVILEGED) {
	    int i;
	    i = addRErrorMsg (&rsComm->rError, 0, 
		"The DBO is not secure.  Some users who can write to the DBO do not have write permission to the DBR.");
	 }
	 if (status) return(status);
      }
      else {
	 return (DBR_ACCESS_PROHIBITED);
      }
   }

   if (dborName != NULL && *dborName!='\0') {
      outDesc=openDBOR(rsComm, dborName, dborOption);
      if (outDesc < 0) return(outDesc);

      /* Use a larger buffer when output will be going to a DBOR.
         Sometime, we should also modify the buffering logic so that
         any size response can be handled; getting more rows after the
         buffer is flushed; but this will handle fairly large sets of
         rows. */
      myMaxOutBuf = LOCAL_BUFFER_SIZE;
      myOutBuf=(char *)malloc(myMaxOutBuf);
   }
   else {
      outDesc = -1;
   }

   ix=getOpenDbrIndex(dbrName);

   if (ix < 0) {
      ix = dbrOpen(dbrName);
      if (ix) { free( myOutBuf ); return(ix); } // JMC cppcheck - leak
      didOpen=1;
   }

   status = getDboSql(rsComm, dboName, dboSQL, dboFormat);
   if (status) { free( myOutBuf ); return(status); } // JMC cppcheck - leak

   if (dboLogSQL) rodsLog(LOG_SQL, "dboExecute SQL: %s\n", dboSQL);

   csvMode=0;
   if (strcmp(dboFormat, "csv")==0) csvMode=1;
   if (strcmp(dboFormat, "CSV")==0) csvMode=1;

   if (!csvMode) {
      printSqlAndArgsEtc(dbrName, dboSQL, args, myOutBuf, myMaxOutBuf);
   }

   outBufStrLen = strlen(myOutBuf);

   status =  dboSqlWithResults(ix, dboSQL, dboFormat, args,
		       myOutBuf+outBufStrLen, myMaxOutBuf-outBufStrLen-1);

   if (status) {
      if (didOpen) {  /* DBR was not originally open */
	 i = dbrClose(dbrName);
      }
      return(status);
   }

   if (outDesc > 0) {
      openedDataObjInp_t dataObjWriteInp;
      int bytesWritten;
      openedDataObjInp_t dataObjCloseInp;

      bytesBuf_t *writeBuf;
      writeBuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
      outBufStrLen = strlen(myOutBuf);
      writeBuf->len = outBufStrLen;
      writeBuf->buf = myOutBuf;

      memset (&dataObjWriteInp, 0, sizeof (dataObjWriteInp));
      dataObjWriteInp.l1descInx = outDesc;
      dataObjWriteInp.len = outBufStrLen;

      bytesWritten = rsDataObjWrite(rsComm, &dataObjWriteInp, writeBuf);
      if (bytesWritten < 0) return(bytesWritten);

      if (bytesWritten != outBufStrLen) {
/*  */
      }

      memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
      dataObjCloseInp.l1descInx = outDesc;
      status = rsDataObjClose (rsComm, &dataObjCloseInp);
      if (status) return(status);

      rstrcpy(outBuf, "Output written to ", maxOutBuf);
      rstrcat(outBuf, dborName, maxOutBuf);
      rstrcat(outBuf, "\n", maxOutBuf);

      free(myOutBuf);
      free(writeBuf);
   }

   if (didOpen) {  /* DBR was not originally open */
      i = dbrClose(dbrName);
      if (i) return(i);
   }
   return(0);
}
