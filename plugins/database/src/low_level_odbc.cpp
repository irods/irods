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

#include "irods/private/low_level_odbc.hpp"

#include "irods/irods_log.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_server_properties.hpp"

#include <cctype>
#include <string>

namespace
{
    using log_db = irods::experimental::log::database;

    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_DB_TRACE(msg, ...) log_db::trace("{}@{}: " msg, __func__, __LINE__ __VA_OPT__(,) __VA_ARGS__);
} // anonymous namespace

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

#ifndef ORA_ICAT
static int didBegin = 0;
#endif
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
                IRODS_DB_TRACE("Setting errorVal to CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME.");
                errorVal = CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME;
            }
        } else if (dbType == DB_TYPE_POSTGRES ) {
            if ( strcmp( ( char * )sqlstate, "23505" ) == 0 &&
                    strstr( ( char * )psgErrorMsg, "duplicate key" ) ) {
                IRODS_DB_TRACE("Setting errorVal to CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME.");
                errorVal = CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME;
            }
        } else if ( dbType == DB_TYPE_ORACLE ) { 
            if ( strcmp( ( char * )sqlstate, "23000" ) == 0 &&
                    strstr( ( char * )psgErrorMsg, "unique constraint" ) ) {
                IRODS_DB_TRACE("Setting errorVal to CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME.");
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
    // ODBC Entry is defined as "iRODS Catalog" or an env variable
    char odbcEntryName[ DB_TYPENAME_LEN ];
    char* odbc_env = getenv( "irodsOdbcDSN" );
    if ( odbc_env ) {
        rodsLog( LOG_DEBUG, "Setting ODBC entry to ENV [%s]", odbc_env );
        snprintf( odbcEntryName, sizeof( odbcEntryName ), "%s", odbc_env );
    }
    else {
        snprintf( odbcEntryName, sizeof( odbcEntryName ), "iRODS Catalog" );
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
  This function is used to check that there are no DB-modifying SQLs pending
  before a disconnect.  If there are, it logs a warning.

  If option is 0, record some of the sql, or clear it (if commit or rollback).
  If option is 1, issue warning the there are some pending (and include
  some of the sql).
*/
#define maxPendingToRecord 5
#define pendingRecordSize 30
#define pBufferSize (maxPendingToRecord*pendingRecordSize)
int
cllCheckPending( const char *sql, int option, int dbType ) {
    static int pendingCount = 0;
    static int pendingIx = 0;
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

    if (pendingCount > 0) {
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
                if (    (strncmp((char *)&pBuffer[i*pendingRecordSize], "SET SESSION",   11) !=0)
                     && (strncmp((char *)&pBuffer[i*pendingRecordSize], "SET character", 13) !=0)) {
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
    }

    return 0;
}

/*
  Disconnect from the DBMS.
*/
int
cllDisconnect( icatSessionStruct *icss ) {

    if ( 1 == cllCheckPending( "", 1, icss->databaseType ) ) {
        /* auto commit any pending SQLs */
        /* Nothing to do if it fails */
        cllExecSqlNoResult( icss, "commit" ); 
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

#ifndef ORA_ICAT
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
#endif
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
        rodsLog( level, "%s", tmpStr );
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
    rodsLog( LOG_DEBUG10, "%s", sql );

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
    auto ec = SQLRowCount(myHstmt, (SQL_INT_OR_LEN*) &rowCount); // NOLINT(google-readability-casting)
    IRODS_DB_TRACE("SQLRowCount returned [{}] and set [rowCount] to [{}].", ec, rowCount);
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
            IRODS_DB_TRACE("Setting result to CAT_SUCCESS_BUT_WITH_NO_INFO.");
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
                IRODS_DB_TRACE("Setting result to CAT_SUCCESS_BUT_WITH_NO_INFO.");
                result = CAT_SUCCESS_BUT_WITH_NO_INFO;
            }
            if ( rowCount == 0 ) {
                IRODS_DB_TRACE("Setting result to CAT_SUCCESS_BUT_WITH_NO_INFO.");
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
    rodsLog( LOG_DEBUG10, "%s", sql );

    HDBC myHdbc = icss->connectPtr;
    HSTMT hstmt;
    SQLRETURN stat = SQLAllocHandle( SQL_HANDLE_STMT, myHdbc, &hstmt );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllExecSqlWithResult: SQLAllocHandle failed for statement: %d",
                 stat );
        return -1;
    }

    // Issue 3862:  Set stmtNum to -1 and in cllFreeStatement if the stmtNum is negative do nothing
    *stmtNum = UNINITIALIZED_STATEMENT_NUMBER;

    int statementNumber = UNINITIALIZED_STATEMENT_NUMBER;
    for ( int i = 0; i < MAX_NUM_OF_CONCURRENT_STMTS && statementNumber < 0; i++ ) {
        if ( icss->stmtPtr[i] == 0 ) {
            statementNumber = i;
        }
    }
    if ( statementNumber < 0 ) {
        rodsLog( LOG_ERROR,
                 "cllExecSqlWithResult: too many concurrent statements" );
        return CAT_STATEMENT_TABLE_FULL;
    }

    icatStmtStrct * myStatement = ( icatStmtStrct * )malloc( sizeof( icatStmtStrct ) );
    memset( myStatement, 0, sizeof( icatStmtStrct ) );

    icss->stmtPtr[statementNumber] = myStatement;
    *stmtNum = statementNumber;

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
        memset( myStatement->resultValue[i], 0, (int)columnLength[i] );

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
        memset( myStatement->resultColName[i], 0, (int)columnLength[i] );

#ifdef ORA_ICAT
        //oracle prints column names (which are case-insensitive) in upper case,
        //so to remain consistent with postgres and mysql, we convert them to lower case.
        for ( int j = 0; j < columnLength[i] && colName[j] != '\0'; j++ ) {
            colName[j] = tolower( colName[j] );
        }
#endif
        strncpy( myStatement->resultColName[i], ( char * )colName, columnLength[i] );

    }

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
    for ( std::size_t i = 0; i < bindVars.size(); i++ ) {
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

    rodsLog( LOG_DEBUG10, "%s", sql );

    HDBC myHdbc = icss->connectPtr;
    HSTMT hstmt;
    SQLRETURN stat = SQLAllocHandle( SQL_HANDLE_STMT, myHdbc, &hstmt );
    if ( stat != SQL_SUCCESS ) {
        rodsLog( LOG_ERROR, "cllExecSqlWithResultBV: SQLAllocHandle failed for statement: %d",
                 stat );
        return -1;
    }

    // Issue 3862:  Set stmtNum to -1 and in cllFreeStatement if the stmtNum is negative do nothing
    *stmtNum = UNINITIALIZED_STATEMENT_NUMBER;

    int statementNumber = UNINITIALIZED_STATEMENT_NUMBER;
    for ( int i = 0; i < MAX_NUM_OF_CONCURRENT_STMTS && statementNumber < 0; i++ ) {
        if ( icss->stmtPtr[i] == 0 ) {
            statementNumber = i;
        }
    }
    if ( statementNumber < 0 ) {
        rodsLog( LOG_ERROR,
                 "cllExecSqlWithResultBV: too many concurrent statements" );
        return CAT_STATEMENT_TABLE_FULL;
    }

    icatStmtStrct * myStatement = ( icatStmtStrct * )malloc( sizeof( icatStmtStrct ) );
    memset( myStatement, 0, sizeof( icatStmtStrct ) );
    icss->stmtPtr[statementNumber] = myStatement;

    *stmtNum = statementNumber;

    myStatement->stmtPtr = hstmt;

    for ( std::size_t i = 0; i < bindVars.size(); i++ ) {
        if ( !bindVars[i].empty() ) {

            stat = SQLBindParameter( hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                     SQL_CHAR, 0, 0, const_cast<char*>( bindVars[i].c_str() ), bindVars[i].size(), const_cast<SQLLEN*>( &GLOBAL_SQL_NTS ) );
            char tmpStr[TMP_STR_LEN];
            snprintf( tmpStr, sizeof( tmpStr ), "bindVar%ju=%s", static_cast<uintmax_t>(i + 1), bindVars[i].c_str() );
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
        memset( myStatement->resultValue[i], 0, (int)columnLength[i] );

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
        memset( myStatement->resultColName[i], 0, (int)columnLength[i] );
#ifdef ORA_ICAT
        //oracle prints column names (which are case-insensitive) in upper case,
        //so to remain consistent with postgres and mysql, we convert them to lower case.
        for ( int j = 0; j < columnLength[i] && colName[j] != '\0'; j++ ) {
            colName[j] = tolower( colName[j] );
        }
#endif
        strncpy( myStatement->resultColName[i], ( char * )colName, columnLength[i] );

    }

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
#ifdef ORA_ICAT
    snprintf( outString, maxSize, "%s.nextval", itemName );
#elif MY_ICAT
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
#ifdef ORA_ICAT
    snprintf( outString, maxSize, "%s.currval", itemName );
#elif MY_ICAT
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
cllFreeStatement( icatSessionStruct *icss, int& statementNumber ) {

    // Issue 3862 - Statement number is set to negative until it is 
    // created.  When the statement is freed it is again set to negative.
    // Do not free when statementNumber is negative.  If this is called twice 
    // by a client, after the first call the statementNumber will be negative
    // and nothing will be done.
    if (statementNumber < 0) {
        return 0;
    }

    icatStmtStrct * myStatement = icss->stmtPtr[statementNumber];
    if ( myStatement == NULL ) { /* already freed */
        statementNumber = UNINITIALIZED_STATEMENT_NUMBER;
        return 0;
    }

    _cllFreeStatementColumns( icss, statementNumber );

    SQLRETURN stat = SQLFreeHandle( SQL_HANDLE_STMT, myStatement->stmtPtr );
    if ( stat != SQL_SUCCESS ) {
        statementNumber = UNINITIALIZED_STATEMENT_NUMBER;
        rodsLog( LOG_ERROR, "cllFreeStatement SQLFreeHandle for statement error: %d", stat );
    }

    free( myStatement );
    icss->stmtPtr[statementNumber] = NULL; /* indicate that the statement is free */
    statementNumber = UNINITIALIZED_STATEMENT_NUMBER;

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
