/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*

 These routines provide the generalInsert and Delete row capabilities.
 Admins (for now) are allowed to call these functions to add or remove
 specified rows into certain tables.  The arguments are similar to the
 generalQuery in that columns are specified via the COL_ #defines.

 Currently, updates can only be done on a single table at a time.

 Initially, this was developed for use with a notification service which
 was postponed and now may be used with rule tables.
*/
#include "rodsGeneralUpdate.h"

#include "rodsClient.h"
#include "icatMidLevelRoutines.h"
#include "icatLowLevel.h"

extern int sGetColumnInfo(int defineVal, char **tableName, char **columnName);
extern icatSessionStruct *chlGetRcs();

extern int icatGeneralQuerySetup();

int updateDebug=0;

int logSQLGenUpdate=0;
char tSQL[MAX_SQL_SIZE];

int
generalInsert(generalUpdateInp_t generalUpdateInp) {
   int i, j;
   char *tableName, *columnName;
   char *firstTableName;
   char nextSeqValueIndex=-1;
   static char nextStr[MAX_NAME_LEN];
   int doBind;
   static char myTime[50];

   rstrcpy(tSQL, "insert into ", MAX_SQL_SIZE);

   for (i=0;i<generalUpdateInp.values.len;i++) { 
      j = sGetColumnInfo(generalUpdateInp.values.inx[i],
			  &tableName, &columnName);
      if (generalUpdateInp.values.inx[i] < MAX_CORE_TABLE_VALUE) {
	 return(CAT_TABLE_ACCESS_DENIED); /* only extended icat tables allowed*/
      }
      if (updateDebug) printf("j=%d\n",j);
      if (j==0) {
	 if (updateDebug) printf("tableName=%s\n",tableName);
	 if (updateDebug) printf("columnName=%s\n",columnName);
      }
      else {
	 return(j);
      }

      doBind=1;
      if (strncmp(generalUpdateInp.values.value[i], 
		  UPDATE_NEXT_SEQ_VALUE, MAX_NAME_LEN)==0) {
         /* caller requesting a next sequence */
	 cllNextValueString("R_ExtObjectID", nextStr, MAX_NAME_LEN);
	 nextSeqValueIndex=i;
	 doBind=0;
      }
      if (i==0) {
	 firstTableName=tableName;
	 rstrcat(tSQL, tableName, MAX_SQL_SIZE);
	 rstrcat(tSQL, " (", MAX_SQL_SIZE);
	 rstrcat(tSQL, columnName, MAX_SQL_SIZE);
	 if (doBind) {
	    if (strncmp(generalUpdateInp.values.value[i], 
			UPDATE_NOW_TIME, MAX_NAME_LEN) == 0) {
	       getNowStr(myTime);
	       cllBindVars[cllBindVarCount++]=myTime;
	    }
	    else {
	       cllBindVars[cllBindVarCount++]=generalUpdateInp.values.value[i];
	    }
	 }
      }
      else {
	 if (strcmp(tableName, firstTableName) !=0) {
	    return(CAT_INVALID_ARGUMENT);
	 }
	 rstrcat(tSQL, ", ", MAX_SQL_SIZE);
	 rstrcat(tSQL, columnName, MAX_SQL_SIZE);
	 if (doBind) {
	    if (strncmp(generalUpdateInp.values.value[i], 
			UPDATE_NOW_TIME, MAX_NAME_LEN) == 0) {
	       getNowStr(myTime);
	       cllBindVars[cllBindVarCount++]=myTime;
	    }
	    else {
	       cllBindVars[cllBindVarCount++]=generalUpdateInp.values.value[i];
	    }
	 }   
      }
   }
   if (nextSeqValueIndex==0) {
      rstrcat(tSQL, ") values (", MAX_SQL_SIZE);
      rstrcat(tSQL, nextStr, MAX_SQL_SIZE);
   }
   else {
      rstrcat(tSQL, ") values (?", MAX_SQL_SIZE);
   }
   for (i=1;i<generalUpdateInp.values.len;i++) { 
      if (nextSeqValueIndex==i) {
	 rstrcat(tSQL, ", ", MAX_SQL_SIZE);
	 rstrcat(tSQL, nextStr, MAX_SQL_SIZE);
      }
      else {
	 rstrcat(tSQL, ", ?", MAX_SQL_SIZE);
      }
   }
   rstrcat(tSQL, ")", MAX_SQL_SIZE);
   if (updateDebug) printf("tSQL: %s\n", tSQL);

   return(0);
}

int
generalDelete(generalUpdateInp_t generalUpdateInp) {
   int i, j;
   char *tableName, *columnName;
   char *firstTableName;

   rstrcpy(tSQL, "delete from ", MAX_SQL_SIZE);

   for (i=0;i<generalUpdateInp.values.len;i++) { 
      if (generalUpdateInp.values.inx[i] < MAX_CORE_TABLE_VALUE) {
	 return(CAT_TABLE_ACCESS_DENIED); /* only extended icat tables allowed*/
      }
      j = sGetColumnInfo(generalUpdateInp.values.inx[i],
			  &tableName, &columnName);
      if (updateDebug) printf("j=%d\n",j);
      if (j==0) {
	 if (updateDebug) printf("tableName=%s\n",tableName);
	 if (updateDebug) printf("columnName=%s\n",columnName);
      }
      else {
	 return(j);
      }
      if (i==0) {
	 firstTableName=tableName;
	 rstrcat(tSQL, tableName, MAX_SQL_SIZE);
	 rstrcat(tSQL, " where ", MAX_SQL_SIZE);
	 rstrcat(tSQL, columnName, MAX_SQL_SIZE);
	 rstrcat(tSQL, " = ?", MAX_SQL_SIZE);
	 cllBindVars[cllBindVarCount++]=generalUpdateInp.values.value[i];
      }
      else {
	 if (strcmp(tableName, firstTableName) !=0) {
	    return(CAT_INVALID_ARGUMENT);
	 }
	 rstrcat(tSQL, " and ", MAX_SQL_SIZE);
	 rstrcat(tSQL, columnName, MAX_SQL_SIZE);
	 rstrcat(tSQL, " = ?", MAX_SQL_SIZE);
	 cllBindVars[cllBindVarCount++]=generalUpdateInp.values.value[i];
      }
   }
   if (updateDebug) printf("tSQL: %s\n", tSQL);
   return(0);
}


/* General Update */
int
chlGeneralUpdate(generalUpdateInp_t generalUpdateInp) {
   int status;
   static int firstCall=1;
   icatSessionStruct *icss;

   icss = chlGetRcs();
   /*   result->rowCount=0; */

   if (firstCall) {
      icatGeneralQuerySetup();
   }
   if (generalUpdateInp.type == GENERAL_UPDATE_INSERT) {
      status = generalInsert(generalUpdateInp);
      if (status) return (status);
      /* Since the sql string is lower case, this is not checked for
         in ICAT test suite; removed since now this is only for
         extented tables and so would be difficult to test */
      if (logSQLGenUpdate) rodsLog(LOG_SQL, "chlGeneralUpdate sql 1");
   }
   else {
      if (generalUpdateInp.type == GENERAL_UPDATE_DELETE) {
	 status = generalDelete(generalUpdateInp);
	 if (status) return (status);
	 if (logSQLGenUpdate) rodsLog(LOG_SQL, "chlGeneralUpdate sql 2");
      }
      else {
	 return(CAT_INVALID_ARGUMENT);
      }
   }

   status =  cmlExecuteNoAnswerSql(tSQL, icss);
   if (status != 0) {
      rodsLog(LOG_NOTICE,
	      "chlGeneralUpdate cmlExecuteNoAnswerSql insert failure %d",
	      status);
      return(status);
   }

   status =  cmlExecuteNoAnswerSql("commit", icss);
   if (status != 0) {
      rodsLog(LOG_NOTICE,
	      "chlGeneralUpdate cmlExecuteNoAnswerSql commit failure %d",
	      status);
      return(status);
   }

   return(0);
}

int
chlDebugGenUpdate(int mode) {
   logSQLGenUpdate = mode;
   return(0);
}
