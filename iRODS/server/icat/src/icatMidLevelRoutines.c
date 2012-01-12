/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/**************************************************************************

  This file contains midLevel functions that can be used to get
  information from the ICAT database. These functions should not have
  any intelligence encoded in them.  These functions use the 'internal
  interface to database' library calls to access the ICAT
  database. Hence these functions can be viewed as providing higher
  level calls to the other routines. These functions do not call any
  of the high-level functions, but do sometimes call each other as well
  as the low-level functions.

**************************************************************************/


#include "icatMidLevelRoutines.h"
#include "icatLowLevel.h"
#include "icatMidLevelHelpers.h"

#include "rcMisc.h"

/* Size of the R_OBJT_AUDIT comment field;must match table column definition */
#define AUDIT_COMMENT_MAX_SIZE       1000

int logSQL_CML=0;
int auditEnabled=0;  /* Set this to 2 and rebuild to enable iRODS
                        auditing (non-zero means auditing but 1 will
                        allow cmlDebug to modify it, so 2 means
                        permanently enabled).  We plan to change this
                        sometime to have better control. */

int cmlDebug(int mode) {
   logSQL_CML = mode;
   if (mode > 1) {
      if (auditEnabled==0) auditEnabled=1; 
      /* This is needed for testing each sql form, which is needed for
	 the 'irodsctl devtest' to pass */
   }
   else {
      if (auditEnabled==1) auditEnabled=0;
   }
   return(0);
}

int cmlOpen( icatSessionStruct *icss) {
   int i;

   /* Initialize the icss statement pointers */
   for (i=0; i<MAX_NUM_OF_CONCURRENT_STMTS; i++) {
      icss->stmtPtr[i]=0;
   }

/*
 Set the ICAT DBMS type.  The Low Level now uses this instead of the
 ifdefs so it can interact with either at the same time (for the
 DBR/DBO feature).
 */
   icss->databaseType = DB_TYPE_POSTGRES;
#ifdef ORA_ICAT
   icss->databaseType = DB_TYPE_ORACLE;
#endif
#ifdef MY_ICAT
   icss->databaseType = DB_TYPE_MYSQL;
#endif


   /* Open Environment */
   i = cllOpenEnv(icss);
   if (i != 0) return(CAT_ENV_ERR);

   /* Connect to the DBMS */
   i = cllConnect(icss);
   if (i != 0) return(CAT_CONNECT_ERR);

   return(0);
}

int cmlClose( icatSessionStruct *icss) {
   int status, stat2;
   static int pending=0;

   if (pending==1) return(0); /* avoid hang if stuck doing this */
   pending=1;

   status = cllDisconnect(icss);

   stat2 = cllCloseEnv(icss);

   pending=0;
   if (status) {
      return(CAT_DISCONNECT_ERR);
   }
   if (stat2) {
      return(CAT_CLOSE_ENV_ERR);
   }
   return(0);
}


int cmlExecuteNoAnswerSql( char *sql, 
			   icatSessionStruct *icss)
{
  int i;
  
  i = cllExecSqlNoResult(icss, sql);
  if (i) { 
     if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
     return(CAT_SQL_ERR);
  }
  return(0);

}

int cmlGetOneRowFromSqlBV (char *sql, 
		   char *cVal[], 
		   int cValSize[], 
		   int numOfCols,
		   char *bindVar1,
		   char *bindVar2,
		   char *bindVar3,
		   char *bindVar4,
		   char *bindVar5,
		   icatSessionStruct *icss)
{
    int i,j, stmtNum, ii;
    
    i = cllExecSqlWithResultBV(icss, &stmtNum, sql,
			       bindVar1,bindVar2,bindVar3,bindVar4,
			       bindVar5,0);
    if (i != 0) {
      if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
      return (CAT_SQL_ERR);
    }
    i = cllGetRow(icss,stmtNum);
    if (i != 0)  {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_GET_ROW_ERR);
    }
    if (icss->stmtPtr[stmtNum]->numOfCols == 0) {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_NO_ROWS_FOUND);
    }
    for (j = 0; j < numOfCols && j < icss->stmtPtr[stmtNum]->numOfCols ; j++ ) 
      rstrcpy(cVal[j],icss->stmtPtr[stmtNum]->resultValue[j],cValSize[j]);

    i = cllFreeStatement(icss,stmtNum);
    return(j);

}

int cmlGetOneRowFromSql (char *sql, 
		   char *cVal[], 
		   int cValSize[], 
		   int numOfCols,
		   icatSessionStruct *icss)
{
    int i,j, stmtNum, ii;
    
    i = cllExecSqlWithResultBV(icss, &stmtNum, sql,
				 0,0,0,0,0,0);
    if (i != 0) {
      if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
      return (CAT_SQL_ERR);
    }
    i = cllGetRow(icss,stmtNum);
    if (i != 0)  {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_GET_ROW_ERR);
    }
    if (icss->stmtPtr[stmtNum]->numOfCols == 0) {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_NO_ROWS_FOUND);
    }
    for (j = 0; j < numOfCols && j < icss->stmtPtr[stmtNum]->numOfCols ; j++ ) 
      rstrcpy(cVal[j],icss->stmtPtr[stmtNum]->resultValue[j],cValSize[j]);

    i = cllFreeStatement(icss,stmtNum);
    return(j);

}

/* like cmlGetOneRowFromSql but cVal uses space from query
   and then caller frees it later (via cmlFreeStatement).
   This is simplier for the caller, in some cases.   */
int cmlGetOneRowFromSqlV2 (char *sql, 
		   char *cVal[], 
		   int maxCols,
		   char *bindVar1,
		   char *bindVar2,
		   icatSessionStruct *icss)
{
    int i,j, stmtNum, ii;
    
    i = cllExecSqlWithResultBV(icss, &stmtNum, sql,
				 bindVar1, bindVar2,0,0,0,0);

    if (i != 0) {
      if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
      return (CAT_SQL_ERR);
    }
    i = cllGetRow(icss,stmtNum);
    if (i != 0)  {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_GET_ROW_ERR);
    }
    if (icss->stmtPtr[stmtNum]->numOfCols == 0)
      return(CAT_NO_ROWS_FOUND);
    for (j = 0; j < maxCols && j < icss->stmtPtr[stmtNum]->numOfCols ; j++ ) 
       cVal[j] = icss->stmtPtr[stmtNum]->resultValue[j];

    return(stmtNum);  /* 0 or positive is the statement number */
}

/*
 Like cmlGetOneRowFromSql but uses bind variable array (via 
    cllExecSqlWithResult).
 */
static
int cmlGetOneRowFromSqlV3 (char *sql, 
		   char *cVal[], 
		   int cValSize[], 
		   int numOfCols,
		   icatSessionStruct *icss)
{
    int i,j, stmtNum, ii;
    
    i = cllExecSqlWithResult(icss, &stmtNum, sql);

    if (i != 0) {
      if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
      return (CAT_SQL_ERR);
    }
    i = cllGetRow(icss,stmtNum);
    if (i != 0)  {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_GET_ROW_ERR);
    }
    if (icss->stmtPtr[stmtNum]->numOfCols == 0) {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_NO_ROWS_FOUND);
    }
    for (j = 0; j < numOfCols && j < icss->stmtPtr[stmtNum]->numOfCols ; j++ ) 
      rstrcpy(cVal[j],icss->stmtPtr[stmtNum]->resultValue[j],cValSize[j]);

    i = cllFreeStatement(icss,stmtNum);
    return(j);

}


int cmlFreeStatement(int statementNumber, icatSessionStruct *icss) 
{
   int i;
   i = cllFreeStatement(icss,statementNumber);
   return(i);
}


int cmlGetFirstRowFromSql (char *sql, 
		   int *statement,
		   int skipCount,
		   icatSessionStruct *icss)
{
    int i, stmtNum, ii;
#ifdef ORA_ICAT
    int j;
#endif
    *statement=0;
    
    i = cllExecSqlWithResult(icss, &stmtNum, sql);

    if (i != 0) {
      if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
      return (CAT_SQL_ERR);
    }

#ifdef ORA_ICAT
    if (skipCount > 0) {
       for (j=0;j<skipCount;j++) {
	  i = cllGetRow(icss,stmtNum);
	  if (i != 0)  {
	     ii = cllFreeStatement(icss,stmtNum);
	     return(CAT_GET_ROW_ERR);
	  }
	  if (icss->stmtPtr[stmtNum]->numOfCols == 0) {
	     i = cllFreeStatement(icss,stmtNum);
	     return(CAT_NO_ROWS_FOUND);
	  }
       }
    }
#endif

    i = cllGetRow(icss,stmtNum);
    if (i != 0)  {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_GET_ROW_ERR);
    }
    if (icss->stmtPtr[stmtNum]->numOfCols == 0) {
      i = cllFreeStatement(icss,stmtNum);
      return(CAT_NO_ROWS_FOUND);
    }

    *statement = stmtNum;
    return(0);
}

/* with bind-variables */
int cmlGetFirstRowFromSqlBV (char *sql, 
	           char *arg1, char *arg2, char *arg3, char *arg4,
		   int *statement,
		   icatSessionStruct *icss)
{
    int i, stmtNum, ii;

    *statement=0;
    
   i = cllExecSqlWithResultBV(icss, &stmtNum, sql,
				 arg1,arg2,arg3,arg4,0,0);

    if (i != 0) {
      if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
      return (CAT_SQL_ERR);
    }
    i = cllGetRow(icss,stmtNum);
    if (i != 0)  {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_GET_ROW_ERR);
    }
    if (icss->stmtPtr[stmtNum]->numOfCols == 0) {
      i = cllFreeStatement(icss,stmtNum);
      return(CAT_NO_ROWS_FOUND);
    }

    *statement = stmtNum;
    return(0);
}

int cmlGetNextRowFromStatement (int stmtNum, 
		   icatSessionStruct *icss)
{
    int i, ii;
    
    i = cllGetRow(icss,stmtNum);
    if (i != 0)  {
      ii = cllFreeStatement(icss,stmtNum);
      return(CAT_GET_ROW_ERR);
    }
    if (icss->stmtPtr[stmtNum]->numOfCols == 0) {
       i = cllFreeStatement(icss,stmtNum);
      return(CAT_NO_ROWS_FOUND);
    }
    return(0);
}

int cmlGetStringValueFromSql (char *sql, 
			   char *cVal,
			   int cValSize,
                           char *bindVar1,
                           char *bindVar2,
                           char *bindVar3,
			   icatSessionStruct *icss)
{
    int i;
    char *cVals[2];
    int iVals[2];

    cVals[0]=cVal;
    iVals[0]=cValSize;

    i = cmlGetOneRowFromSqlBV (sql, cVals, iVals, 1, 
			       bindVar1, bindVar2, bindVar3, 0, 0, icss);
    if (i == 1)
      return(0);
    else
      return(i);

}

int cmlGetStringValuesFromSql (char *sql, 
			   char *cVal[], 
			   int cValSize[],
		           int numberOfStringsToGet,
                           char *bindVar1,
                           char *bindVar2,
                           char *bindVar3,
			   icatSessionStruct *icss)
{
    int i;

    i = cmlGetOneRowFromSqlBV (sql, cVal, cValSize, numberOfStringsToGet,
			       bindVar1, bindVar2, bindVar3, 0, 0, icss);
    if (i == numberOfStringsToGet)
      return(0);
    else
      return(i);

}

int cmlGetMultiRowStringValuesFromSql (char *sql, 
				       char *returnedStrings,  
			      int maxStringLen,
			      int maxNumberOfStringsToGet, 
			      char *bindVar1,
			      char *bindVar2,
 		              icatSessionStruct *icss) {

    int i,j, stmtNum, ii;
    int tsg; /* total strings gotten */
    char *pString;
    
    if (maxNumberOfStringsToGet <= 0) return(CAT_INVALID_ARGUMENT);

    i = cllExecSqlWithResultBV(icss, &stmtNum, sql,
				 bindVar1,bindVar2,0,0,0,0);
    if (i != 0) {
      if (i <= CAT_ENV_ERR) return(i); /* already an iRODS error code */
      return (CAT_SQL_ERR);
    }
    tsg = 0;
    pString = returnedStrings;
    for (;;) {
       i = cllGetRow(icss,stmtNum);
       if (i != 0)  {
	  ii = cllFreeStatement(icss,stmtNum);
	  if (tsg > 0) return(tsg);
	  return(CAT_GET_ROW_ERR);
       }
       if (icss->stmtPtr[stmtNum]->numOfCols == 0) {
	  ii = cllFreeStatement(icss,stmtNum);
	  if (tsg > 0) return(tsg);
	  return(CAT_NO_ROWS_FOUND);
       }
       for (j = 0; j < icss->stmtPtr[stmtNum]->numOfCols;j++) {
	  rstrcpy(pString, icss->stmtPtr[stmtNum]->resultValue[j],
		  maxStringLen);
	  tsg++;
	  pString+=maxStringLen;
	  if (tsg >= maxNumberOfStringsToGet) {
	     i = cllFreeStatement(icss,stmtNum);
	     return(tsg);
	  }
       }
    }
}


int cmlGetIntegerValueFromSql (char *sql, 
			       rodsLong_t *iVal,
			       char *bindVar1,
			       char *bindVar2,
			       char *bindVar3,
			       char *bindVar4,
			       char *bindVar5,
			       icatSessionStruct *icss)
{
  int i, cValSize;
  char *cVal[2];
  char cValStr[MAX_INTEGER_SIZE+10];

  cVal[0]=cValStr;
  cValSize = MAX_INTEGER_SIZE;

  i = cmlGetOneRowFromSqlBV (sql, cVal, &cValSize, 1, 
			     bindVar1, bindVar2, bindVar3, bindVar4, 
			     bindVar5, icss);
  if (i == 1) {
     if (*cVal[0]=='\0') {
	return(CAT_NO_ROWS_FOUND);
     }
    *iVal = strtoll(*cVal, NULL, 0);
    return(0);
  }
  return(i);
}

/* Like cmlGetIntegerValueFromSql but uses bind-variable array */
int cmlGetIntegerValueFromSqlV3 (char *sql, 
			       rodsLong_t *iVal,
			       icatSessionStruct *icss)
{
  int i, cValSize;
  char *cVal[2];
  char cValStr[MAX_INTEGER_SIZE+10];

  cVal[0]=cValStr;
  cValSize = MAX_INTEGER_SIZE;

  i = cmlGetOneRowFromSqlV3 (sql, cVal, &cValSize, 1, icss);
  if (i == 1) {
     if (*cVal[0]=='\0') {
	return(CAT_NO_ROWS_FOUND);
     }
    *iVal = strtoll(*cVal, NULL, 0);
    return(0);
  }
  return(i);
}

int cmlCheckNameToken(char *nameSpace, char *tokenName, icatSessionStruct *icss)
{

  rodsLong_t iVal;
  int status;

  if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckNameToken SQL 1 ");
  status = cmlGetIntegerValueFromSql (
  "select token_id from  R_TOKN_MAIN where token_namespace=? and token_name=?",
      &iVal, nameSpace, tokenName, 0, 0, 0, icss);
  return(status);

}

int cmlModifySingleTable (char *tableName, 
		       char *updateCols[],
		       char *updateValues[],
		       char *whereColsAndConds[],
		       char *whereValues[],
		       int numOfUpdates, 
		       int numOfConds, 
		       icatSessionStruct *icss)
{
  char tsql[MAX_SQL_SIZE];
  int i, l;
  char *rsql;

  if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlModifySingleTable SQL 1 ");

  snprintf(tsql, MAX_SQL_SIZE, "update %s set ", tableName);
  l = strlen(tsql);
  rsql = tsql + l;

  cmlArraysToStrWithBind ( rsql, "", updateCols,updateValues, numOfUpdates, " = ", ", ",MAX_SQL_SIZE - l);
  l = strlen(tsql);
  rsql = tsql + l;

  cmlArraysToStrWithBind(rsql, " where ", whereColsAndConds, whereValues, numOfConds, "", " and ", MAX_SQL_SIZE - l);
    
  i = cmlExecuteNoAnswerSql( tsql, icss);
  return(i);

}

#define STR_LEN 100
rodsLong_t
cmlGetNextSeqVal(icatSessionStruct *icss) {
   char nextStr[STR_LEN];
   char sql[STR_LEN];
   int status;
   rodsLong_t iVal;

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlGetNextSeqVal SQL 1 ");

   nextStr[0]='\0';

   cllNextValueString("R_ObjectID", nextStr, STR_LEN);
      /* R_ObjectID is created in icatSysTables.sql as
         the sequence item for objects */

#ifdef ORA_ICAT
   /* For Oracle, use the built-in one-row table */
   snprintf(sql, STR_LEN, "select %s from DUAL", nextStr);
#else
   /* Postgres can just get the next value without a table */
   snprintf(sql, STR_LEN, "select %s", nextStr);
#endif

   status = cmlGetIntegerValueFromSql (sql, &iVal, 0, 0, 0, 0, 0, icss);
   if (status < 0) {
      rodsLog(LOG_NOTICE, 
	      "cmlGetNextSeqVal cmlGetIntegerValueFromSql failure %d", status);
      return(status);
   }
   return(iVal);
}

rodsLong_t
cmlGetCurrentSeqVal(icatSessionStruct *icss) {
   char nextStr[STR_LEN];
   char sql[STR_LEN];
   int status;
   rodsLong_t iVal;

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlGetCurrentSeqVal SQL 1 ");

   nextStr[0]='\0';

   cllCurrentValueString("R_ObjectID", nextStr, STR_LEN);
      /* R_ObjectID is created in icatSysTables.sql as
         the sequence item for objects */

#ifdef ORA_ICAT
   /* For Oracle, use the built-in one-row table */
   snprintf(sql, STR_LEN, "select %s from DUAL", nextStr);
#else
   /* Postgres can just get the next value without a table */
   snprintf(sql, STR_LEN, "select %s", nextStr);
#endif

   status = cmlGetIntegerValueFromSql (sql, &iVal, 0, 0, 0, 0, 0, icss);
   if (status < 0) {
      rodsLog(LOG_NOTICE, 
	      "cmlGetCurrentSeqVal cmlGetIntegerValueFromSql failure %d",
	      status);
      return(status);
   }
   return(iVal);
}

int 
cmlGetNextSeqStr(char *seqStr, int maxSeqStrLen, icatSessionStruct *icss) {
   char nextStr[STR_LEN];
   char sql[STR_LEN];
   int status;

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlGetNextSeqStr SQL 1 ");

   nextStr[0]='\0';
   cllNextValueString("R_ObjectID", nextStr, STR_LEN);
      /* R_ObjectID is created in icatSysTables.sql as
         the sequence item for objects */

#ifdef ORA_ICAT
   snprintf(sql, STR_LEN, "select %s from DUAL", nextStr);
#else
   snprintf(sql, STR_LEN, "select %s", nextStr);
#endif

   status = cmlGetStringValueFromSql (sql, seqStr, maxSeqStrLen, 0, 0, 0, icss);
   if (status < 0) {
      rodsLog(LOG_NOTICE, 
	      "cmlGetNextSeqStr cmlGetStringValueFromSql failure %d", status);
   }
   return(status);
}

/* modifed for various tests */
int cmlTest( icatSessionStruct *icss) {
  int i, cValSize;
  char *cVal[2];
  char cValStr[MAX_INTEGER_SIZE+10];
  char sql[100];
  rodsLong_t iVal;

  strncpy(icss->databaseUsername,"schroede", DB_USERNAME_LEN);
  strncpy(icss->databasePassword,"", DB_PASSWORD_LEN);
  i = cmlOpen(icss);
  if (i != 0) return(i);
  
  cVal[0]=cValStr;
  cValSize = MAX_INTEGER_SIZE;
  snprintf(sql,sizeof sql, 
	   "select coll_id from R_COLL_MAIN where coll_name='a'");

  i = cmlGetOneRowFromSql (sql, cVal, &cValSize, 1, icss);
  if (i == 1) {
    printf("result = %s\n",cValStr);
    i = 0;
  }
  else {
     return(i);
  }

  snprintf(sql, sizeof sql, 
	"select data_id from R_DATA_MAIN where coll_id='1' and data_name='a'");
  i = cmlGetOneRowFromSql (sql, cVal, &cValSize, 1, icss);
  if (i == 1) {
    printf("result = %s\n",cValStr);
    i = 0;
  }

  iVal = cmlGetCurrentSeqVal(icss);

  return(i);

}

/*
  Check that a resource exists and user has 'accessLevel' permission.
  Return code is either an iRODS error code (< 0) or the collectionId.
*/ 
rodsLong_t
cmlCheckResc( char *rescName, char *userName, char *userZone, char *accessLevel,
		 icatSessionStruct *icss)
{
   int status;
   rodsLong_t iVal;

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckResc SQL 1 ");

   status = cmlGetIntegerValueFromSql(
  	        "select resc_id from R_RESC_MAIN RM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where RM.resc_name=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = RM.resc_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
	      &iVal, rescName, userName, userZone, accessLevel, 0, icss);
   if (status) { 
      /* There was an error, so do another sql to see which 
         of the two likely cases is problem. */

      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckResc SQL 2 ");

      status = cmlGetIntegerValueFromSql(
		 "select resc_id from R_RESC_MAIN where resc_name=?",
		 &iVal, rescName, 0, 0, 0, 0, icss);
      if (status) {
	 return(CAT_UNKNOWN_RESOURCE);
      }
      return (CAT_NO_ACCESS_PERMISSION);
   }

   return(iVal);

}


/*
  Check that a collection exists and user has 'accessLevel' permission.
  Return code is either an iRODS error code (< 0) or the collectionId.
*/
rodsLong_t
cmlCheckDir( char *dirName, char *userName, char *userZone, char *accessLevel,
		 icatSessionStruct *icss)
{
   int status;
   rodsLong_t iVal;

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDir SQL 1 ");

   status = cmlGetIntegerValueFromSql(
  	        "select coll_id from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_name=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
	      &iVal, dirName, userName, userZone, accessLevel, 0, icss);
   if (status) { 
      /* There was an error, so do another sql to see which 
         of the two likely cases is problem. */

      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDir SQL 2 ");

      status = cmlGetIntegerValueFromSql(
		 "select coll_id from R_COLL_MAIN where coll_name=?",
		 &iVal, dirName, 0, 0, 0, 0, icss);
      if (status) {
	 return(CAT_UNKNOWN_COLLECTION);
      }
      return (CAT_NO_ACCESS_PERMISSION);
   }

   return(iVal);

}


/*
  Check that a collection exists and user has 'accessLevel' permission.
  Return code is either an iRODS error code (< 0) or the collectionId.
  While at it, get the inheritance flag.
*/
rodsLong_t
cmlCheckDirAndGetInheritFlag( char *dirName, char *userName, char *userZone,
			      char *accessLevel, int *inheritFlag,
		 icatSessionStruct *icss)
{
   int status;
   rodsLong_t iVal;

   int cValSize[2];
   char *cVal[3];
   char cValStr1[MAX_INTEGER_SIZE+10];
   char cValStr2[MAX_INTEGER_SIZE+10];

   cVal[0]=cValStr1;
   cVal[1]=cValStr2;
   cValSize[0] = MAX_INTEGER_SIZE;
   cValSize[1] = MAX_INTEGER_SIZE;

   *inheritFlag = 0;
   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDirAndGetInheritFlag SQL 1 ");

   status = cmlGetOneRowFromSqlBV ("select coll_id, coll_inheritance from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_name=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?", cVal, cValSize, 2, dirName, userName, userZone, accessLevel, 0, icss);
   if (status == 2) {
      if (*cVal[0]=='\0') {
	 return(CAT_NO_ROWS_FOUND);
      }
      iVal = strtoll(*cVal, NULL, 0);
      if (cValStr2[0]=='1') *inheritFlag = 1;
      status = 0;
   }

   if (status) { 
      /* There was an error, so do another sql to see which 
         of the two likely cases is problem. */

      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDirAndGetInheritFlag SQL 2 ");

      status = cmlGetIntegerValueFromSql(
		 "select coll_id from R_COLL_MAIN where coll_name=?",
		 &iVal, dirName, 0, 0, 0, 0, icss);
      if (status) {
	 return(CAT_UNKNOWN_COLLECTION);
      }
      return (CAT_NO_ACCESS_PERMISSION);
   }

   return(iVal);

}


/*
  Check that a collection exists and user has 'accessLevel' permission.
  Return code is either an iRODS error code (< 0) or the collectionId.
*/
rodsLong_t
cmlCheckDirId( char *dirId, char *userName, char *userZone, 
	       char *accessLevel, icatSessionStruct *icss)
{
   int status;
   rodsLong_t iVal;

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDirId SQL 1 ");

   status = cmlGetIntegerValueFromSql(
  	        "select object_id from R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = ? and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
	      &iVal, userName, userZone, dirId, accessLevel, 0, icss);
   if (status) { 
      /* There was an error, so do another sql to see which 
         of the two likely cases is problem. */

      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDirId SQL 2 ");

      status = cmlGetIntegerValueFromSql(
		 "select coll_id from R_COLL_MAIN where coll_id=?",
		 &iVal, dirId, 0, 0, 0, 0, icss);
      if (status) {
	 return(CAT_UNKNOWN_COLLECTION);
      }
      return (CAT_NO_ACCESS_PERMISSION);
   }

   return(0);
}

/*
  Check that a collection exists and user owns it
*/
rodsLong_t
cmlCheckDirOwn( char *dirName, char *userName, char *userZone,
			icatSessionStruct *icss)
{
   int status; 
   rodsLong_t iVal; 

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDirOwn SQL 1 ");

   status = cmlGetIntegerValueFromSql(
	    "select coll_id from R_COLL_MAIN where coll_name=? and coll_owner_name=? and coll_owner_zone=?",
	    &iVal, dirName, userName, userZone, 0, 0, icss);
   if (status < 0) return(status);
   return(iVal);
}


/*
  Check that a dataObj (iRODS file) exists and user has specified permission
  (but don't check the collection access, only its existance).
  Return code is either an iRODS error code (< 0) or the dataId.
*/
rodsLong_t
cmlCheckDataObjOnly( char *dirName, char *dataName, 
		     char *userName, char *userZone,  
		     char *accessLevel, icatSessionStruct *icss)
{
   int status;
   rodsLong_t iVal; 

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDataObjOnly SQL 1 ");

   status = cmlGetIntegerValueFromSql(
  	        "select data_id from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM, R_COLL_MAIN CM where DM.data_name=? and DM.coll_id=CM.coll_id and CM.coll_name=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
		 &iVal, dataName, dirName, userName, userZone, 
		accessLevel, icss);

   if (status) { 
      /* There was an error, so do another sql to see which 
         of the two likely cases is problem. */
      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDataObjOnly SQL 2 ");

      status = cmlGetIntegerValueFromSql(
         "select data_id from R_DATA_MAIN DM, R_COLL_MAIN CM where DM.data_name=? and DM.coll_id=CM.coll_id and CM.coll_name=?",
	  &iVal, dataName, dirName, 0, 0, 0, icss);
      if (status) {
	 return(CAT_UNKNOWN_FILE);
      }
      return (CAT_NO_ACCESS_PERMISSION);
   }

   return(iVal);

}

/*
  Check that a dataObj (iRODS file) exists and user owns it
*/
rodsLong_t
cmlCheckDataObjOwn( char *dirName, char *dataName, char *userName, 
		    char *userZone, icatSessionStruct *icss)
{
   int status;
   rodsLong_t iVal, collId;
   char collIdStr[MAX_NAME_LEN];

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDataObjOwn SQL 1 ");
   status = cmlGetIntegerValueFromSql(
	    "select coll_id from R_COLL_MAIN where coll_name=?",
	    &iVal, dirName, 0, 0, 0, 0, icss);
   if (status < 0) return(status);
   collId = iVal;
   snprintf(collIdStr, MAX_NAME_LEN, "%lld", collId);

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDataObjOwn SQL 2 ");
   status = cmlGetIntegerValueFromSql(
	         "select data_id from R_DATA_MAIN where data_name=? and coll_id=? and data_owner_name=? and data_owner_zone=?",
		 &iVal, dataName, collIdStr, userName, userZone, 0, icss);

   if (status) {
      return (status);
   }
   return(iVal);
}

/*
  Check that a user has the specified permission or better to a dataObj.
  Return value is either an iRODS error code (< 0) or success (0).
*/
int cmlCheckDataObjId( char *dataId, char *userName,  char *zoneName, 
		     char *accessLevel, icatSessionStruct *icss)
{
   int status;
   rodsLong_t iVal; 

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckDataObjId SQL 1 ");

   iVal=0;
   status = cmlGetIntegerValueFromSql(
  	        "select object_id from R_OBJT_ACCESS OA, R_DATA_MAIN DM, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where OA.object_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
	    &iVal,
	    dataId,
	    userName,
	    zoneName,
	    accessLevel,
	    0,
	    icss);
   if (status != 0) return (CAT_NO_ACCESS_PERMISSION);
   if (iVal==0)  return (CAT_NO_ACCESS_PERMISSION);
   cmlAudit2(AU_ACCESS_GRANTED, dataId, userName, zoneName, accessLevel, icss);
   return(status);
}

/* 
 Check that the user has permission to add or remover a user to a
 group (other than rodsadmin).  The user must be of type 'groupadmin'
 and a member of the specified group.
 */
int cmlCheckGroupAdminAccess(char *userName, char *userZone, 
			     char *groupName, icatSessionStruct *icss) {
   int status;
   char sVal[MAX_NAME_LEN];
   rodsLong_t iVal;

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckGroupAdminAccess SQL 1 ");

   status = cmlGetStringValueFromSql(
      "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name='groupadmin'",
      sVal, MAX_NAME_LEN, userName, userZone, 0, icss);
   if (status==CAT_NO_ROWS_FOUND) return (CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
   if (status) return(status);

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlCheckGroupAdminAccess SQL 2 ");

   status = cmlGetIntegerValueFromSql(
      "select group_user_id from R_USER_GROUP where user_id=? and group_user_id = (select user_id from R_USER_MAIN where user_type_name='rodsgroup' and user_name=?)",
      &iVal, sVal,  groupName, 0, 0, 0, icss);
   if (status==CAT_NO_ROWS_FOUND) return (CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
   if (status) return(status);
   return(0);
}

/*********************************************************************
The following are the auditing functions, different forms.  cmlAudit1,
2, 3, 4, and 5 each audit (record activity in the audit table) but
have different input arguments.
 *********************************************************************/

/* 
 Audit - record auditing information, form 1
 */
int
cmlAudit1(int actionId, char *clientUser, char *zone, char *targetUser, 
	  char *comment, icatSessionStruct *icss)
{
   char myTime[50];
   char actionIdStr[50];
   int status;

   if (auditEnabled==0) return(0);

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlAudit1 SQL 1 ");

   getNowStr(myTime);

   snprintf(actionIdStr, sizeof actionIdStr, "%d", actionId);

   cllBindVars[0]=targetUser;
   cllBindVars[1]=zone;
   cllBindVars[2]=clientUser;
   cllBindVars[3]=zone;
   cllBindVars[4]=actionIdStr;
   cllBindVars[5]=comment;
   cllBindVars[6]=myTime;
   cllBindVars[7]=myTime;
   cllBindVarCount=8;

   status = cmlExecuteNoAnswerSql(
				  "insert into R_OBJT_AUDIT (object_id, user_id, action_id, r_comment, create_ts, modify_ts) values ((select user_id from R_USER_MAIN where user_name=? and zone_name=?), (select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)", icss);
   if (status != 0) {
      rodsLog(LOG_NOTICE, "cmlAudit1 insert failure %d", status);
   }
#ifdef ORA_ICAT
#else
   else {
      cllCheckPending("",2, icss->databaseType);
      /* Indicate that this was an audit SQL and so should be
	 committed on disconnect if still pending. */
   }
#endif
   return(status);
}

int 
cmlAudit2(int actionId, char *dataId, char *userName, char *zoneName, 
          char *accessLevel, icatSessionStruct *icss) 
{
   char myTime[50];
   char actionIdStr[50];
   int status;

   if (auditEnabled==0) return(0);

   if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlAudit2 SQL 1 ");

   getNowStr(myTime);

   snprintf(actionIdStr, sizeof actionIdStr, "%d", actionId);

   cllBindVars[0]=dataId;
   cllBindVars[1]=userName;
   cllBindVars[2]=zoneName;
   cllBindVars[3]=actionIdStr;
   cllBindVars[4]=accessLevel;
   cllBindVars[5]=myTime;
   cllBindVars[6]=myTime;
   cllBindVarCount=7;

   status = cmlExecuteNoAnswerSql(
				  "insert into R_OBJT_AUDIT (object_id, user_id, action_id, r_comment, create_ts, modify_ts) values (?, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)", icss);
   if (status != 0) {
      rodsLog(LOG_NOTICE, "cmlAudit2 insert failure %d", status);
   }
#ifdef ORA_ICAT
#else
   else {
      cllCheckPending("",2,  icss->databaseType);
      /* Indicate that this was an audit SQL and so should be
	 committed on disconnect if still pending. */
   }
#endif

   return(status);
}


int 
cmlAudit3(int actionId, char *dataId, char *userName, char *zoneName, 
          char *comment, icatSessionStruct *icss) 
{
   char myTime[50];
   char actionIdStr[50];
   int status;
   char myComment[AUDIT_COMMENT_MAX_SIZE+10];

   if (auditEnabled==0) return(0);

   getNowStr(myTime);

   snprintf(actionIdStr, sizeof actionIdStr, "%d", actionId);

   /* Truncate the comment if necessary (or else SQL will fail)*/
   myComment[AUDIT_COMMENT_MAX_SIZE-1]='\0';
   strncpy(myComment, comment, AUDIT_COMMENT_MAX_SIZE-1);

   if (zoneName[0]=='\0') {
      /* This no longer seems to occur.  I'm leaving the code in place
         (just in case) but I'm removing the rodsLog call so the ICAT
         test suite does not require it to be tested.
      */
      /*      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlAu---dit3 S--QL 1 "); */
      cllBindVars[0]=dataId;
      cllBindVars[1]=userName;
      cllBindVars[2]=actionIdStr;
      cllBindVars[3]=myComment;
      cllBindVars[4]=myTime;
      cllBindVars[5]=myTime;
      cllBindVarCount=6;
      status = cmlExecuteNoAnswerSql(
				  "insert into R_OBJT_AUDIT (object_id, user_id, action_id, r_comment, create_ts, modify_ts) values (?, (select user_id from R_USER_MAIN where user_name=? and zone_name=(select zone_name from R_ZONE_MAIN where zone_type_name='local')), ?, ?, ?, ?)", icss);
   }
   else {
      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlAudit3 SQL 2 ");
      cllBindVars[0]=dataId;
      cllBindVars[1]=userName;
      cllBindVars[2]=zoneName;
      cllBindVars[3]=actionIdStr;
      cllBindVars[4]=myComment;
      cllBindVars[5]=myTime;
      cllBindVars[6]=myTime;
      cllBindVarCount=7;
      status = cmlExecuteNoAnswerSql(
				  "insert into R_OBJT_AUDIT (object_id, user_id, action_id, r_comment, create_ts, modify_ts) values (?, (select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)", icss);
   }

   if (status != 0) {
      rodsLog(LOG_NOTICE, "cmlAudit3 insert failure %d", status);
   }
#ifdef ORA_ICAT
#else
   else {
      cllCheckPending("",2,  icss->databaseType);
      /* Indicate that this was an audit SQL and so should be
	 committed on disconnect if still pending. */
   }
#endif

   return(status);
}


int 
cmlAudit4(int actionId, char *sql, char *sqlParm, char *userName, 
	  char *zoneName, char *comment, icatSessionStruct *icss) 
{
   char myTime[50];
   char actionIdStr[50];
   char myComment[AUDIT_COMMENT_MAX_SIZE+10];
   char mySQL[MAX_SQL_SIZE];
   int status;
   int i;

   if (auditEnabled==0) return(0);

   getNowStr(myTime);

   snprintf(actionIdStr, sizeof actionIdStr, "%d", actionId);

   /* Truncate the comment if necessary (or else SQL will fail)*/
   myComment[AUDIT_COMMENT_MAX_SIZE-1]='\0';
   strncpy(myComment, comment, AUDIT_COMMENT_MAX_SIZE-1);

   if (zoneName[0]=='\0') {
      /* This no longer seems to occur.  I'm leaving the code in place
         (just in case) but I'm removing the rodsLog call so the ICAT
         test suite does not require it to be tested.
      */
      /*   
      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlA---udit4 S--QL 1 ");
      */
      snprintf(mySQL, MAX_SQL_SIZE, 
	       "insert into R_OBJT_AUDIT (object_id, user_id, action_id, r_comment, create_ts, modify_ts) values ((%s), (select user_id from R_USER_MAIN where user_name=? and zone_name=(select zone_name from R_ZONE_MAIN where zone_type_name='local')), ?, ?, ?, ?)",
	       sql);
      i=0;
      if (sqlParm[0]!='\0') {
	 cllBindVars[i++]=sqlParm;
      }
      cllBindVars[i++]=userName;
      cllBindVars[i++]=actionIdStr;
      cllBindVars[i++]=myComment;
      cllBindVars[i++]=myTime;
      cllBindVars[i++]=myTime;
      cllBindVarCount=i;
      status = cmlExecuteNoAnswerSql(mySQL,icss);
   }
   else {
      if (logSQL_CML!=0) rodsLog(LOG_SQL, "cmlAudit4 SQL 2 ");
      snprintf(mySQL, MAX_SQL_SIZE, 
	       "insert into R_OBJT_AUDIT (object_id, user_id, action_id, r_comment, create_ts, modify_ts) values ((%s), (select user_id from R_USER_MAIN where user_name=? and zone_name=?), ?, ?, ?, ?)",
	       sql);
      i=0;
      if (sqlParm[0]!='\0') {
	 cllBindVars[i++]=sqlParm;
      }
      cllBindVars[i++]=userName;
      cllBindVars[i++]=zoneName;
      cllBindVars[i++]=actionIdStr;
      cllBindVars[i++]=myComment;
      cllBindVars[i++]=myTime;
      cllBindVars[i++]=myTime;
      cllBindVarCount=i;
      status = cmlExecuteNoAnswerSql(mySQL,icss);
   }

   if (status != 0) {
      rodsLog(LOG_NOTICE, "cmlAudit4 insert failure %d", status);
   }
#ifdef ORA_ICAT
#else
   else {
      cllCheckPending("",2,  icss->databaseType);
      /* Indicate that this was an audit SQL and so should be
	 committed on disconnect if still pending. */
   }
#endif

   return(status);
}


/* 
 Audit - record auditing information
 */
int
cmlAudit5(int actionId, char *objId, char *userId, char *comment, 
	    icatSessionStruct *icss)
{
   char myTime[50];
   char actionIdStr[50];
   int status;

   if (auditEnabled==0) return(0);

   getNowStr(myTime);

   snprintf(actionIdStr, sizeof actionIdStr, "%d", actionId);

   cllBindVars[0]=objId;
   cllBindVars[1]=userId;
   cllBindVars[2]=actionIdStr;
   cllBindVars[3]=comment;
   cllBindVars[4]=myTime;
   cllBindVars[5]=myTime;
   cllBindVarCount=6;

   status = cmlExecuteNoAnswerSql(
	  "insert into R_OBJT_AUDIT (object_id, user_id, action_id, r_comment, create_ts, modify_ts) values (?,?,?,?,?,?)",
	  icss);
   if (status != 0) {
      rodsLog(LOG_NOTICE, "cmlAudit5 insert failure %d", status);
   }
#ifdef ORA_ICAT
#else
   else {
      cllCheckPending("",2,  icss->databaseType);
      /* Indicate that this was an audit SQL and so should be
	 committed on disconnect if still pending. */
   }
#endif
   return(status);
}
