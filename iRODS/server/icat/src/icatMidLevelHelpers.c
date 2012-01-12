/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/**************************************************************************

  This file contains helper functions that can be used by the rcat
  routines.  Many of these are currently unused.
**************************************************************************/


#include "icatMidLevelHelpers.h"
#include "icatMidLevelRoutines.h"
#include "stringOpr.h"
#include <stdio.h>
#include <time.h>

/*
 opt 0 do not add single '
 opt 1 do add single quotes
 opt 2 add single quotes if string does not start with (
 */
char *cmlArrToSepStr(char *str, 
		  char *preStr,
		  char *arr[], 
		  int   arrLen, 
		  char *sep, 
		  int  maxLen,
		  int opt)
{
  /* a[0] s a[1] s a[2] .... */
  int i;

  rstrcpy(str, preStr, maxLen);

  if (arrLen > 0)
    rstrcat(str, arr[0], maxLen);
  for (i = 1; i < arrLen; i++) {
    rstrcat(str, sep, maxLen);
    if (opt==1) rstrcat(str, "'", maxLen);
    if (opt==2 && *arr[i]!='(') rstrcat(str, "'", maxLen);
    rstrcat(str, arr[i], maxLen);
    if (opt==1) rstrcat(str, "'", maxLen);
    if (opt==2 && *arr[i]!='(') rstrcat(str, "'", maxLen);
  }
  return(str);
  
}

/* Based on some naming rules and individual cases, determine if a
   particular table column is a string field or not */
int columnIsText(char *column) {
   if (strstr(column, "_id") != NULL) return(0); /* integer */
   if (strstr(column, "maxsize") != NULL) return(0); /* integer */
   if (strstr(column, "data_size") != NULL) return(0); /* integer */
   if (strstr(column, "data_map_id") != NULL) return(0); /* integer */
   if (strstr(column, "attr_expose") != NULL) return(0); /* integer */
   if (strstr(column, "fk_owner") != NULL) return(0); /* integer */
   if (strstr(column, "uschema_owner") != NULL) return(0); /* integer */
   if (strstr(column, "schema_owner") != NULL) return(0); /* integer */
   if (strstr(column, "data_repl_num") != NULL) return(0); /* integer */
   if (strstr(column, "data_is_dirty") != NULL) return(0); /* integer */
   if (strstr(column, "data_map_id") != NULL) return(0); /* integer */
   if (strstr(column, "rule_seq_num") != NULL) return(0); /* integer */
   return(1);  /* the rest are strings */
}  


char *cmlArr2ToSepStr(char *str, 
		   char *preStr,
		   char *arr[], 
		   char *arr2[],
		   int   arrLen, 
		   char *sep, 
		   int  maxLen)
{
  /* a[0] a2[0] s a[1] a2[1] s a[2] a2[2] .... */
  int i;

  rstrcpy(str, preStr, maxLen);

  if (arrLen > 0) {
    rstrcat(str, arr[0], maxLen);
    rstrcat(str, arr2[0], maxLen);
  }
  for (i = 1; i < arrLen; i++) {
    rstrcat(str, sep, maxLen);
    rstrcat(str, arr[i], maxLen);
    rstrcat(str, arr2[i], maxLen);
  }
  return(str);
  
}


char *cmlArr2ToSep2Str(char *str, 
		   char *preStr,
		   char *arr[], 
		   char *arr2[],
		   int   arrLen, 
		   char *sep, 
		   char *sep2, 
		   int  maxLen)
{
  /* a[0] s  a2[0] s2 a[1] s a2[1] s2 a[2] s a2[2] .... */
  int i;
  int isText, hasQuote;
  char *cp;

  rstrcpy(str, preStr, maxLen);

  for (i = 0; i < arrLen; i++) {
     if (i>0) {
	rstrcat(str, sep2, maxLen);
     }
     rstrcat(str, arr[i], maxLen);
     rstrcat(str, sep, maxLen);
     isText = columnIsText(arr[i]);
     cp = arr[i];
     hasQuote = 0;
     if (*cp == '\'') hasQuote=1;
     if (isText && !hasQuote) {
	rstrcat(str, "'", maxLen);
	rstrcat(str, arr2[i], maxLen);
	rstrcat(str, "'", maxLen);
     }
     else {
	rstrcat(str, arr2[i], maxLen);
     }
  }
  return(str);
  
}

/*
 Convert the intput arrays to a string and add bind variables
 */
char *cmlArraysToStrWithBind(char *str, 
		   char *preStr,
		   char *arr[], 
		   char *arr2[],
		   int   arrLen, 
		   char *sep, 
		   char *sep2, 
		   int  maxLen)
{
  int i;

  rstrcpy(str, preStr, maxLen);

  for (i = 0; i < arrLen; i++) {
     if (i>0) {
	rstrcat(str, sep2, maxLen);
     }
     rstrcat(str, arr[i], maxLen);
     rstrcat(str, sep, maxLen);
     rstrcat(str, "?", maxLen);
     cllBindVars[cllBindVarCount++]=arr2[i];
  }
  return(str);
  
}

/* Currently unused */
int cmlGetOneRowFromSingleTableUnused (char *tableName, 
			   char *cVal[], 
			   int cValSize[],
			   char *selectCols[], 
			   char *whereCols[],
			   char *whereConds[], 
			   int numOfSels, 
			   int numOfConds, 
			   icatSessionStruct *icss)
{
  char tsql[MAX_SQL_SIZE];
  int i;
  char *rsql;
  char *tArr[2];

  tArr[0] = tableName;

  cmlArrToSepStr(tsql, "select ",  selectCols, numOfSels, ", ", 
		 MAX_SQL_SIZE,0);
  i = strlen(tsql);
  rsql = tsql + i;

  cmlArrToSepStr(rsql, " from ", tArr, 1, ", ", MAX_SQL_SIZE - i, 0);
  i = strlen(tsql);
  rsql = tsql + i;

  cmlArr2ToSepStr(rsql, " where ", whereCols, whereConds, numOfConds, " and ",
		  MAX_SQL_SIZE - i);
  
  i = cmlGetOneRowFromSql (tsql, cVal, cValSize, numOfSels, icss);
  return(i);
}


/* Currently unused */
int cmlDeleteFromSingleTableUnused (char *tableName, 
			   char *selectCols[],
			   char *selectConds[],
			   int numOfConds, 
			   icatSessionStruct *icss)
{
  char tsql[MAX_SQL_SIZE];
  int i, l;
  char *rsql;

  snprintf(tsql, MAX_SQL_SIZE, "delete from %s ", tableName);
  l = strlen(tsql);
  rsql = tsql + l;
  
  cmlArr2ToSepStr(rsql, " where ", selectCols, selectConds, numOfConds,
		  " and ", MAX_SQL_SIZE - l);
  i =  cmlExecuteNoAnswerSql( tsql, icss);
  return(i);

}



/* currently unused */
int cmlInsertIntoSingleTableUnused(char *tableName,
			   char *insertCols[], 
			   char *insertValues[], 
			   int numOfCols,
			   icatSessionStruct *icss)
{
  char tsql[MAX_SQL_SIZE];
  int i, len;
  char *rsql;

  snprintf(tsql,  MAX_SQL_SIZE, "insert into %s (", tableName );
  len = strlen(tsql);
  rsql = tsql + len;

  cmlArrToSepStr(rsql, "", insertCols, numOfCols, ", ", MAX_SQL_SIZE - len, 0);
  len = strlen(tsql);
  rsql = tsql + len;

  rstrcat(tsql, ") values (", MAX_SQL_SIZE);
  for (i=0;i<numOfCols;i++) {
     if (i==0) {
	rstrcat(tsql, "?", MAX_SQL_SIZE);
     } 
     else {
	rstrcat(tsql, ",?", MAX_SQL_SIZE);
     }
     cllBindVars[cllBindVarCount++]=insertValues[i];
  }

  rstrcat(tsql, ")", MAX_SQL_SIZE);
  i =  cmlExecuteNoAnswerSql( tsql, icss);
  return(i);
}

/* like cmlInsertIntoSingleTable but the insertColumns are already
   in a single string*/
/* No longer needed with bind vars */
int cmlInsertIntoSingleTableV2Unused (char *tableName,
			   char *insertCols, 
			   char *insertValues[], 
			   int numOfCols,
			   icatSessionStruct *icss)
{
  char tsql[MAX_SQL_SIZE];
  int i, l;
  char *rsql;

  snprintf(tsql,  MAX_SQL_SIZE, "insert into %s (", tableName );
  l = strlen(tsql);
  rsql = tsql + l;

  rstrcat(tsql, insertCols, MAX_SQL_SIZE);
  l = strlen(tsql);
  rsql = tsql + l;

  cmlArrToSepStr(rsql,") values (",insertValues, numOfCols, ", ", MAX_SQL_SIZE - l, 2);

  rstrcat(tsql, ")", MAX_SQL_SIZE);
  i =  cmlExecuteNoAnswerSql( tsql, icss);
  return(i);
}


/* Currently unused */
int cmlGetUserIdUnused( char *userName, icatSessionStruct *icss) {
   int status;
   char tsql[MAX_SQL_SIZE];
   rodsLong_t iVal;

   /* 
   Need to add domain sometime
    */

   snprintf(tsql, MAX_SQL_SIZE, 
	    "select user_id from R_USER_MAIN where user_name = ?");
   status = cmlGetIntegerValueFromSql (tsql, &iVal, userName, 0, 0, 0, 0,icss);
   if (status == 0) return(iVal);

   /* need to cache a few and skip the sql if found */

   return(status);
}
