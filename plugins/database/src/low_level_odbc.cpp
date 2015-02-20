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

#include "low_level_odbc.hpp"

#include "irods_log.hpp"
#include "irods_stacktrace.hpp"

int _cllFreeStatementColumns( icatSessionStruct *icss, int statementNumber );

int
_cllExecSqlNoResult( icatSessionStruct *icss, const char *sql, int option );


int cllBindVarCount = 0;
const char *cllBindVars[MAX_BIND_VARS];
int cllBindVarCountPrev = 0; /* cllBindVarCount earlier in processing */

SQLCHAR  psgErrorMsg[SQL_MAX_MESSAGE_LENGTH + 10];
const static SQLLEN GLOBAL_SQL_NTS = SQL_NTS;

/* Different argument types are needed on at least Ubuntu 11.04 on a
   64-bit host when using MySQL, but may or may not apply to all
   64-bit hosts.  The ODBCVER in sql.h is the same, 0x0351, but some
   of the defines differ.  If it's using new defines and this isn't
   used, there may be compiler warnings but it might link OK, but not
   operate correctly consistently. */
#define SQL_INT_OR_LEN SQLLEN
#define SQL_UINT_OR_ULEN SQLULEN

/* for now: */
#define MAX_TOKEN 256

#define TMP_STR_LEN 1040

SQLINTEGER columnLength[MAX_TOKEN];  /* change me ! */

#include <stdio.h>
#include <pwd.h>
#include <ctype.h>

#include <vector>
#include <string>

static int didBegin = 0;
static int noResultRowCount = 0;

// =-=-=-=-=-=-=-
// JMC :: Needed to add this due to crash issues with the SQLBindCol + SQLFetch
//     :: combination where the fetch fails if a var is not passed to the bind for
//     :: the result data size
static const short MAX_NUMBER_ICAT_COLUMS = 32;
static SQLLEN resultDataSizeArray[ MAX_NUMBER_ICAT_COLUMS ];


/*
  call SQLError to get error information and log it
*/
int
logPsgError( int level, HENV henv, HDBC hdbc, HSTMT hstmt, int dbType ) {
    SQLCHAR         sqlstate[ SQL_SQLSTATE_SIZE + 10];
    SQLINTEGER sqlcode;
    SQLSMALLINT length;
    int errorVal = -2;
    while ( SQLError( henv, hdbc, hstmt, sqlstate, &sqlcode, psgErrorMsg,
                      SQL_MAX_MESSAGE_LENGTH + 1, &length ) == SQL_SUCCESS ) {
        if ( dbType == DB_TYPE_MYSQL ) {
            if ( strcmp( ( char * )sqlstate, "23000" ) == 0 &&
                    strstr( ( char * )psgErrorMsg, "Duplicate entry" ) ) {
                errorVal = CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME;
            }
        }
        else {
            if ( strstr( ( char * )psgErrorMsg, "duplicate key" ) ) {
                errorVal = CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME;
            }
        }
        rodsLog( level, "SQLSTATE: %s", sqlstate );
        rodsLog( level, "SQLCODE: %ld", sqlcode );
        rodsLog( level, "SQL Error message: %s", psgErrorMsg );
    }
    return errorVal;
}

int
cllGetLastErrorMessage( char *msg, int maxChars ) {
    strncpy( msg, ( char * )&psgErrorMsg, maxChars );
    return 0;
}

/*
   Allocate the environment structure for use by the SQL routines.
*/
int
cllOpenEnv( icatSessionStruct *icss ) {

    HENV myHenv;
    if ( SQLAllocEnv( &myHenv ) != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllOpenEnv: SQLAllocHandle failed for env" );
        return -1;
    }

    icss->environPtr = myHenv;
    return 0;
}


/*
   Deallocate the environment structure.
*/
int
cllCloseEnv( icatSessionStruct *icss ) {

    SQLRETURN stat = SQLFreeEnv( icss->environPtr );

    if ( stat == SQL_SUCCESS ) {
        icss->environPtr = NULL;
    }
    else {
        rodsLog( LOG_ERROR, "cllCloseEnv: SQLFreeEnv failed" );
    }
    return stat;
}

/*
  Connect to the DBMS.
*/
int
cllConnect( icatSessionStruct *icss ) {

    HDBC myHdbc;
    SQLRETURN stat = SQLAllocHandle( SQL_HANDLE_DBC, icss->environPtr, &myHdbc );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllConnect: SQLAllocHandle failed for connect: %s", stat );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // ODBC Entry is defined by the DB type or an env variable
    char odbcEntryName[ DB_TYPENAME_LEN ];
    char* odbc_env = getenv( "irodsOdbcDSN" );
    if ( odbc_env ) {
        rodsLog( LOG_DEBUG, "Setting ODBC entry to ENV [%s]", odbc_env );
        snprintf( odbcEntryName, sizeof( odbcEntryName ), "%s", odbc_env );
    }
    else {
        snprintf( odbcEntryName, sizeof( odbcEntryName ), "%s", icss->database_plugin_type );
    }

    // =-=-=-=-=-=-=-
    // initialize a connection to the catalog
    stat = SQLConnect(
               myHdbc,
               ( unsigned char * )odbcEntryName, strlen( odbcEntryName ),
               ( unsigned char * )icss->databaseUsername, strlen( icss->databaseUsername ),
               ( unsigned char * )icss->databasePassword, strlen( icss->databasePassword ) );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllConnect: SQLConnect failed: %d", stat );
        rodsLog( LOG_ERROR,
                 "cllConnect: SQLConnect failed:odbcEntry=%s,user=%s,pass=XXXXX\n",
                 odbcEntryName,
                 icss->databaseUsername );
        SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
        SQLINTEGER sqlcode;
        SQLCHAR buffer[SQL_MAX_MESSAGE_LENGTH + 1];
        SQLSMALLINT length;
        while ( SQLError( icss->environPtr, myHdbc , 0, sqlstate, &sqlcode, buffer,
                          SQL_MAX_MESSAGE_LENGTH + 1, &length ) == SQL_SUCCESS ) {
            rodsLog( LOG_ERROR, "cllConnect:          SQLSTATE: %s\n", sqlstate );
            rodsLog( LOG_ERROR, "cllConnect:  Native Error Code: %ld\n", sqlcode );
            rodsLog( LOG_ERROR, "cllConnect: %s \n", buffer );
        }

        SQLDisconnect( myHdbc );
        SQLFreeHandle( SQL_HANDLE_DBC, myHdbc );
        return -1;
    }

    icss->connectPtr = myHdbc;

    if ( icss->databaseType == DB_TYPE_MYSQL ) {
        /* MySQL must be running in ANSI mode (or at least in
           PIPES_AS_CONCAT mode) to be able to understand Postgres
           SQL. STRICT_TRANS_TABLES must be st too, otherwise inserting NULL
           into NOT NULL column does not produce error. */
        cllExecSqlNoResult( icss, "SET SESSION autocommit=0" ) ;
        cllExecSqlNoResult( icss, "SET SESSION sql_mode='ANSI,STRICT_TRANS_TABLES'" ) ;
        cllExecSqlNoResult( icss, "SET character_set_client = utf8" ) ;
        cllExecSqlNoResult( icss, "SET character_set_results = utf8" ) ;
        cllExecSqlNoResult( icss, "SET character_set_connection = utf8" ) ;
    }

    return 0;
}

/*
  Connect to the DBMS for Rda access.
*/
int
cllConnectRda( icatSessionStruct *icss ) {
    HDBC myHdbc;
    SQLRETURN stat = SQLAllocHandle( SQL_HANDLE_DBC, icss->environPtr, &myHdbc );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllConnect: SQLAllocHandle failed for connect: %d", stat );
        return -1;
    }

    stat = SQLSetConnectOption( myHdbc,
                                SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllConnect: SQLSetConnectOption failed: %d", stat );
        return -1;
    }

    stat = SQLConnect( myHdbc, ( unsigned char * )RDA_ODBC_ENTRY_NAME, strlen( RDA_ODBC_ENTRY_NAME ),
                       ( unsigned char * )icss->databaseUsername, strlen( icss->databaseUsername ),
                       ( unsigned char * )icss->databasePassword, strlen( icss->databasePassword ) );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllConnect: SQLConnect failed: %d", stat );
        rodsLog( LOG_ERROR,
                 "cllConnect: SQLConnect failed:odbcEntry=%s,user=%s,pass=XXXXX\n",
                 RDA_ODBC_ENTRY_NAME, icss->databaseUsername );
        SQLCHAR         sqlstate[SQL_SQLSTATE_SIZE + 1];
        SQLINTEGER      sqlcode;
        SQLCHAR         buffer[SQL_MAX_MESSAGE_LENGTH + 1];
        SQLSMALLINT     length;
        while ( SQLError( icss->environPtr, myHdbc , 0, sqlstate, &sqlcode, buffer,
                          SQL_MAX_MESSAGE_LENGTH + 1, &length ) == SQL_SUCCESS ) {
            rodsLog( LOG_ERROR, "cllConnect:          SQLSTATE: %s\n", sqlstate );
            rodsLog( LOG_ERROR, "cllConnect:  Native Error Code: %ld\n", sqlcode );
            rodsLog( LOG_ERROR, "cllConnect: %s \n", buffer );
        }

        SQLDisconnect( myHdbc );
        SQLFreeHandle( SQL_HANDLE_DBC, myHdbc );
        return -1;
    }

    icss->connectPtr = myHdbc;

    if ( icss->databaseType == DB_TYPE_MYSQL ) {
        /*
          MySQL must be running in ANSI mode (or at least in PIPES_AS_CONCAT
          mode) to be able to understand Postgres SQL. STRICT_TRANS_TABLES
          must be st too, otherwise inserting NULL into NOT NULL column does
          not produce error.
        */
        cllExecSqlNoResult( icss, "SET SESSION autocommit=0" ) ;
        cllExecSqlNoResult( icss,
                            "SET SESSION sql_mode='ANSI,STRICT_TRANS_TABLES'" );
    }
    return 0;
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
cllCheckPending( const char *sql, int option, int dbType ) {
    static int pendingCount = 0;
    static int pendingIx = 0;
    static int pendingAudits = 0;
    static char pBuffer[pBufferSize + 2];
    static int firstTime = 1;

    if ( firstTime ) {
        firstTime = 0;
        memset( pBuffer, 0, pBufferSize );
    }
    if ( option == 0 ) {
        if ( strncmp( sql, "commit", 6 ) == 0 ||
                strncmp( sql, "rollback", 8 ) == 0 ) {
            pendingIx = 0;
            pendingCount = 0;
            pendingAudits = 0;
            memset( pBuffer, 0, pBufferSize );
            return 0;
        }
        if ( pendingIx < maxPendingToRecord ) {
            strncpy( ( char * )&pBuffer[pendingIx * pendingRecordSize], sql,
                     pendingRecordSize - 1 );
            pendingIx++;
        }
        pendingCount++;
        return 0;
    }
    if ( option == 2 ) {
        pendingAudits++;
        return 0;
    }

    /* if there are some non-Audit pending SQL, log them */
    if ( pendingCount > pendingAudits ) {
        /* but ignore a single pending "begin" which can be normal */
        if ( pendingIx == 1 ) {
            if ( strncmp( ( char * )&pBuffer[0], "begin", 5 ) == 0 ) {
                return 0;
            }
        }
        if ( dbType == DB_TYPE_MYSQL ) {
            /* For mySQL, may have a few SET SESSION sql too, which we
               should ignore */
            int skip = 1;
            if ( strncmp( ( char * )&pBuffer[0], "begin", 5 ) != 0 ) {
                skip = 0;
            }
            int max = maxPendingToRecord;
            if ( pendingIx < max ) {
                max = pendingIx;
            }
            for ( int i = 1; i < max && skip == 1; i++ ) {
                if ( strncmp( ( char * )&pBuffer[i * pendingRecordSize],
                              "SET SESSION", 11 ) != 0 ) {
                    skip = 0;
                }
            }
            if ( skip ) {
                return 0;
            }
        }

        rodsLog( LOG_NOTICE, "Warning, pending SQL at cllDisconnect, count: %d",
                 pendingCount );
        int max = maxPendingToRecord;
        if ( pendingIx < max ) {
            max = pendingIx;
        }
        for ( int i = 0; i < max; i++ ) {
            rodsLog( LOG_NOTICE, "Warning, pending SQL: %s ...",
                     ( char * )&pBuffer[i * pendingRecordSize] );
        }
        if ( pendingAudits > 0 ) {
            rodsLog( LOG_NOTICE, "Warning, SQL will be commited with audits" );
        }
    }

    if ( pendingAudits > 0 ) {
        rodsLog( LOG_NOTICE,
                 "Notice, pending Auditing SQL committed at cllDisconnect" );
        return ( 1 ); /* tell caller (cllDisconect) to do a commit */
    }
    return 0;
}

/*
  Disconnect from the DBMS.
*/
int
cllDisconnect( icatSessionStruct *icss ) {

    int i = cllCheckPending( "", 1, icss->databaseType );
    if ( i == 1 ) {
        i = cllExecSqlNoResult( icss, "commit" ); /* auto commit any
                                                   pending SQLs, including
                                                   the Audit ones */
        /* Nothing to do if it fails */
    }

    SQLRETURN stat = SQLDisconnect( icss->connectPtr );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllDisconnect: SQLDisconnect failed: %d", stat );
        return -1;
    }

    stat = SQLFreeHandle( SQL_HANDLE_DBC, icss->connectPtr );
    if ( stat == SQL_SUCCESS ) {
        icss->connectPtr = NULL;
    }
    else {
        rodsLog( LOG_ERROR, "cllDisconnect: SQLFreeHandle failed for connect: %d", stat );
        return -2;
    }

    return 0;
}
/*
  Execute a SQL command which has no resulting table.  Examples include
  insert, delete, update, or ddl.
  Insert a 'begin' statement, if necessary.
*/
int
cllExecSqlNoResult( icatSessionStruct *icss, const char *sql ) {

    if ( strncmp( sql, "commit", 6 ) == 0 ||
            strncmp( sql, "rollback", 8 ) == 0 ) {
        didBegin = 0;
    }
    else {
        if ( didBegin == 0 ) {
            int status = _cllExecSqlNoResult( icss, "begin", 1 );
            if ( status != SQL_SUCCESS ) {
                return status;
            }
        }
        didBegin = 1;
    }
    return _cllExecSqlNoResult( icss, sql, 0 );
}

/*
  Log the bind variables from the global array (after an error)
*/
void
logTheBindVariables( int level ) {
    for ( int i = 0; i < cllBindVarCountPrev; i++ ) {
        char tmpStr[TMP_STR_LEN + 2];
        snprintf( tmpStr, TMP_STR_LEN, "bindVar[%d]=%s", i + 1, cllBindVars[i] );
        rodsLog( level, tmpStr );
    }
}

/*
  Bind variables from the global array.
*/
int
bindTheVariables( HSTMT myHstmt, const char *sql ) {

    int myBindVarCount = cllBindVarCount;
    cllBindVarCountPrev = cllBindVarCount; /* save in case we need to log error */
    cllBindVarCount = 0; /* reset for next call */

    for ( int i = 0; i < myBindVarCount; ++i ) {
        SQLRETURN stat = SQLBindParameter( myHstmt, i + 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                           SQL_CHAR, 0, 0, const_cast<char*>( cllBindVars[i] ), strlen( cllBindVars[i] ), const_cast<SQLLEN*>( &GLOBAL_SQL_NTS ) );
        char tmpStr[TMP_STR_LEN];
        snprintf( tmpStr, sizeof( tmpStr ), "bindVar[%d]=%s", i + 1, cllBindVars[i] );
        rodsLogSql( tmpStr );
        if ( stat != SQL_SUCCESS ) {
            rodsLog( LOG_ERROR,
                     "bindTheVariables: SQLBindParameter failed: %d", stat );
            return -1;
        }
    }

    return 0;
}

/*
  Case-insensitive string comparison, first string can be any case and
  contain leading and trailing spaces, second string must be lowercase,
  no spaces.
*/
static int cmp_stmt( const char *str1, const char *str2 ) {
    /* skip leading spaces */
    while ( isspace( *str1 ) ) {
        ++str1 ;
    }

    /* start comparing */
    for ( ; *str1 && *str2 ; ++str1, ++str2 ) {
        if ( tolower( *str1 ) != *str2 ) {
            return 0 ;
        }
    }

    /* skip trailing spaces */
    while ( isspace( *str1 ) ) {
        ++str1 ;
    }

    /* if we are at the end of the strings then they are equal */
    return *str1 == *str2 ;
}

/*
  Execute a SQL command which has no resulting table.  With optional
  bind variables.
  If option is 1, skip the bind variables.
*/
int
_cllExecSqlNoResult(
    icatSessionStruct* icss,
    const char*        sql,
    int                option ) {

    rodsLog( LOG_DEBUG1, sql );

    HDBC myHdbc = icss->connectPtr;
    HSTMT myHstmt;
    SQLRETURN stat = SQLAllocHandle( SQL_HANDLE_STMT, myHdbc, &myHstmt );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "_cllExecSqlNoResult: SQLAllocHandle failed for statement: %d", stat );
        return -1;
    }

    if ( option == 0 && bindTheVariables( myHstmt, sql ) != 0 ) {
        return -1;
    }

    rodsLogSql( sql );

    stat = SQLExecDirect( myHstmt, ( unsigned char * )sql, strlen( sql ) );
    SQL_INT_OR_LEN rowCount = 0;
    SQLRowCount( myHstmt, ( SQL_INT_OR_LEN * )&rowCount );
    switch ( stat ) {
    case SQL_SUCCESS:
        rodsLogSqlResult( "SUCCESS" );
        break;
    case SQL_SUCCESS_WITH_INFO:
        rodsLogSqlResult( "SUCCESS_WITH_INFO" );
        break;
    case SQL_NO_DATA_FOUND:
        rodsLogSqlResult( "NO_DATA" );
        break;
    case SQL_ERROR:
        rodsLogSqlResult( "SQL_ERROR" );
        break;
    case SQL_INVALID_HANDLE:
        rodsLogSqlResult( "HANDLE_ERROR" );
        break;
    default:
        rodsLogSqlResult( "UNKNOWN" );
    }

    int result;
    if ( stat == SQL_SUCCESS ||
            stat == SQL_SUCCESS_WITH_INFO ||
            stat == SQL_NO_DATA_FOUND ) {
        cllCheckPending( sql, 0, icss->databaseType );
        result = 0;
        if ( stat == SQL_NO_DATA_FOUND ) {
            result = CAT_SUCCESS_BUT_WITH_NO_INFO;
        }
        /* ODBC says that if statement is not UPDATE, INSERT, or DELETE then
           SQLRowCount may return anything. So for BEGIN, COMMIT and ROLLBACK
           we don't want to call it but just return OK.
        */
        if ( ! cmp_stmt( sql, "begin" )  &&
                ! cmp_stmt( sql, "commit" ) &&
                ! cmp_stmt( sql, "rollback" ) ) {
            /* Doesn't seem to return SQL_NO_DATA_FOUND, so check */
            rowCount = 0;
            if ( SQLRowCount( myHstmt, ( SQL_INT_OR_LEN * )&rowCount ) ) {
                /* error getting rowCount???, just call it no_info */
                result = CAT_SUCCESS_BUT_WITH_NO_INFO;
            }
            if ( rowCount == 0 ) {
                result = CAT_SUCCESS_BUT_WITH_NO_INFO;
            }
        }
    }
    else {
        if ( option == 0 ) {
            logTheBindVariables( LOG_NOTICE );
        }
        rodsLog( LOG_NOTICE, "_cllExecSqlNoResult: SQLExecDirect error: %d sql:%s",
                 stat, sql );
        result = logPsgError( LOG_NOTICE, icss->environPtr, myHdbc, myHstmt,
                              icss->databaseType );
    }

    stat = SQLFreeHandle( SQL_HANDLE_STMT, myHstmt );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "_cllExecSqlNoResult: SQLFreeHandle for statement error: %d", stat );
    }

    noResultRowCount = rowCount;

    return result;
}

/*
   Execute a SQL command that returns a result table, and
   and bind the default row.
   This version now uses the global array of bind variables.
*/
int
cllExecSqlWithResult( icatSessionStruct *icss, int *stmtNum, const char *sql ) {


    /* In 2.2 and some versions before, this would call
       _cllExecSqlNoResult with "begin", similar to how cllExecSqlNoResult
       does.  But since this function is called for 'select's, this is not
       needed here, and in fact causes postgres processes to be in the
       'idle in transaction' state which prevents some operations (such as
       backup).  So this was removed. */
    rodsLog( LOG_DEBUG1, sql );

    HDBC myHdbc = icss->connectPtr;
    HSTMT hstmt;
    SQLRETURN stat = SQLAllocHandle( SQL_HANDLE_STMT, myHdbc, &hstmt );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllExecSqlWithResult: SQLAllocHandle failed for statement: %d",
                 stat );
        return -1;
    }

    int statementNumber = -1;
    for ( int i = 0; i < MAX_NUM_OF_CONCURRENT_STMTS && statementNumber < 0; i++ ) {
        if ( icss->stmtPtr[i] == 0 ) {
            statementNumber = i;
        }
    }
    if ( statementNumber < 0 ) {
        rodsLog( LOG_ERROR,
                 "cllExecSqlWithResult: too many concurrent statements" );
        return -2;
    }

    icatStmtStrct * myStatement = ( icatStmtStrct * )malloc( sizeof( icatStmtStrct ) );
    icss->stmtPtr[statementNumber] = myStatement;

    myStatement->stmtPtr = hstmt;

    if ( bindTheVariables( hstmt, sql ) != 0 ) {
        return -1;
    }

    rodsLogSql( sql );
    stat = SQLExecDirect( hstmt, ( unsigned char * )sql, strlen( sql ) );

    switch ( stat ) {
    case SQL_SUCCESS:
        rodsLogSqlResult( "SUCCESS" );
        break;
    case SQL_SUCCESS_WITH_INFO:
        rodsLogSqlResult( "SUCCESS_WITH_INFO" );
        break;
    case SQL_NO_DATA_FOUND:
        rodsLogSqlResult( "NO_DATA" );
        break;
    case SQL_ERROR:
        rodsLogSqlResult( "SQL_ERROR" );
        break;
    case SQL_INVALID_HANDLE:
        rodsLogSqlResult( "HANDLE_ERROR" );
        break;
    default:
        rodsLogSqlResult( "UNKNOWN" );
    }

    if ( stat != SQL_SUCCESS &&
            stat != SQL_SUCCESS_WITH_INFO &&
            stat != SQL_NO_DATA_FOUND ) {
        logTheBindVariables( LOG_NOTICE );
        rodsLog( LOG_NOTICE,
                 "cllExecSqlWithResult: SQLExecDirect error: %d, sql:%s",
                 stat, sql );
        logPsgError( LOG_NOTICE, icss->environPtr, myHdbc, hstmt,
                     icss->databaseType );
        return -1;
    }

    SQLSMALLINT numColumns;
    stat = SQLNumResultCols( hstmt, &numColumns );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllExecSqlWithResult: SQLNumResultCols failed: %d",
                 stat );
        return -2;
    }
    myStatement->numOfCols = numColumns;
    for ( int i = 0; i < numColumns; i++ ) {
        SQLCHAR colName[MAX_TOKEN] = "";
        SQLSMALLINT colNameLen;
        SQLSMALLINT colType;
        SQL_UINT_OR_ULEN precision;
        SQLSMALLINT scale;
        stat = SQLDescribeCol( hstmt, i + 1, colName, sizeof( colName ),
                               &colNameLen, &colType, &precision, &scale, NULL );
        if ( stat != SQL_SUCCESS ) {
            rodsLog( LOG_ERROR, "cllExecSqlWithResult: SQLDescribeCol failed: %d",
                     stat );
            return -3;
        }
        /*  printf("colName='%s' precision=%d\n",colName, precision); */
        columnLength[i] = precision;
        SQL_INT_OR_LEN displaysize;
        stat = SQLColAttribute( hstmt, i + 1, SQL_COLUMN_DISPLAY_SIZE,
                                NULL, 0, NULL, &displaysize ); // JMC :: fixed for odbc
        if ( stat != SQL_SUCCESS ) {
            rodsLog( LOG_ERROR,
                     "cllExecSqlWithResult: SQLColAttributes failed: %d",
                     stat );
            return -3;
        }

        if ( displaysize > ( ( int )strlen( ( char * ) colName ) ) ) {
            columnLength[i] = displaysize + 1;
        }
        else {
            columnLength[i] = strlen( ( char * ) colName ) + 1;
        }
        /*      printf("columnLength[%d]=%d\n",i,columnLength[i]); */

        myStatement->resultValue[i] = ( char* )malloc( ( int )columnLength[i] );

        strcpy( ( char * )myStatement->resultValue[i], "" );

        // =-=-=-=-=-=-=-
        // JMC :: added static array to catch the result set size.  this was necessary to
        stat = SQLBindCol( hstmt, i + 1, SQL_C_CHAR, myStatement->resultValue[i], columnLength[i], &resultDataSizeArray[ i ] );
        if ( stat != SQL_SUCCESS ) {
            rodsLog( LOG_ERROR,
                     "cllExecSqlWithResult: SQLColAttributes failed: %d",
                     stat );
            return -4;
        }


        myStatement->resultColName[i] = ( char* )malloc( ( int )columnLength[i] );
        strncpy( myStatement->resultColName[i], ( char * )colName, columnLength[i] );

    }

    *stmtNum = statementNumber;
    return 0;
}

/* logBindVars
   For when an error occurs, log the bind variables which were used
   with the sql.
*/
void
logBindVars(
    int level,
    std::vector<std::string> &bindVars ) {
    for ( int i = 0; i < bindVars.size(); i++ ) {
        if ( !bindVars[i].empty() ) {
            rodsLog( level, "bindVar%d=%s", i + 1, bindVars[i].c_str() );
        }
    }
}


/*
   Execute a SQL command that returns a result table, and
   and bind the default row; and allow optional bind variables.
*/
int
cllExecSqlWithResultBV(
    icatSessionStruct *icss,
    int *stmtNum,
    const char *sql,
    std::vector< std::string > &bindVars ) {

    rodsLog( LOG_DEBUG1, sql );

    HDBC myHdbc = icss->connectPtr;
    HSTMT hstmt;
    SQLRETURN stat = SQLAllocHandle( SQL_HANDLE_STMT, myHdbc, &hstmt );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllExecSqlWithResultBV: SQLAllocHandle failed for statement: %d",
                 stat );
        return -1;
    }

    int statementNumber = -1;
    for ( int i = 0; i < MAX_NUM_OF_CONCURRENT_STMTS && statementNumber < 0; i++ ) {
        if ( icss->stmtPtr[i] == 0 ) {
            statementNumber = i;
        }
    }
    if ( statementNumber < 0 ) {
        rodsLog( LOG_ERROR,
                 "cllExecSqlWithResultBV: too many concurrent statements" );
        return -2;
    }

    icatStmtStrct * myStatement = ( icatStmtStrct * )malloc( sizeof( icatStmtStrct ) );
    icss->stmtPtr[statementNumber] = myStatement;

    myStatement->stmtPtr = hstmt;

    for ( int i = 0; i < bindVars.size(); i++ ) {
        if ( !bindVars[i].empty() ) {

            stat = SQLBindParameter( hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                     SQL_CHAR, 0, 0, const_cast<char*>( bindVars[i].c_str() ), bindVars[i].size(), const_cast<SQLLEN*>( &GLOBAL_SQL_NTS ) );
            char tmpStr[TMP_STR_LEN];
            snprintf( tmpStr, sizeof( tmpStr ), "bindVar%d=%s", i + 1, bindVars[i].c_str() );
            rodsLogSql( tmpStr );
            if ( stat != SQL_SUCCESS ) {
                rodsLog( LOG_ERROR,
                         "cllExecSqlWithResultBV: SQLBindParameter failed: %d", stat );
                return -1;
            }
        }
    }
    rodsLogSql( sql );
    stat = SQLExecDirect( hstmt, ( unsigned char * )sql, strlen( sql ) );

    switch ( stat ) {
    case SQL_SUCCESS:
        rodsLogSqlResult( "SUCCESS" );
        break;
    case SQL_SUCCESS_WITH_INFO:
        rodsLogSqlResult( "SUCCESS_WITH_INFO" );
        break;
    case SQL_NO_DATA_FOUND:
        rodsLogSqlResult( "NO_DATA" );
        break;
    case SQL_ERROR:
        rodsLogSqlResult( "SQL_ERROR" );
        break;
    case SQL_INVALID_HANDLE:
        rodsLogSqlResult( "HANDLE_ERROR" );
        break;
    default:
        rodsLogSqlResult( "UNKNOWN" );
    }

    if ( stat != SQL_SUCCESS &&
            stat != SQL_SUCCESS_WITH_INFO &&
            stat != SQL_NO_DATA_FOUND ) {
        logBindVars( LOG_NOTICE, bindVars );
        rodsLog( LOG_NOTICE,
                 "cllExecSqlWithResultBV: SQLExecDirect error: %d, sql:%s",
                 stat, sql );
        logPsgError( LOG_NOTICE, icss->environPtr, myHdbc, hstmt,
                     icss->databaseType );
        return -1;
    }

    SQLSMALLINT numColumns;
    stat = SQLNumResultCols( hstmt, &numColumns );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllExecSqlWithResultBV: SQLNumResultCols failed: %d",
                 stat );
        return -2;
    }
    myStatement->numOfCols = numColumns;

    for ( int i = 0; i < numColumns; i++ ) {
        SQLCHAR colName[MAX_TOKEN] = "";
        SQLSMALLINT colNameLen;
        SQLSMALLINT colType;
        SQL_UINT_OR_ULEN precision;
        SQLSMALLINT scale;
        stat = SQLDescribeCol( hstmt, i + 1, colName, sizeof( colName ),
                               &colNameLen, &colType, &precision, &scale, NULL );
        if ( stat != SQL_SUCCESS ) {
            rodsLog( LOG_ERROR, "cllExecSqlWithResultBV: SQLDescribeCol failed: %d",
                     stat );
            return -3;
        }
        /*  printf("colName='%s' precision=%d\n",colName, precision); */
        columnLength[i] = precision;
        SQL_INT_OR_LEN  displaysize;
        stat = SQLColAttribute( hstmt, i + 1, SQL_COLUMN_DISPLAY_SIZE,
                                NULL, 0, NULL, &displaysize ); // JMC :: changed to SQLColAttribute for odbc update
        if ( stat != SQL_SUCCESS ) {
            rodsLog( LOG_ERROR,
                     "cllExecSqlWithResultBV: SQLColAttributes failed: %d",
                     stat );
            return -3;
        }

        if ( displaysize > ( ( int )strlen( ( char * ) colName ) ) ) {
            columnLength[i] = displaysize + 1;
        }
        else {
            columnLength[i] = strlen( ( char * ) colName ) + 1;
        }
        /*      printf("columnLength[%d]=%d\n",i,columnLength[i]); */

        myStatement->resultValue[i] = ( char* )malloc( ( int )columnLength[i] );

        myStatement->resultValue[i][0] = '\0';
        // =-=-=-=-=-=-=-
        // JMC :: added static array to catch the result set size.  this was necessary to
        stat = SQLBindCol( hstmt, i + 1, SQL_C_CHAR, myStatement->resultValue[i], columnLength[i], &resultDataSizeArray[i] );

        if ( stat != SQL_SUCCESS ) {
            rodsLog( LOG_ERROR,
                     "cllExecSqlWithResultBV: SQLColAttributes failed: %d",
                     stat );
            return -4;
        }


        myStatement->resultColName[i] = ( char* )malloc( ( int )columnLength[i] );
        strncpy( myStatement->resultColName[i], ( char * )colName, columnLength[i] );

    }
    *stmtNum = statementNumber;
    return 0;
}

/*
  Return a row from a previous cllExecSqlWithResult call.
*/
int
cllGetRow( icatSessionStruct *icss, int statementNumber ) {
    icatStmtStrct *myStatement = icss->stmtPtr[statementNumber];

    for ( int i = 0; i < myStatement->numOfCols; i++ ) {
        strcpy( ( char * )myStatement->resultValue[i], "" );
    }
    SQLRETURN stat =  SQLFetch( myStatement->stmtPtr );
    if ( stat != SQL_SUCCESS && stat != SQL_NO_DATA_FOUND ) {
        rodsLog( LOG_ERROR, "cllGetRow: SQLFetch failed: %d", stat );
        return -1;
    }
    if ( stat == SQL_NO_DATA_FOUND ) {
        _cllFreeStatementColumns( icss, statementNumber );
        myStatement->numOfCols = 0;
    }
    return 0;
}

/*
   Return the string needed to get the next value in a sequence item.
   The syntax varies between RDBMSes, so it is here, in the DBMS-specific code.
*/
int
cllNextValueString( const char *itemName, char *outString, int maxSize ) {
#ifdef MY_ICAT
    snprintf( outString, maxSize, "%s_nextval()", itemName );
#else
    snprintf( outString, maxSize, "nextval('%s')", itemName );
#endif
    return 0;
}

int
cllGetRowCount( icatSessionStruct *icss, int statementNumber ) {

    if ( statementNumber < 0 ) {
        return noResultRowCount;
    }

    icatStmtStrct * myStatement = icss->stmtPtr[statementNumber];
    HSTMT hstmt = myStatement->stmtPtr;

    SQL_INT_OR_LEN RowCount;
    int i = SQLRowCount( hstmt, ( SQL_INT_OR_LEN * )&RowCount );
    if ( i ) {
        return i;
    }
    return RowCount;
}

int
cllCurrentValueString( const char *itemName, char *outString, int maxSize ) {
#ifdef MY_ICAT
    snprintf( outString, maxSize, "%s_currval()", itemName );
#else
    snprintf( outString, maxSize, "currval('%s')", itemName );
#endif
    return 0;
}

/*
   Free a statement (from a previous cllExecSqlWithResult call) and the
   corresponding resultValue array.
*/
int
cllFreeStatement( icatSessionStruct *icss, int statementNumber ) {

    icatStmtStrct * myStatement = icss->stmtPtr[statementNumber];
    if ( myStatement == NULL ) { /* already freed */
        return 0;
    }

    _cllFreeStatementColumns( icss, statementNumber );

    SQLRETURN stat = SQLFreeHandle( SQL_HANDLE_STMT, myStatement->stmtPtr );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllFreeStatement SQLFreeHandle for statement error: %d", stat );
    }

    free( myStatement );

    icss->stmtPtr[statementNumber] = NULL; /* indicate that the statement is free */

    return 0;
}

/*
   Free the statement columns (from a previous cllExecSqlWithResult call),
   but not the whole statement.
*/
int
_cllFreeStatementColumns( icatSessionStruct *icss, int statementNumber ) {

    icatStmtStrct * myStatement = icss->stmtPtr[statementNumber];

    for ( int i = 0; i < myStatement->numOfCols; i++ ) {
        free( myStatement->resultValue[i] );
        myStatement->resultValue[i] = NULL;
        free( myStatement->resultColName[i] );
        myStatement->resultColName[i] = NULL;
    }
    return 0;
}

/*
  A few tests to verify basic functionality (including talking with
  the database via ODBC).
*/
extern "C" int cllTest( const char *userArg, const char *pwArg ) {

    icatSessionStruct icss;
    icss.stmtPtr[0] = 0;
    strncpy( icss.database_plugin_type, "postgres", DB_TYPENAME_LEN );
    icss.databaseType = DB_TYPE_POSTGRES; // JMC - backport 4712
#ifdef MY_ICAT
    icss.databaseType = DB_TYPE_MYSQL;
    strncpy( icss.database_plugin_type, "mysql", DB_TYPENAME_LEN );
#endif

    rodsLogSqlReq( 1 );
    int OK = !cllOpenEnv( &icss );

    char userName[500];
    if ( userArg == 0 || *userArg == '\0' ) {
        struct passwd *ppasswd;
        ppasswd = getpwuid( getuid() );  // get user passwd entry
        snprintf( userName, sizeof( userName ), "%s", ppasswd->pw_name ); // get user name
    }
    else {
        snprintf( userName, sizeof( userName ), "%s", userArg );
    }
    printf( "userName=%s\n", userName );
    printf( "password=%s\n", pwArg );

    snprintf( icss.databaseUsername, DB_USERNAME_LEN, "%s", userName );

    if ( pwArg == 0 || *pwArg == '\0' ) {
        icss.databasePassword[0] = '\0';
    }
    else {
        snprintf( icss.databasePassword, DB_PASSWORD_LEN, "%s", pwArg );
    }

    int status = cllConnect( &icss );
    if ( status != 0 ) {
        printf( "cllConnect failed with error %d.\n", status );
        return status;
    }

    status = cllExecSqlNoResult( &icss, "create table test (i integer, j integer, a varchar(32))" );
    OK &= ( status == 0 || status == CAT_SUCCESS_BUT_WITH_NO_INFO );
    OK &= !cllExecSqlNoResult( &icss, "insert into test values (2, 3, 'a')" );
    OK &= !cllExecSqlNoResult( &icss, "commit" );
    OK &= !!cllExecSqlNoResult( &icss, "bad sql" ); /* should fail, if not it's not OK */
    OK &= !cllExecSqlNoResult( &icss, "rollback" ); /* close the bad transaction*/
    status = cllExecSqlNoResult( &icss, "delete from test where i = 1" );
    OK &= ( status == 0 || status == CAT_SUCCESS_BUT_WITH_NO_INFO );
    OK &= !cllExecSqlNoResult( &icss, "commit" );

    int stmt;
    std::vector<std::string> bindVars;
    bindVars.push_back( "a" );
    status = cllExecSqlWithResultBV( &icss, &stmt, "select * from test where a = ?", bindVars );
    OK &= !status;
    if ( status == 0 ) {
        int numOfCols = 1;
        for ( int i = 0; i < 10 && numOfCols > 0; i++ ) {
            status = cllGetRow( &icss, stmt );
            OK &= !status;
            if ( status == 0 ) {
                numOfCols = icss.stmtPtr[stmt]->numOfCols;
                if ( numOfCols == 0 ) {
                    printf( "No more rows returned\n" );
                    cllFreeStatement( &icss, stmt );
                }
                else {
                    for ( int j = 0; j < numOfCols || j < icss.stmtPtr[stmt]->numOfCols; j++ ) {
                        printf( "resultValue[%d]=%s\n", j,
                                icss.stmtPtr[stmt]->resultValue[j] );
                    }
                }
            }
        }
    }

    bindVars.clear();
    bindVars.push_back( "2" );
    status = cllExecSqlWithResultBV( &icss, &stmt,
                                     "select * from test where i = ?",
                                     bindVars );
    OK &= !status;
    if ( status == 0 ) {
        int numOfCols = 1;
        for ( int i = 0; i < 10 && numOfCols > 0; i++ ) {
            status = cllGetRow( &icss, stmt );
            OK &= !status;
            if ( status == 0 ) {
                numOfCols = icss.stmtPtr[stmt]->numOfCols;
                if ( numOfCols == 0 ) {
                    printf( "No more rows returned\n" );
                    cllFreeStatement( &icss, stmt );
                }
                else {
                    for ( int j = 0; j < numOfCols || j < icss.stmtPtr[stmt]->numOfCols; j++ ) {
                        printf( "resultValue[%d]=%s\n", j,
                                icss.stmtPtr[stmt]->resultValue[j] );
                    }
                }
            }
        }
    }

    status = cllExecSqlNoResult( &icss, "drop table test;" );
    OK &= ( status == 0 || status == CAT_SUCCESS_BUT_WITH_NO_INFO );
    OK &= !cllExecSqlNoResult( &icss, "commit" );
    OK &= !cllDisconnect( &icss );
    OK &= !cllCloseEnv( &icss );

    if ( OK ) {
        printf( "The tests all completed normally\n" );
        return 0;
    }
    else {
        printf( "One or more tests DID NOT complete normally\n" );
        return -1;
    }
}
