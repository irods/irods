/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* 

These are the Catalog Low Level (cll) routines for talking to postgresql.

For each of the supported database systems there is .c file like this
one with a set of routines by the same names.

Callable functions:
   cllOpenEnv
   cllCloseEnv
   cllConnect
   cllDisconnect
   cllGetRowCount
   cllExecSqlNoResult
   cllExecSqlWithResult
   cllDoneWithResult
   cllDoneWithDefaultResult
   cllGetRow
   cllGetRows
   cllGetNumberOfColumns
   cllGetColumnInfo
   cllNextValueString

Internal functions are those that do not begin with cll.
The external functions used are those that begin with SQL.

*/

#include "icatLowLevelOdbc.h"
int _cllFreeStatementColumns(icatSessionStruct *icss, int statementNumber);

int
_cllExecSqlNoResult(icatSessionStruct *icss, char *sql, int option);


int cllBindVarCount=0;
char *cllBindVars[MAX_BIND_VARS];
int cllBindVarCountPrev=0; /* cclBindVarCount earlier in processing */

SQLCHAR  psgErrorMsg[SQL_MAX_MESSAGE_LENGTH + 10];

#ifdef ADDR_64BITS
/* Different argument types are needed on at least Ubuntu 11.04 on a
   64-bit host when using MySQL, but may or may not apply to all
   64-bit hosts.  The ODBCVER in sql.h is the same, 0x0351, but some
   of the defines differ.  If it's using new defines and this isn't
   used, there may be compiler warnings but it might link OK, but not
   operate correctly consistently.  I'm not sure how to properly
   differentiate between the two, but am using ADDR_64BITS for
   now.  */
#define SQL_INT_OR_LEN SQLLEN
#define SQL_UINT_OR_ULEN SQLULEN
#else
#define SQL_INT_OR_LEN SQLINTEGER
#define SQL_UINT_OR_ULEN SQLUINTEGER
#endif

/* for now: */
#define MAX_TOKEN 256

#define TMP_STR_LEN 1040

SQLINTEGER columnLength[MAX_TOKEN];  /* change me ! */

#include <stdio.h>
#include <pwd.h>
#include <ctype.h>

static int didBegin=0;

/*
  call SQLError to get error information and log it
 */
int
logPsgError(int level, HENV henv, HDBC hdbc, HSTMT hstmt, int dbType)
{
   SQLCHAR         sqlstate[ SQL_SQLSTATE_SIZE + 10];
   SQLINTEGER sqlcode;
   SQLSMALLINT length;
   int errorVal=-2;
   while (SQLError(henv, hdbc, hstmt, sqlstate, &sqlcode, psgErrorMsg,
		   SQL_MAX_MESSAGE_LENGTH + 1, &length) == SQL_SUCCESS) {
      if (dbType == DB_TYPE_MYSQL) {
	 if (strcmp((char *)sqlstate,"23000") == 0 && 
	     strstr((char *)psgErrorMsg, "Duplicate entry")) {
	    errorVal = CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME;
	 }
      }
      else {
	 if (strstr((char *)psgErrorMsg, "duplicate key")) {
	    errorVal = CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME;
	 }
      }
      rodsLog(level,"SQLSTATE: %s", sqlstate);
      rodsLog(level,"SQLCODE: %ld", sqlcode);
      rodsLog(level,"SQL Error message: %s", psgErrorMsg);
   }
   return(errorVal);
}

int
cllGetLastErrorMessage(char *msg, int maxChars) {
   strncpy(msg, (char *)&psgErrorMsg, maxChars);
   return(0);
}

/* 
 Allocate the environment structure for use by the SQL routines.
 */
int
cllOpenEnv(icatSessionStruct *icss) {
   RETCODE stat;

   HENV myHenv;
   stat = SQLAllocEnv(&myHenv);

   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllOpenEnv: SQLAllocEnv failed");
      return (-1);
   }

   icss->environPtr=myHenv;
   return(0);
}


/* 
 Deallocate the environment structure.
 */
int
cllCloseEnv(icatSessionStruct *icss) {
   RETCODE stat;
   HENV myHenv;

   myHenv = icss->environPtr;
   stat =SQLFreeEnv(myHenv);

   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllCloseEnv: SQLFreeEnv failed");
   }
   return(stat);
}

/*
 Connect to the DBMS.
 */
int 
cllConnect(icatSessionStruct *icss) {
   RETCODE stat;
   RETCODE stat2;

   SQLCHAR         buffer[SQL_MAX_MESSAGE_LENGTH + 1];
   SQLCHAR         sqlstate[SQL_SQLSTATE_SIZE + 1];
   SQLINTEGER      sqlcode;
   SQLSMALLINT     length;

   HDBC myHdbc;

   stat = SQLAllocConnect(icss->environPtr,
			  &myHdbc);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllConnect: SQLAllocConnect failed: %d, stat");
      return (-1);
   }

   stat = SQLConnect(myHdbc, (unsigned char *)CATALOG_ODBC_ENTRY_NAME, SQL_NTS,
		     (unsigned char *)icss->databaseUsername, SQL_NTS, 
		     (unsigned char *)icss->databasePassword, SQL_NTS);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllConnect: SQLConnect failed: %d", stat);
      rodsLog(LOG_ERROR, 
          "cllConnect: SQLConnect failed:odbcEntry=%s,user=%s,pass=%s\n",
	  CATALOG_ODBC_ENTRY_NAME,icss->databaseUsername, 
	  icss->databasePassword);
      while (SQLError(icss->environPtr,myHdbc , 0, sqlstate, &sqlcode, buffer,
                    SQL_MAX_MESSAGE_LENGTH + 1, &length) == SQL_SUCCESS) {
        rodsLog(LOG_ERROR, "cllConnect:          SQLSTATE: %s\n", sqlstate);
        rodsLog(LOG_ERROR, "cllConnect:  Native Error Code: %ld\n", sqlcode);
        rodsLog(LOG_ERROR, "cllConnect: %s \n", buffer);
    }

      stat2 = SQLDisconnect(myHdbc);
      stat2 = SQLFreeConnect(myHdbc);
      return (-1);
   }

   icss->connectPtr=myHdbc;

   if (icss->databaseType == DB_TYPE_MYSQL) {
      /* MySQL must be running in ANSI mode (or at least in
	 PIPES_AS_CONCAT mode) to be able to understand Postgres
	 SQL. STRICT_TRANS_TABLES must be st too, otherwise inserting NULL
	 into NOT NULL column does not produce error. */
      cllExecSqlNoResult ( icss, "SET SESSION autocommit=0" ) ;
      cllExecSqlNoResult ( icss, "SET SESSION sql_mode='ANSI,STRICT_TRANS_TABLES'" ) ;
   }

   return(0);
}

/*
 Connect to the DBMS for Rda access.
 */
int 
cllConnectRda(icatSessionStruct *icss) {
   RETCODE stat;
   RETCODE stat2;

   SQLCHAR         buffer[SQL_MAX_MESSAGE_LENGTH + 1];
   SQLCHAR         sqlstate[SQL_SQLSTATE_SIZE + 1];
   SQLINTEGER      sqlcode;
   SQLSMALLINT     length;

   HDBC myHdbc;

   stat = SQLAllocConnect(icss->environPtr,
			  &myHdbc);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllConnect: SQLAllocConnect failed: %d, stat");
      return (-1);
   }

   stat = SQLSetConnectOption(myHdbc,
			      SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllConnect: SQLSetConnectOption failed: %d", stat);
      return (-1);
   }

   stat = SQLConnect(myHdbc, (unsigned char *)RDA_ODBC_ENTRY_NAME, SQL_NTS, 
		     (unsigned char *)icss->databaseUsername, SQL_NTS, 
		     (unsigned char *)icss->databasePassword, SQL_NTS);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllConnect: SQLConnect failed: %d", stat);
      rodsLog(LOG_ERROR, 
         "cllConnect: SQLConnect failed:odbcEntry=%s,user=%s,pass=%s\n",
         RDA_ODBC_ENTRY_NAME,icss->databaseUsername,  icss->databasePassword);
      while (SQLError(icss->environPtr,myHdbc , 0, sqlstate, &sqlcode, buffer,
                    SQL_MAX_MESSAGE_LENGTH + 1, &length) == SQL_SUCCESS) {
        rodsLog(LOG_ERROR, "cllConnect:          SQLSTATE: %s\n", sqlstate);
        rodsLog(LOG_ERROR, "cllConnect:  Native Error Code: %ld\n", sqlcode);
        rodsLog(LOG_ERROR, "cllConnect: %s \n", buffer);
    }

      stat2 = SQLDisconnect(myHdbc);
      stat2 = SQLFreeConnect(myHdbc);
      return (-1);
   }

   icss->connectPtr=myHdbc;

   if (icss->databaseType == DB_TYPE_MYSQL) {
   /*
    MySQL must be running in ANSI mode (or at least in PIPES_AS_CONCAT
    mode) to be able to understand Postgres SQL. STRICT_TRANS_TABLES
    must be st too, otherwise inserting NULL into NOT NULL column does
    not produce error.
   */
      cllExecSqlNoResult ( icss, "SET SESSION autocommit=0" ) ;
      cllExecSqlNoResult ( icss, 
			   "SET SESSION sql_mode='ANSI,STRICT_TRANS_TABLES'" );
   }
   return(0);
}

/*
 Connect to the DBMS for database-resource/db-objects access.
 */
int 
cllConnectDbr(icatSessionStruct *icss, char *odbcEntryName) {
   RETCODE stat;
   RETCODE stat2;

   SQLCHAR         buffer[SQL_MAX_MESSAGE_LENGTH + 1];
   SQLCHAR         sqlstate[SQL_SQLSTATE_SIZE + 1];
   SQLINTEGER      sqlcode;
   SQLSMALLINT     length;

   HDBC myHdbc;

   stat = SQLAllocConnect(icss->environPtr,
			  &myHdbc);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllConnect: SQLAllocConnect failed: %d, stat");
      return (-1);
   }

   stat = SQLSetConnectOption(myHdbc,
			      SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllConnect: SQLSetConnectOption failed: %d", stat);
      return (-1);
   }

   stat = SQLConnect(myHdbc, (unsigned char *)odbcEntryName, SQL_NTS, 
		     (unsigned char *)icss->databaseUsername, SQL_NTS, 
		     (unsigned char *)icss->databasePassword, SQL_NTS);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllConnect: SQLConnect failed: %d", stat);
      rodsLog(LOG_ERROR, 
         "cllConnect: SQLConnect failed:odbcEntry=%s,user=%s,pass=%s\n",
         odbcEntryName,icss->databaseUsername,  icss->databasePassword);
      while (SQLError(icss->environPtr,myHdbc , 0, sqlstate, &sqlcode, buffer,
                    SQL_MAX_MESSAGE_LENGTH + 1, &length) == SQL_SUCCESS) {
        rodsLog(LOG_ERROR, "cllConnect:          SQLSTATE: %s\n", sqlstate);
        rodsLog(LOG_ERROR, "cllConnect:  Native Error Code: %ld\n", sqlcode);
        rodsLog(LOG_ERROR, "cllConnect: %s \n", buffer);
    }

      stat2 = SQLDisconnect(myHdbc);
      stat2 = SQLFreeConnect(myHdbc);
      return (-1);
   }

   icss->connectPtr=myHdbc;

   if (icss->databaseType == DB_TYPE_MYSQL) {
   /*
    MySQL must be running in ANSI mode (or at least in PIPES_AS_CONCAT
    mode) to be able to understand Postgres SQL. STRICT_TRANS_TABLES
    must be st too, otherwise inserting NULL into NOT NULL column does
    not produce error.
   */
      cllExecSqlNoResult ( icss, "SET SESSION autocommit=0" ) ;
      cllExecSqlNoResult ( icss, 
			   "SET SESSION sql_mode='ANSI,STRICT_TRANS_TABLES'" );
   }
   return(0);
}

/*
 This function is used to check that there are no DB-modifying SQLs pending 
 before a disconnect.  If there are, it logs a warning.

 If option is 0, record some of the sql, or clear it (if commit or rollback).
 If option is 1, issue warning the there are some pending (and include
    some of the sql).
 If option is 2, this is indicating that the previous option 0 call was
    for an audit-type SQL.
 */
#define maxPendingToRecord 5
#define pendingRecordSize 30
#define pBufferSize (maxPendingToRecord*pendingRecordSize)
int
cllCheckPending(char *sql, int option, int dbType) {
   static int pendingCount=0;
   static int pendingIx=0;
   static int pendingAudits=0;
   static char pBuffer[pBufferSize+2];
   static int firstTime=1;

   if (firstTime) {
      firstTime=0;
      memset(pBuffer, 0, pBufferSize);
   }
   if (option==0) {
      if (strncmp(sql,"commit", 6)==0 ||
	  strncmp(sql,"rollback", 8)==0) {
	 pendingIx=0;
	 pendingCount=0;
	 pendingAudits=0;
	 memset(pBuffer, 0, pBufferSize);
	 return(0);
      }
      if (pendingIx < maxPendingToRecord) {
	 strncpy((char *)&pBuffer[pendingIx*pendingRecordSize], sql, 
		 pendingRecordSize-1);
	 pendingIx++;
      }
      pendingCount++;
      return(0);
   }
   if (option==2) {
      pendingAudits++;
      return(0);
   }

   /* if there are some non-Audit pending SQL, log them */
   if (pendingCount > pendingAudits) {
      int i, max;
      int skip;
      /* but ignore a single pending "begin" which can be normal */
      if (pendingIx == 1) {
	 if (strncmp((char *)&pBuffer[0], "begin", 5)==0) {
	    return(0);
	 }
      }
      if (dbType == DB_TYPE_MYSQL) {
	 /* For mySQL, may have a few SET SESSION sql too, which we
	    should ignore */
	 skip=1;
	 if (strncmp((char *)&pBuffer[0], "begin", 5)!=0) skip=0;
	 max = maxPendingToRecord;
	 if (pendingIx < max) max = pendingIx;
	 for (i=1;i<max && skip==1;i++) {
	    if (strncmp((char *)&pBuffer[i*pendingRecordSize],
			"SET SESSION",11) !=0) {
	       skip=0;
	    }
	 }
	 if (skip) return(0);
      }

      rodsLog(LOG_NOTICE, "Warning, pending SQL at cllDisconnect, count: %d",
	      pendingCount);
      max = maxPendingToRecord;
      if (pendingIx < max) max = pendingIx;
      for (i=0;i<max;i++) {
	 rodsLog(LOG_NOTICE, "Warning, pending SQL: %s ...", 
		 (char *)&pBuffer[i*pendingRecordSize]);
      }
      if (pendingAudits > 0) {
	 rodsLog(LOG_NOTICE, "Warning, SQL will be commited with audits");
      }
   }

   if (pendingAudits > 0) {
      rodsLog(LOG_NOTICE, 
	      "Notice, pending Auditing SQL committed at cllDisconnect");
      return(1); /* tell caller (cllDisconect) to do a commit */
   }
   return(0);
}

/*
 Disconnect from the DBMS.
*/
int
cllDisconnect(icatSessionStruct *icss) {
   RETCODE stat;
   HDBC myHdbc;
   int i;

   myHdbc = icss->connectPtr;

   i = cllCheckPending("", 1, icss->databaseType);
   if (i==1) {
      i = cllExecSqlNoResult(icss, "commit"); /* auto commit any
						 pending SQLs, including
                                                 the Audit ones */
      /* Nothing to do if it fails */
   }

   stat = SQLDisconnect(myHdbc);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllDisconnect: SQLDisconnect failed: %d", stat);
      return(-1);
   }

   stat = SQLFreeConnect(myHdbc);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllDisconnect: SQLFreeConnect failed: %d", stat);
      return(-2);
   }

   icss->connectPtr = myHdbc;
   return(0);
}

/*
 Execute a SQL command which has no resulting table.  Examples include
 insert, delete, update, or ddl.
 Insert a 'begin' statement, if necessary.
 */
int
cllExecSqlNoResult(icatSessionStruct *icss, char *sql)
{
   int status;

   if (strncmp(sql,"commit", 6)==0 ||
       strncmp(sql,"rollback", 8)==0) {
      didBegin=0;
   }
   else {
      if (didBegin==0) {
	 status = _cllExecSqlNoResult(icss, "begin", 1);
	 if (status != SQL_SUCCESS) return(status);
      }
      didBegin=1;
   }
   return (_cllExecSqlNoResult(icss, sql, 0));
}

/*
 Log the bind variables from the global array (after an error)
*/
void
logTheBindVariables(int level)
{
   int i;
   char tmpStr[TMP_STR_LEN+2];
   for (i=0;i<cllBindVarCountPrev;i++) {
      snprintf(tmpStr, TMP_STR_LEN, "bindVar[%d]=%s", i+1, cllBindVars[i]);
      rodsLog(level, tmpStr);
   }
}

/*
 Bind variables from the global array.
 */
int
bindTheVariables(HSTMT myHstmt, char *sql) {
   int myBindVarCount;
   RETCODE stat;
   int i;
   char tmpStr[TMP_STR_LEN+2];

   myBindVarCount = cllBindVarCount;
   cllBindVarCountPrev=cllBindVarCount; /* save in case we need to log error */
   cllBindVarCount = 0; /* reset for next call */

   if (myBindVarCount > 0) {
      rodsLogSql("SQLPrepare");
      stat = SQLPrepare(myHstmt,  (unsigned char *)sql, SQL_NTS);
      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, "bindTheVariables: SQLPrepare failed: %d",
		 stat);
	 return(-1);
      }

      for (i=0;i<myBindVarCount;i++) {
	 stat = SQLBindParameter(myHstmt, i+1, SQL_PARAM_INPUT, SQL_C_CHAR,
				 SQL_C_CHAR, 0, 0, cllBindVars[i], 0, 0);
	 snprintf(tmpStr, TMP_STR_LEN, "bindVar[%d]=%s", i+1, cllBindVars[i]);
	 rodsLogSql(tmpStr);
	 if (stat != SQL_SUCCESS) {
	    rodsLog(LOG_ERROR, 
		    "bindTheVariables: SQLBindParameter failed: %d", stat);
	    return(-1);
	 }
      }

      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, "bindTheVariables: SQLAllocStmt failed: %d",
		 stat);
	 return(-1);
      }
   }
   return(0);
}

/*
   Case-insensitive string comparison, first string can be any case and
   contain leading and trailing spaces, second string must be lowercase, 
   no spaces.
*/
#ifdef NEW_ODBC
static int cmp_stmt (char *str1, char *str2)
{
  /* skip leading spaces */
  while ( isspace(*str1) ) ++str1 ;
  
  /* start comparing */
  for ( ; *str1 && *str2 ; ++str1, ++str2 ) {
    if ( tolower(*str1) != *str2 ) return 0 ;
  }
  
  /* skip trailing spaces */
  while ( isspace(*str1) ) ++str1 ;

  /* if we are at the end of the strings then they are equal */
  return *str1 == *str2 ;
}
#endif

/*
 Execute a SQL command which has no resulting table.  With optional
 bind variables.
 If option is 1, skip the bind variables.
 */
int
_cllExecSqlNoResult(icatSessionStruct *icss, char *sql,
		   int option) {
   RETCODE stat;
   HDBC myHdbc;
   HSTMT myHstmt;
   int result;
   char *status;
   SQL_INT_OR_LEN rowCount;
#ifdef NEW_ODBC
   int i;
#endif
   myHdbc = icss->connectPtr;
   rodsLog(LOG_DEBUG1, sql);
   stat = SQLAllocStmt(myHdbc, &myHstmt); 
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "_cllExecSqlNoResult: SQLAllocStmt failed: %d", stat);
      return(-1);
   }

#if 0
   if (bindVar1 != 0 && *bindVar1 != '\0') {
      stat = SQLBindParameter(myHstmt, 1, SQL_PARAM_INPUT, SQL_C_SBIGINT,
			      SQL_C_SBIGINT, 0, 0, 0, 0, 0);
      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, "_cllExecSqlNoResult: SQLAllocStmt failed: %d",
		 stat);
	 return(-1);
      }
   }
#endif

   if (option==0) {
      if (bindTheVariables(myHstmt, sql) != 0) return(-1);
   }

   rodsLogSql(sql);

   stat = SQLExecDirect(myHstmt, (unsigned char *)sql, SQL_NTS);
   status = "UNKNOWN";
   if (stat == SQL_SUCCESS) status= "SUCCESS";
   if (stat == SQL_SUCCESS_WITH_INFO) status="SUCCESS_WITH_INFO";
   if (stat == SQL_NO_DATA_FOUND) status="NO_DATA";
   if (stat == SQL_ERROR) status="SQL_ERROR";
   if (stat == SQL_INVALID_HANDLE) status="HANDLE_ERROR";
   rodsLogSqlResult(status);

   if (stat == SQL_SUCCESS || stat == SQL_SUCCESS_WITH_INFO ||
      stat == SQL_NO_DATA_FOUND) {
      cllCheckPending(sql, 0, icss->databaseType);
      result = 0;
      if (stat == SQL_NO_DATA_FOUND) result = CAT_SUCCESS_BUT_WITH_NO_INFO;
#ifdef NEW_ODBC
      /* ODBC says that if statement is not UPDATE, INSERT, or DELETE then
         SQLRowCount may return anything. So for BEGIN, COMMIT and ROLLBACK
	 we don't want to call it but just return OK.
      */
      if ( ! cmp_stmt(sql,"begin") && ! cmp_stmt(sql,"commit") && ! cmp_stmt(sql,"rollback") ) {
	 /* Doesn't seem to return SQL_NO_DATA_FOUND, so check */
	 i = SQLRowCount (myHstmt, (SQL_INT_OR_LEN *)&rowCount);
	 if (i) {
	    /* error getting rowCount???, just call it no_info */
	    result = CAT_SUCCESS_BUT_WITH_NO_INFO;
	 }
	 if (rowCount==0) result = CAT_SUCCESS_BUT_WITH_NO_INFO;
      }
#else
      rowCount=0; /* avoid compiler warning */
#endif
   }
   else {
      if (option==0) {
	 logTheBindVariables(LOG_NOTICE);
      }
      rodsLog(LOG_NOTICE,"_cllExecSqlNoResult: SQLExecDirect error: %d sql:%s",
	      stat, sql);
      result = logPsgError(LOG_NOTICE, icss->environPtr, myHdbc, myHstmt,
			   icss->databaseType);
   }

   stat = SQLFreeStmt(myHstmt, SQL_DROP);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "_cllExecSqlNoResult: SQLFreeStmt error: %d", stat);
   }

   return(result);
}

/* 
  Execute a SQL command that returns a result table, and
  and bind the default row.
  This version now uses the global array of bind variables.
*/
int
cllExecSqlWithResult(icatSessionStruct *icss, int *stmtNum, char *sql) {

   RETCODE stat;
   HDBC myHdbc;
   HSTMT hstmt;	
   SQLSMALLINT numColumns;

   SQLCHAR         colName[MAX_TOKEN];
   SQLSMALLINT     colType;
   SQLSMALLINT     colNameLen;
   SQL_UINT_OR_ULEN precision;
   SQLSMALLINT     scale;
   SQL_INT_OR_LEN  displaysize;
#ifndef NEW_ODBC
   static SQLINTEGER resultDataSize;
#endif

   icatStmtStrct *myStatement;

   int i;
   int statementNumber;
   char *status;

/* In 2.2 and some versions before, this would call
   _cllExecSqlNoResult with "begin", similar to how cllExecSqlNoResult
   does.  But since this function is called for 'select's, this is not
   needed here, and in fact causes postgres processes to be in the
   'idle in transaction' state which prevents some operations (such as
   backup).  So this was removed. */

   myHdbc = icss->connectPtr;
   rodsLog(LOG_DEBUG1, sql);
   stat = SQLAllocStmt(myHdbc, &hstmt); 
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllExecSqlWithResult: SQLAllocStmt failed: %d",
	      stat);
      return(-1);
   }

   statementNumber=-1;
   for (i=0;i<MAX_NUM_OF_CONCURRENT_STMTS && statementNumber<0;i++) {
      if (icss->stmtPtr[i]==0) {
	 statementNumber=i;
      }
   }
   if (statementNumber<0) {
      rodsLog(LOG_ERROR, 
	      "cllExecSqlWithResult: too many concurrent statements");
      return(-2);
   }

   myStatement = (icatStmtStrct *)malloc(sizeof(icatStmtStrct));
   icss->stmtPtr[statementNumber]=myStatement;

   myStatement->stmtPtr=hstmt;

   if (bindTheVariables(hstmt, sql) != 0) return(-1);

   rodsLogSql(sql);

   stat = SQLExecDirect(hstmt, (unsigned char *)sql, SQL_NTS);
   status = "UNKNOWN";
   if (stat == SQL_SUCCESS) status= "SUCCESS";
   if (stat == SQL_SUCCESS_WITH_INFO) status="SUCCESS_WITH_INFO";
   if (stat == SQL_NO_DATA_FOUND) status="NO_DATA";
   if (stat == SQL_ERROR) status="SQL_ERROR";
   if (stat == SQL_INVALID_HANDLE) status="HANDLE_ERROR";
   rodsLogSqlResult(status);

   if (stat == SQL_SUCCESS ||
       stat == SQL_SUCCESS_WITH_INFO || 
       stat == SQL_NO_DATA_FOUND) {
   }
   else {
      logTheBindVariables(LOG_NOTICE);
      rodsLog(LOG_NOTICE, 
	      "cllExecSqlWithResult: SQLExecDirect error: %d, sql:%s",
	      stat, sql);
      logPsgError(LOG_NOTICE, icss->environPtr, myHdbc, hstmt,
		  icss->databaseType);
      return(-1);
   }

   stat =  SQLNumResultCols(hstmt, &numColumns);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllExecSqlWithResult: SQLNumResultCols failed: %d",
	      stat);
      return(-2);
   }
   myStatement->numOfCols=numColumns;

   for (i = 0; i<numColumns; i++) {
      stat = SQLDescribeCol(hstmt, i+1, colName, sizeof(colName),
			    &colNameLen, &colType, &precision, &scale, NULL);
      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, "cllExecSqlWithResult: SQLDescribeCol failed: %d",
	      stat);
	 return(-3);
      }
      /*  printf("colName='%s' precision=%d\n",colName, precision); */
      columnLength[i]=precision;
      stat = SQLColAttributes(hstmt, i+1, SQL_COLUMN_DISPLAY_SIZE, 
			      NULL, 0, NULL, &displaysize);
      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, 
		 "cllExecSqlWithResult: SQLColAttributes failed: %d",
		 stat);
	 return(-3);
      }

      if (displaysize > ((int)strlen((char *) colName))) {
	 columnLength[i] = displaysize + 1;
      }
      else {
	 columnLength[i] = strlen((char *) colName) + 1;
      }
      /*      printf("columnLength[%d]=%d\n",i,columnLength[i]); */
 
      myStatement->resultValue[i] = (char*)malloc((int)columnLength[i]);

      strcpy((char *)myStatement->resultValue[i],"");

#if NEW_ODBC
      stat = SQLBindCol(hstmt, i+1, SQL_C_CHAR, myStatement->resultValue[i], 
			columnLength[i], NULL);
      /* The last argument could be resultDataSize (a SQLINTEGER
         location), which will be returned later via the SQLFetch.
         Since unused now, passing in NULL tells ODBC to skip it */
#else
      /* The old ODBC needs a non-NULL value */
      stat = SQLBindCol(hstmt, i+1, SQL_C_CHAR, myStatement->resultValue[i], 
			columnLength[i], &resultDataSize);
#endif

      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, 
		 "cllExecSqlWithResult: SQLColAttributes failed: %d",
		 stat);
	 return(-4);
      }


      myStatement->resultColName[i] = (char*)malloc((int)columnLength[i]);
      strncpy(myStatement->resultColName[i], (char *)colName, columnLength[i]);

   }
   *stmtNum = statementNumber;
   return(0);
}

/* logBindVars 
   For when an error occurs, log the bind variables which were used
   with the sql.
*/
void
logBindVars(int level, char *bindVar1, char *bindVar2, char *bindVar3,
		char *bindVar4, char *bindVar5, char *bindVar6) {
   if (bindVar1 != 0 && *bindVar1 != '\0') {
      rodsLog(level,"bindVar1=%s", bindVar1);
   }
   if (bindVar2 != 0 && *bindVar2 != '\0') {
      rodsLog(level,"bindVar2=%s", bindVar2);
   }
   if (bindVar3 != 0 && *bindVar3 != '\0') {
      rodsLog(level,"bindVar3=%s", bindVar3);
   }
   if (bindVar4 != 0 && *bindVar4 != '\0') {
      rodsLog(level,"bindVar4=%s", bindVar4);
   }
}


/* 
  Execute a SQL command that returns a result table, and
  and bind the default row; and allow optional bind variables.
*/
int
cllExecSqlWithResultBV(icatSessionStruct *icss, int *stmtNum, char *sql,
 			 char *bindVar1, char *bindVar2, char *bindVar3,
			 char *bindVar4, char *bindVar5, char *bindVar6) {

   RETCODE stat;
   HDBC myHdbc;
   HSTMT hstmt;	
   SQLSMALLINT numColumns;

   SQLCHAR         colName[MAX_TOKEN];
   SQLSMALLINT     colType;
   SQLSMALLINT     colNameLen;
   SQL_UINT_OR_ULEN precision;
   SQLSMALLINT     scale;
   SQL_INT_OR_LEN  displaysize;
#ifndef NEW_ODBC
   static SQLINTEGER resultDataSize;
#endif

   icatStmtStrct *myStatement;

   int i;
   int statementNumber;
   char *status;
   char tmpStr[TMP_STR_LEN+2];

   myHdbc = icss->connectPtr;
   rodsLog(LOG_DEBUG1, sql);
   stat = SQLAllocStmt(myHdbc, &hstmt); 
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllExecSqlWithResultBV: SQLAllocStmt failed: %d",
	      stat);
      return(-1);
   }

   statementNumber=-1;
   for (i=0;i<MAX_NUM_OF_CONCURRENT_STMTS && statementNumber<0;i++) {
      if (icss->stmtPtr[i]==0) {
	 statementNumber=i;
      }
   }
   if (statementNumber<0) {
      rodsLog(LOG_ERROR, 
	      "cllExecSqlWithResultBV: too many concurrent statements");
      return(-2);
   }

   myStatement = (icatStmtStrct *)malloc(sizeof(icatStmtStrct));
   icss->stmtPtr[statementNumber]=myStatement;

   myStatement->stmtPtr=hstmt;

   if ((bindVar1 != 0 && *bindVar1 != '\0')  ||
       (bindVar2 != 0 && *bindVar2 != '\0')  ||
       (bindVar3 != 0 && *bindVar3 != '\0')  ||
       (bindVar4 != 0 && *bindVar4 != '\0')) {

      rodsLogSql("SQLPrepare");
      stat = SQLPrepare(hstmt,  (unsigned char *)sql, SQL_NTS);
      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, "cllExecSqlNoResult: SQLPrepare failed: %d",
		 stat);
	 return(-1);
      }

      if (bindVar1 != 0 && *bindVar1 != '\0') {
	  stat = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
	 			 SQL_C_CHAR, 0, 0, bindVar1, 0, 0);
	  snprintf(tmpStr, TMP_STR_LEN, 
		   "bindVar1=%s", bindVar1);
	  rodsLogSql(tmpStr);
	  if (stat != SQL_SUCCESS) {
	     rodsLog(LOG_ERROR, 
		     "cllExecSqlNoResult: SQLBindParameter failed: %d", stat);
	     return(-1);
	 }
      }
      if (bindVar2 != 0 && *bindVar2 != '\0') {
	 stat = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,
				 SQL_C_CHAR, 0, 0, bindVar2, 0, 0);
	 snprintf(tmpStr, TMP_STR_LEN, 
		  "bindVar2=%s", bindVar2);
	 rodsLogSql(tmpStr);
	 if (stat != SQL_SUCCESS) {
	    rodsLog(LOG_ERROR, 
		    "cllExecSqlNoResult: SQLBindParameter failed: %d", stat);
	    return(-1);
	 }
      }
      if (bindVar3 != 0 && *bindVar3 != '\0') {
	 stat = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR,
				 SQL_C_CHAR, 0, 0, bindVar3, 0, 0);
	 snprintf(tmpStr, TMP_STR_LEN, 
		   "bindVar3=%s", bindVar3);
	 rodsLogSql(tmpStr);
	 if (stat != SQL_SUCCESS) {
	    rodsLog(LOG_ERROR, "cllExecSqlNoResult: SQLBindParameter failed: %d",
		    stat);
	    return(-1);
	 }
      }
      if (bindVar4 != 0 && *bindVar4 != '\0') {
	 stat = SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR,
				 SQL_C_CHAR, 0, 0, bindVar4, 0, 0);
	 snprintf(tmpStr, TMP_STR_LEN, 
		   "bindVar4=%s", bindVar4);
	 rodsLogSql(tmpStr);
	 if (stat != SQL_SUCCESS) {
	    rodsLog(LOG_ERROR, "cllExecSqlNoResult: SQLBindParameter failed: %d",
		    stat);
	    return(-1);
	 }
      }
      if (bindVar5 != 0 && *bindVar5 != '\0') {
	 stat = SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_CHAR,
				 SQL_C_CHAR, 0, 0, bindVar5, 0, 0);
	 snprintf(tmpStr, TMP_STR_LEN, 
		   "bindVar5=%s", bindVar5);
	 rodsLogSql(tmpStr);
	 if (stat != SQL_SUCCESS) {
	    rodsLog(LOG_ERROR, "cllExecSqlNoResult: SQLBindParameter failed: %d",
		    stat);
	    return(-1);
	 }
      }
      rodsLogSql(sql);
      stat = SQLExecute(hstmt);
   }
   else {
      rodsLogSql(sql);
      stat = SQLExecDirect(hstmt, (unsigned char *)sql, SQL_NTS);
   }

   status = "UNKNOWN";
   if (stat == SQL_SUCCESS) status= "SUCCESS";
   if (stat == SQL_SUCCESS_WITH_INFO) status="SUCCESS_WITH_INFO";
   if (stat == SQL_NO_DATA_FOUND) status="NO_DATA";
   if (stat == SQL_ERROR) status="SQL_ERROR";
   if (stat == SQL_INVALID_HANDLE) status="HANDLE_ERROR";
   rodsLogSqlResult(status);

   if (stat == SQL_SUCCESS ||
       stat == SQL_SUCCESS_WITH_INFO || 
       stat == SQL_NO_DATA_FOUND) {
   }
   else {
      logBindVars(LOG_NOTICE, bindVar1, bindVar2, bindVar3, bindVar4,
		  bindVar5, bindVar6);
      rodsLog(LOG_NOTICE, 
	      "cllExecSqlWithResultBV: SQLExecDirect error: %d, sql:%s",
	      stat, sql);
      logPsgError(LOG_NOTICE, icss->environPtr, myHdbc, hstmt,
		  icss->databaseType);
      return(-1);
   }

   stat =  SQLNumResultCols(hstmt, &numColumns);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllExecSqlWithResultBV: SQLNumResultCols failed: %d",
	      stat);
      return(-2);
   }
   myStatement->numOfCols=numColumns;

   for (i = 0; i<numColumns; i++) {
      stat = SQLDescribeCol(hstmt, i+1, colName, sizeof(colName),
			    &colNameLen, &colType, &precision, &scale, NULL);
      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, "cllExecSqlWithResultBV: SQLDescribeCol failed: %d",
	      stat);
	 return(-3);
      }
      /*  printf("colName='%s' precision=%d\n",colName, precision); */
      columnLength[i]=precision;
      stat = SQLColAttributes(hstmt, i+1, SQL_COLUMN_DISPLAY_SIZE, 
			      NULL, 0, NULL, &displaysize);
      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, 
		 "cllExecSqlWithResultBV: SQLColAttributes failed: %d",
		 stat);
	 return(-3);
      }

      if (displaysize > ((int)strlen((char *) colName))) {
	 columnLength[i] = displaysize + 1;
      }
      else {
	 columnLength[i] = strlen((char *) colName) + 1;
      }
      /*      printf("columnLength[%d]=%d\n",i,columnLength[i]); */
 
      myStatement->resultValue[i] = (char*)malloc((int)columnLength[i]);

      strcpy((char *)myStatement->resultValue[i],"");

#ifdef NEW_ODBC
      stat = SQLBindCol(hstmt, i+1, SQL_C_CHAR, myStatement->resultValue[i], 
			columnLength[i], NULL);
      /* The last argument could be resultDataSize (a SQLINTEGER
         location), which will be returned later via the SQLFetch.
         Since unused now, passing in NULL tells ODBC to skip it */
#else
      /* The old ODBC needs a non-NULL value */
      stat = SQLBindCol(hstmt, i+1, SQL_C_CHAR, myStatement->resultValue[i], 
			columnLength[i], &resultDataSize);
#endif

      if (stat != SQL_SUCCESS) {
	 rodsLog(LOG_ERROR, 
		 "cllExecSqlWithResultBV: SQLColAttributes failed: %d",
		 stat);
	 return(-4);
      }


      myStatement->resultColName[i] = (char*)malloc((int)columnLength[i]);
      strncpy(myStatement->resultColName[i], (char *)colName, columnLength[i]);

   }
   *stmtNum = statementNumber;
   return(0);
}

/*
  Return a row from a previous cllExecSqlWithResult call.
 */
int
cllGetRow(icatSessionStruct *icss, int statementNumber) {
   HSTMT hstmt;
   RETCODE stat;
   int nCols, i;
   icatStmtStrct *myStatement;
   int logGetRows=0; /* Another handy debug flag.  When set and if
                        spLogSql is set, this function will log each
                        time a row is gotten, the number of columns ,
                        and the contents of the first column.  */

   myStatement=icss->stmtPtr[statementNumber];
   hstmt = myStatement->stmtPtr;
   nCols = myStatement->numOfCols;

   for (i=0;i<nCols;i++) {
      strcpy((char *)myStatement->resultValue[i],"");
   }
   stat =  SQLFetch(hstmt);
   if (stat != SQL_SUCCESS && stat != SQL_NO_DATA_FOUND) {
      rodsLog(LOG_ERROR, "cllGetRow: SQLFetch failed: %d", stat);
      return(-1);
   }
   if (stat == SQL_NO_DATA_FOUND) {
      if (logGetRows) {
	 rodsLogSql("cllGetRow: NO DATA FOUND");
      }
      _cllFreeStatementColumns(icss,statementNumber);
      myStatement->numOfCols=0;
   }
   else {
      if (logGetRows) {
	 char tmpstr[210];
	 snprintf(tmpstr, 200, "cllGetRow columns:%d first column: %s", nCols, 
		  myStatement->resultValue[0]);
	 rodsLogSql(tmpstr);
      }
   }
   return(0);
}

/* 
  Return the string needed to get the next value in a sequence item.
  The syntax varies between RDBMSes, so it is here, in the DBMS-specific code.
 */
int
cllNextValueString(char *itemName, char *outString, int maxSize) {
#ifdef MY_ICAT
   snprintf(outString, maxSize, "%s_nextval()", itemName);
#else
   snprintf(outString, maxSize, "nextval('%s')", itemName);
#endif
   return 0;
}

int
cllGetRowCount(icatSessionStruct *icss, int statementNumber) {
   int i;
   HSTMT hstmt;
   icatStmtStrct *myStatement;
   SQL_INT_OR_LEN RowCount;

   myStatement=icss->stmtPtr[statementNumber];
   hstmt = myStatement->stmtPtr;

   i = SQLRowCount (hstmt, (SQL_INT_OR_LEN *)&RowCount);
   if (i) return(i);
   return(RowCount);
}

int
cllCurrentValueString(char *itemName, char *outString, int maxSize) {
#ifdef MY_ICAT
   snprintf(outString, maxSize, "%s_currval()", itemName);
#else
   snprintf(outString, maxSize, "currval('%s')", itemName);
#endif
   return 0;
}

/* 
  Free a statement (from a previous cllExecSqlWithResult call) and the
  corresponding resultValue array.
*/
int
cllFreeStatement(icatSessionStruct *icss, int statementNumber) {
   HSTMT hstmt;
   RETCODE stat;
   int i;

   icatStmtStrct *myStatement;

   myStatement=icss->stmtPtr[statementNumber];
   if (myStatement==NULL) { /* already freed */
      return(0);
   }
   hstmt = myStatement->stmtPtr;

   for (i=0;i<myStatement->numOfCols;i++) {
      free(myStatement->resultValue[i]);
      free(myStatement->resultColName[i]);
   }

   stat = SQLFreeStmt(hstmt, SQL_DROP);
   if (stat != SQL_SUCCESS) {
      rodsLog(LOG_ERROR, "cllFreeStatement SQLFreeStmt error: %d", stat);
   }

   free(myStatement);

   icss->stmtPtr[statementNumber]=0;  /* indicate that the statement is free */

   return (0);
}

/* 
  Free the statement columns (from a previous cllExecSqlWithResult call),
  but not the whole statement.
*/
int
_cllFreeStatementColumns(icatSessionStruct *icss, int statementNumber) {
   HSTMT hstmt;
   int i;

   icatStmtStrct *myStatement;

   myStatement=icss->stmtPtr[statementNumber];
   hstmt = myStatement->stmtPtr;

   for (i=0;i<myStatement->numOfCols;i++) {
      free(myStatement->resultValue[i]);
      free(myStatement->resultColName[i]);
   }
   return (0);
}

/*
 A few tests to verify basic functionality (including talking with
 the database via ODBC). 
 */
int cllTest(char *userArg, char *pwArg) {
   int i;
   int j, k;
   int OK;
   int stmt;
   int numOfCols;
   char userName[500];
   int ival;

   struct passwd *ppasswd;
   icatSessionStruct icss;

   icss.stmtPtr[0]=0;
   rodsLogSqlReq(1);
   OK=1;
   i = cllOpenEnv(&icss);
   if (i != 0) OK=0;

   if (userArg==0 || *userArg=='\0') {
      ppasswd = getpwuid(getuid());    /* get user passwd entry             */
      strcpy(userName,ppasswd->pw_name);  /* get user name                  */
   }
   else {
      strncpy(userName, userArg, 500);
   }
   printf("userName=%s\n",userName);
   printf("password=%s\n",pwArg);

   strncpy(icss.databaseUsername,userName, DB_USERNAME_LEN);

   if (pwArg==0 || *pwArg=='\0') {
      strcpy(icss.databasePassword,"");
   }
   else {
      strncpy(icss.databasePassword,pwArg, DB_PASSWORD_LEN);
   }

   i = cllConnect(&icss);
   if (i != 0) exit(-1);

   i = cllExecSqlNoResult(&icss,"create table test (i integer, j integer, a varchar(32))");
   if (i != 0 && i != CAT_SUCCESS_BUT_WITH_NO_INFO) OK=0;

   i = cllExecSqlNoResult(&icss, "insert into test values (2, 3, 'a')");
   if (i != 0) OK=0;

   i = cllExecSqlNoResult(&icss, "commit");
   if (i != 0) OK=0;

   i = cllExecSqlNoResult(&icss, "bad sql");
   if (i == 0) OK=0;   /* should fail, if not it's not OK */
   i = cllExecSqlNoResult(&icss, "rollback"); /* close the bad transaction*/

   i = cllExecSqlNoResult(&icss, "delete from test where i = 1");
   if (i != 0 && i != CAT_SUCCESS_BUT_WITH_NO_INFO) OK=0;

   i = cllExecSqlNoResult(&icss, "commit");
   if (i != 0) OK=0;

   i = cllExecSqlWithResultBV(&icss, &stmt, 
				"select * from test where a = ?",
				"a",0 ,0,0,0,0);
   if (i != 0) OK=0;

   if (i == 0) {
      numOfCols = 1;
      for (j=0;j<10 && numOfCols>0;j++) {
	 i = cllGetRow(&icss, stmt);
	 if (i != 0) {
	    OK=0;
	 }
	 else {
	    numOfCols = icss.stmtPtr[stmt]->numOfCols;
	    if (numOfCols == 0) {
	       printf("No more rows returned\n");
	       i = cllFreeStatement(&icss,stmt);
	    }
	    else {
	       for (k=0; k<numOfCols || k < icss.stmtPtr[stmt]->numOfCols; k++){
		  printf("resultValue[%d]=%s\n",k,
			 icss.stmtPtr[stmt]->resultValue[k]);
	       }
	    }
	 }
      }
   }

   ival=2;
   i = cllExecSqlWithResultBV(&icss, &stmt, 
				"select * from test where i = ?",
				"2",0,0,0,0,0);
   if (i != 0) OK=0;

   if (i == 0) {
      numOfCols = 1;
      for (j=0;j<10 && numOfCols>0;j++) {
	 i = cllGetRow(&icss, stmt);
	 if (i != 0) {
	    OK=0;
	 }
	 else {
	    numOfCols = icss.stmtPtr[stmt]->numOfCols;
	    if (numOfCols == 0) {
	       printf("No more rows returned\n");
	       i = cllFreeStatement(&icss,stmt);
	    }
	    else {
	       for (k=0; k<numOfCols || k < icss.stmtPtr[stmt]->numOfCols; k++){
		  printf("resultValue[%d]=%s\n",k,
			 icss.stmtPtr[stmt]->resultValue[k]);
	       }
	    }
	 }
      }
   }

   i = cllExecSqlNoResult(&icss,"drop table test;");
   if (i != 0 && i != CAT_SUCCESS_BUT_WITH_NO_INFO) OK=0;

   i = cllExecSqlNoResult(&icss, "commit");
   if (i != 0) OK=0;

   i = cllDisconnect(&icss);
   if (i != 0) OK=0;

   i = cllCloseEnv(&icss);
   if (i != 0) OK=0;

   if (OK) {
      printf("The tests all completed normally\n");
      return(0);
   }
   else {
      printf("One or more tests DID NOT complete normally\n");
      return(-1);
   }
}
