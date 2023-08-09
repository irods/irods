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

#include "irods/private/mid_level.hpp"
#include "irods/private/low_level.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_virtual_path.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/rcMisc.h"

#include <vector>
#include <string>

#include <boost/filesystem.hpp>
#include <fmt/format.h>

/* Size of the R_OBJT_AUDIT comment field;must match table column definition */
#define AUDIT_COMMENT_MAX_SIZE       1000

extern int logSQL_CML;

int checkObjIdByTicket( const char *dataId, const char *accessLevel,
                        const char *ticketStr, const char *ticketHost,
                        const char *userName, const char *userZone,
                        icatSessionStruct *icss );

/*
  Convert the input arrays to a string and add bind variables
*/
char *cmlArraysToStrWithBind( char*         str,
                              const char*   preStr,
                              const char*   arr[],
                              const char*   arr2[],
                              int           arrLen,
                              const char*   sep,
                              const char*   sep2,
                              int           maxLen ) {
    int i;

    rstrcpy( str, preStr, maxLen );

    for ( i = 0; i < arrLen; i++ ) {
        if ( i > 0 ) {
            rstrcat( str, sep2, maxLen );
        }
        rstrcat( str, arr[i], maxLen );
        rstrcat( str, sep, maxLen );
        rstrcat( str, "?", maxLen );
        cllBindVars[cllBindVarCount++] = arr2[i];
    }

    return str;

}

int cmlDebug( int mode ) {
    logSQL_CML = mode;
    return 0;
}

int cmlOpen( icatSessionStruct *icss ) {
    int i;

    /* Initialize the icss statement pointers */
    for ( i = 0; i < MAX_NUM_OF_CONCURRENT_STMTS; i++ ) {
        icss->stmtPtr[i] = 0;
    }

    /*
     Set the ICAT DBMS type.  The Low Level now uses this instead of the
     ifdefs so it can interact with either at the same time.
     */
    icss->databaseType = DB_TYPE_POSTGRES;
#ifdef ORA_ICAT
    icss->databaseType = DB_TYPE_ORACLE;
#endif
#ifdef MY_ICAT
    icss->databaseType = DB_TYPE_MYSQL;
#endif


    /* Open Environment */
    i = cllOpenEnv( icss );
    if ( i != 0 ) {
        return CAT_ENV_ERR;
    }

    /* Connect to the DBMS */
    i = cllConnect( icss );
    if ( i != 0 ) {
        return CAT_CONNECT_ERR;
    }

    return 0;
}

int cmlClose( icatSessionStruct *icss ) {
    int status, stat2;
    static int pending = 0;

    if ( pending == 1 ) {
        return ( 0 );   /* avoid hang if stuck doing this */
    }
    pending = 1;

    status = cllDisconnect( icss );

    stat2 = cllCloseEnv( icss );

    pending = 0;
    if ( status ) {
        return CAT_DISCONNECT_ERR;
    }
    if ( stat2 ) {
        return CAT_CLOSE_ENV_ERR;
    }
    return 0;
}


int cmlExecuteNoAnswerSql( const char *sql,
                           icatSessionStruct *icss ) {
    int i;
    i = cllExecSqlNoResult( icss, sql );
    if ( i ) {
        if ( i <= CAT_ENV_ERR ) {
            return ( i );   /* already an iRODS error code */
        }
        return CAT_SQL_ERR;
    }
    return 0;

}

int cmlGetOneRowFromSqlBV( const char *sql,
                           char *cVal[],
                           int cValSize[],
                           int numOfCols,
                           std::vector<std::string> &bindVars,
                           icatSessionStruct *icss ) {
    int stmtNum = UNINITIALIZED_STATEMENT_NUMBER;
    char updatedSql[MAX_SQL_SIZE + 1];

//TODO: this should be a function, probably inside low-level icat
#ifdef ORA_ICAT
    strncpy( updatedSql, sql, MAX_SQL_SIZE );
    updatedSql[MAX_SQL_SIZE] = '\0';
#else
    strncpy( updatedSql, sql, MAX_SQL_SIZE );
    updatedSql[MAX_SQL_SIZE] = '\0';
    /* Verify there no limit or offset statement */
    if ( ( strstr( updatedSql, "limit " ) == NULL ) && ( strstr( updatedSql, "offset " ) == NULL ) ) {
        /* add 'limit 1' for performance improvement */
        strncat( updatedSql, " limit 1", MAX_SQL_SIZE );
        rodsLog( LOG_DEBUG10, "cmlGetOneRowFromSqlBV %s", updatedSql );
    }
#endif
    int status = cllExecSqlWithResultBV( icss, &stmtNum, updatedSql,
                                         bindVars );
    if ( status != 0 ) {
        cllFreeStatement(icss, stmtNum);
        if ( status <= CAT_ENV_ERR ) {
            return status;    /* already an iRODS error code */
        }
        return CAT_SQL_ERR;
    }

    if ( cllGetRow( icss, stmtNum ) != 0 )  {
        cllFreeStatement( icss, stmtNum );
        return CAT_GET_ROW_ERR;
    }
    if ( icss->stmtPtr[stmtNum]->numOfCols == 0 ) {
        cllFreeStatement( icss, stmtNum );
        return CAT_NO_ROWS_FOUND;
    }
    int numCVal = std::min( numOfCols, icss->stmtPtr[stmtNum]->numOfCols );
    for ( int j = 0; j < numCVal ; j++ ) {
        rstrcpy( cVal[j], icss->stmtPtr[stmtNum]->resultValue[j], cValSize[j] );
    }

    cllFreeStatement( icss, stmtNum );
    return numCVal;

}

int cmlGetOneRowFromSql( const char *sql,
                         char *cVal[],
                         int cValSize[],
                         int numOfCols,
                         icatSessionStruct *icss ) {
    int i, j, stmtNum = UNINITIALIZED_STATEMENT_NUMBER;
    char updatedSql[MAX_SQL_SIZE + 1];

//TODO: this should be a function, probably inside low-level icat
#ifdef ORA_ICAT
    strncpy( updatedSql, sql, MAX_SQL_SIZE );
    updatedSql[MAX_SQL_SIZE] = '\0';
#else
    strncpy( updatedSql, sql, MAX_SQL_SIZE );
    updatedSql[MAX_SQL_SIZE] = '\0';
    /* Verify there no limit or offset statement */
    if ( ( strstr( updatedSql, "limit " ) == NULL ) && ( strstr( updatedSql, "offset " ) == NULL ) ) {
        /* add 'limit 1' for performance improvement */
        strncat( updatedSql, " limit 1", MAX_SQL_SIZE );
        rodsLog( LOG_DEBUG10, "cmlGetOneRowFromSql %s", updatedSql );
    }
#endif

    std::vector<std::string> emptyBindVars;
    i = cllExecSqlWithResultBV( icss, &stmtNum, updatedSql,
                                emptyBindVars );
    if ( i != 0 ) {
        cllFreeStatement( icss, stmtNum );
        if ( i <= CAT_ENV_ERR ) {
            return ( i );   /* already an iRODS error code */
        }
        return CAT_SQL_ERR;
    }
    i = cllGetRow( icss, stmtNum );
    if ( i != 0 )  {
        cllFreeStatement( icss, stmtNum );
        return CAT_GET_ROW_ERR;
    }
    if ( icss->stmtPtr[stmtNum]->numOfCols == 0 ) {
        cllFreeStatement( icss, stmtNum );
        return CAT_NO_ROWS_FOUND;
    }
    for ( j = 0; j < numOfCols && j < icss->stmtPtr[stmtNum]->numOfCols ; j++ ) {
        rstrcpy( cVal[j], icss->stmtPtr[stmtNum]->resultValue[j], cValSize[j] );
    }

    cllFreeStatement( icss, stmtNum );
    return j;

}

/* like cmlGetOneRowFromSql but cVal uses space from query
   and then caller frees it later (via cmlFreeStatement).
   This is simpler for the caller, in some cases.   */
int cmlGetOneRowFromSqlV2( const char *sql,
                           char *cVal[],
                           int maxCols,
                           std::vector<std::string> &bindVars,
                           icatSessionStruct *icss ) {
    int i, j, stmtNum = UNINITIALIZED_STATEMENT_NUMBER;
    char updatedSql[MAX_SQL_SIZE + 1];

//TODO: this should be a function, probably inside low-level icat
#ifdef ORA_ICAT
    strncpy( updatedSql, sql, MAX_SQL_SIZE );
    updatedSql[MAX_SQL_SIZE] = '\0';
#else
    strncpy( updatedSql, sql, MAX_SQL_SIZE );
    updatedSql[MAX_SQL_SIZE] = '\0';
    /* Verify there no limit or offset statement */
    if ( ( strstr( updatedSql, "limit " ) == NULL ) && ( strstr( updatedSql, "offset " ) == NULL ) ) {
        /* add 'limit 1' for performance improvement */
        strncat( updatedSql, " limit 1", MAX_SQL_SIZE );
        rodsLog( LOG_DEBUG10, "cmlGetOneRowFromSqlV2 %s", updatedSql );
    }
#endif

    i = cllExecSqlWithResultBV( icss, &stmtNum, updatedSql,
                                bindVars );

    if ( i != 0 ) {
        cllFreeStatement( icss, stmtNum ); 
        if ( i <= CAT_ENV_ERR ) {
            return ( i );   /* already an iRODS error code */
        }
        return CAT_SQL_ERR;
    }
    i = cllGetRow( icss, stmtNum );
    if ( i != 0 )  {
        cllFreeStatement( icss, stmtNum );
        return CAT_GET_ROW_ERR;
    }
    if ( icss->stmtPtr[stmtNum]->numOfCols == 0 ) {
        return CAT_NO_ROWS_FOUND;
    }
    for ( j = 0; j < maxCols && j < icss->stmtPtr[stmtNum]->numOfCols ; j++ ) {
        cVal[j] = icss->stmtPtr[stmtNum]->resultValue[j];
    }

    return ( stmtNum ); /* 0 or positive is the statement number */
}

/*
 Like cmlGetOneRowFromSql but uses bind variable array (via
    cllExecSqlWithResult).
 */
static
int cmlGetOneRowFromSqlV3( const char *sql,
                           char *cVal[],
                           int cValSize[],
                           int numOfCols,
                           icatSessionStruct *icss ) {
    int i, j, stmtNum = UNINITIALIZED_STATEMENT_NUMBER;
    char updatedSql[MAX_SQL_SIZE + 1];

//TODO: this should be a function, probably inside low-level icat
#ifdef ORA_ICAT
    strncpy( updatedSql, sql, MAX_SQL_SIZE );
    updatedSql[MAX_SQL_SIZE] = '\0';
#else
    strncpy( updatedSql, sql, MAX_SQL_SIZE );
    updatedSql[MAX_SQL_SIZE] = '\0';
    /* Verify there no limit or offset statement */
    if ( ( strstr( updatedSql, "limit " ) == NULL ) && ( strstr( updatedSql, "offset " ) == NULL ) ) {
        /* add 'limit 1' for performance improvement */
        strncat( updatedSql, " limit 1", MAX_SQL_SIZE );
        rodsLog( LOG_DEBUG10, "cmlGetOneRowFromSqlV3 %s", updatedSql );
    }
#endif

    i = cllExecSqlWithResult( icss, &stmtNum, updatedSql );

    if ( i != 0 ) {
        cllFreeStatement( icss, stmtNum );
        if ( i <= CAT_ENV_ERR ) {
            return ( i );   /* already an iRODS error code */
        }
        return CAT_SQL_ERR;
    }
    i = cllGetRow( icss, stmtNum );
    if ( i != 0 )  {
        cllFreeStatement( icss, stmtNum );
        return CAT_GET_ROW_ERR;
    }
    if ( icss->stmtPtr[stmtNum]->numOfCols == 0 ) {
        cllFreeStatement( icss, stmtNum );
        return CAT_NO_ROWS_FOUND;
    }
    for ( j = 0; j < numOfCols && j < icss->stmtPtr[stmtNum]->numOfCols ; j++ ) {
        rstrcpy( cVal[j], icss->stmtPtr[stmtNum]->resultValue[j], cValSize[j] );
    }

    cllFreeStatement( icss, stmtNum );
    return j;

}


int cmlFreeStatement( int statementNumber, icatSessionStruct *icss ) {
    int i;
    i = cllFreeStatement( icss, statementNumber );
    return i;
}

/*
 * caller must free on success */
int cmlGetFirstRowFromSql( const char *sql,
                           int *statement,
                           int skipCount,
                           icatSessionStruct *icss ) {

    int i = cllExecSqlWithResult( icss, statement, sql );

    if ( i != 0 ) {
        cllFreeStatement( icss, *statement );
        *statement = UNINITIALIZED_STATEMENT_NUMBER;
        if ( i <= CAT_ENV_ERR ) {
            return ( i );   /* already an iRODS error code */
        }
        return CAT_SQL_ERR;
    }

#ifdef ORA_ICAT
    if ( skipCount > 0 ) {
        for ( int j = 0; j < skipCount; j++ ) {
            i = cllGetRow( icss, *statement );
            if ( i != 0 )  {
                cllFreeStatement( icss, *statement );
                *statement = UNINITIALIZED_STATEMENT_NUMBER;
                return CAT_GET_ROW_ERR;
            }
            if ( icss->stmtPtr[*statement]->numOfCols == 0 ) {
                cllFreeStatement( icss, *statement );
                *statement = UNINITIALIZED_STATEMENT_NUMBER;
                return CAT_NO_ROWS_FOUND;
            }
        }
    }
#endif

    i = cllGetRow( icss, *statement );
    if ( i != 0 )  {
        cllFreeStatement( icss, *statement );
        *statement = UNINITIALIZED_STATEMENT_NUMBER;
        return CAT_GET_ROW_ERR;
    }
    if ( icss->stmtPtr[*statement]->numOfCols == 0 ) {
        cllFreeStatement( icss, *statement );
        *statement = UNINITIALIZED_STATEMENT_NUMBER;
        return CAT_NO_ROWS_FOUND;
    }

    return 0;
}

/* with bind-variables 
 * caller must free on success */
int cmlGetFirstRowFromSqlBV( const char *sql,
                             std::vector<std::string> &bindVars,
                             int *statement,
                             icatSessionStruct *icss ) {
    if ( int status = cllExecSqlWithResultBV( icss, statement, sql, bindVars ) ) {
        *statement = UNINITIALIZED_STATEMENT_NUMBER;
        if ( status <= CAT_ENV_ERR ) {
            return status;    /* already an iRODS error code */
        }
        return CAT_SQL_ERR;
    }
    if ( cllGetRow( icss, *statement ) ) {
        cllFreeStatement( icss, *statement );
        *statement = UNINITIALIZED_STATEMENT_NUMBER;
        return CAT_GET_ROW_ERR;
    }
    if ( icss->stmtPtr[*statement]->numOfCols == 0 ) {
        cllFreeStatement( icss, *statement );
        *statement = UNINITIALIZED_STATEMENT_NUMBER;
        return CAT_NO_ROWS_FOUND;
    }
    return 0;
}

/*
 * caller must free on success */
int cmlGetNextRowFromStatement( int stmtNum,
                                icatSessionStruct *icss ) {

    if ( 0 != cllGetRow( icss, stmtNum ) )  {
        cllFreeStatement( icss, stmtNum );
        return CAT_GET_ROW_ERR;
    }
    if ( icss->stmtPtr[stmtNum]->numOfCols == 0 ) {
        cllFreeStatement( icss, stmtNum );
        return CAT_NO_ROWS_FOUND;
    }
    return 0;
}

int cmlGetStringValueFromSql( const char *sql,
                              char *cVal,
                              int cValSize,
                              std::vector<std::string> &bindVars,
                              icatSessionStruct *icss ) {
    int status;
    char *cVals[2];
    int iVals[2];

    cVals[0] = cVal;
    iVals[0] = cValSize;

    status = cmlGetOneRowFromSqlBV( sql, cVals, iVals, 1,
                                    bindVars, icss );
    if ( status == 1 ) {
        return 0;
    }
    else {
        return status;
    }

}

int cmlGetStringValuesFromSql( const char *sql,
                               char *cVal[],
                               int cValSize[],
                               int numberOfStringsToGet,
                               std::vector<std::string> &bindVars,
                               icatSessionStruct *icss ) {

    int i = cmlGetOneRowFromSqlBV( sql, cVal, cValSize, numberOfStringsToGet,
                                   bindVars, icss );
    if ( i == numberOfStringsToGet ) {
        return 0;
    }
    else {
        return i;
    }

}

int cmlGetMultiRowStringValuesFromSql( const char *sql,
                                       char *returnedStrings,
                                       int maxStringLen,
                                       int maxNumberOfStringsToGet,
                                       std::vector<std::string> &bindVars,
                                       icatSessionStruct *icss ) {

    int i, j, stmtNum = UNINITIALIZED_STATEMENT_NUMBER;
    int tsg; /* total strings gotten */
    char *pString;

    if ( maxNumberOfStringsToGet <= 0 ) {
        return CAT_INVALID_ARGUMENT;
    }

    i = cllExecSqlWithResultBV( icss, &stmtNum, sql, bindVars );
    if ( i != 0 ) {
        cllFreeStatement( icss, stmtNum );
        if ( i <= CAT_ENV_ERR ) {
            return ( i );   /* already an iRODS error code */
        }
        return CAT_SQL_ERR;
    }
    tsg = 0;
    pString = returnedStrings;
    for ( ;; ) {
        i = cllGetRow( icss, stmtNum );
        if ( i != 0 )  {
            cllFreeStatement( icss, stmtNum );
            if ( tsg > 0 ) {
                return tsg;
            }
            return CAT_GET_ROW_ERR;
        }
        if ( icss->stmtPtr[stmtNum]->numOfCols == 0 ) {
            cllFreeStatement( icss, stmtNum );
            if ( tsg > 0 ) {
                return tsg;
            }
            return CAT_NO_ROWS_FOUND;
        }
        for ( j = 0; j < icss->stmtPtr[stmtNum]->numOfCols; j++ ) {
            rstrcpy( pString, icss->stmtPtr[stmtNum]->resultValue[j],
                     maxStringLen );
            tsg++;
            pString += maxStringLen;
            if ( tsg >= maxNumberOfStringsToGet ) {
                cllFreeStatement( icss, stmtNum );
                return tsg;
            }
        }
    }
    cllFreeStatement( icss, stmtNum );
    return 0;
}


int cmlGetIntegerValueFromSql( const char *sql,
                               rodsLong_t *iVal,
                               std::vector<std::string> &bindVars,
                               icatSessionStruct *icss ) {
    int i, cValSize;
    char *cVal[2];
    char cValStr[MAX_INTEGER_SIZE + 10];

    cVal[0] = cValStr;
    cValSize = MAX_INTEGER_SIZE;

    i = cmlGetOneRowFromSqlBV( sql, cVal, &cValSize, 1,
                               bindVars, icss );
    if ( i == 1 ) {
        if ( *cVal[0] == '\0' ) {
            return CAT_NO_ROWS_FOUND;
        }
        *iVal = strtoll( *cVal, NULL, 0 );
        return 0;
    }
    return i;
}

/* Like cmlGetIntegerValueFromSql but uses bind-variable array */
int cmlGetIntegerValueFromSqlV3( const char *sql,
                                 rodsLong_t *iVal,
                                 icatSessionStruct *icss ) {
    int i, cValSize;
    char *cVal[2];
    char cValStr[MAX_INTEGER_SIZE + 10];

    cVal[0] = cValStr;
    cValSize = MAX_INTEGER_SIZE;

    i = cmlGetOneRowFromSqlV3( sql, cVal, &cValSize, 1, icss );
    if ( i == 1 ) {
        if ( *cVal[0] == '\0' ) {
            return CAT_NO_ROWS_FOUND;
        }
        *iVal = strtoll( *cVal, NULL, 0 );
        return 0;
    }
    return i;
}

int cmlCheckNameToken( const char *nameSpace, const char *tokenName, icatSessionStruct *icss ) {

    rodsLong_t iVal;
    int status;

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckNameToken SQL 1 " );
    }
    std::vector<std::string> bindVars;
    bindVars.push_back( nameSpace );
    bindVars.push_back( tokenName );
    status = cmlGetIntegerValueFromSql(
                 "select token_id from  R_TOKN_MAIN where token_namespace=? and token_name=?",
                 &iVal, bindVars, icss );
    return status;

}

int cmlModifySingleTable( const char *tableName,
                          const char *updateCols[],
                          const char *updateValues[],
                          const char *whereColsAndConds[],
                          const char *whereValues[],
                          int numOfUpdates,
                          int numOfConds,
                          icatSessionStruct *icss ) {
    char tsql[MAX_SQL_SIZE];
    int i, l;
    char *rsql;

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlModifySingleTable SQL 1 " );
    }

    snprintf( tsql, MAX_SQL_SIZE, "update %s set ", tableName );
    l = strlen( tsql );
    rsql = tsql + l;

    cmlArraysToStrWithBind( rsql, "", updateCols, updateValues, numOfUpdates, " = ", ", ", MAX_SQL_SIZE - l );
    l = strlen( tsql );
    rsql = tsql + l;

    cmlArraysToStrWithBind( rsql, " where ", whereColsAndConds, whereValues, numOfConds, "", " and ", MAX_SQL_SIZE - l );

    i = cmlExecuteNoAnswerSql( tsql, icss );
    return i;

}

#define STR_LEN 100
rodsLong_t
cmlGetNextSeqVal( icatSessionStruct *icss ) {
    char nextStr[STR_LEN];
    char sql[STR_LEN];
    int status;
    rodsLong_t iVal{};

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlGetNextSeqVal SQL 1 " );
    }

    nextStr[0] = '\0';

    cllNextValueString( "R_ObjectID", nextStr, STR_LEN );
    /* R_ObjectID is created in icatSysTables.sql as
       the sequence item for objects */

#ifdef ORA_ICAT
    /* For Oracle, use the built-in one-row table */
    snprintf( sql, STR_LEN, "select %s from DUAL", nextStr );
#else
    /* Postgres can just get the next value without a table */
    snprintf( sql, STR_LEN, "select %s", nextStr );
#endif

    std::vector<std::string> emptyBindVars;
    status = cmlGetIntegerValueFromSql( sql, &iVal, emptyBindVars, icss );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "cmlGetNextSeqVal cmlGetIntegerValueFromSql failure %d", status );
        return status;
    }
    return iVal;
}

rodsLong_t
cmlGetCurrentSeqVal( icatSessionStruct *icss ) {
    char nextStr[STR_LEN];
    char sql[STR_LEN];
    int status;
    rodsLong_t iVal;

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlGetCurrentSeqVal S-Q-L 1 " );
    }

    nextStr[0] = '\0';

    cllCurrentValueString( "R_ObjectID", nextStr, STR_LEN );
    /* R_ObjectID is created in icatSysTables.sql as
       the sequence item for objects */

#ifdef ORA_ICAT
    /* For Oracle, use the built-in one-row table */
    snprintf( sql, STR_LEN, "select %s from DUAL", nextStr );
#else
    /* Postgres can just get the next value without a table */
    snprintf( sql, STR_LEN, "select %s", nextStr );
#endif

    std::vector<std::string> emptyBindVars;
    status = cmlGetIntegerValueFromSql( sql, &iVal, emptyBindVars, icss );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "cmlGetCurrentSeqVal cmlGetIntegerValueFromSql failure %d",
                 status );
        return status;
    }
    return iVal;
}

int
cmlGetNextSeqStr( char *seqStr, int maxSeqStrLen, icatSessionStruct *icss ) {
    char nextStr[STR_LEN];
    char sql[STR_LEN];
    int status;

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlGetNextSeqStr SQL 1 " );
    }

    nextStr[0] = '\0';
    cllNextValueString( "R_ObjectID", nextStr, STR_LEN );
    /* R_ObjectID is created in icatSysTables.sql as
       the sequence item for objects */

#ifdef ORA_ICAT
    snprintf( sql, STR_LEN, "select %s from DUAL", nextStr );
#else
    snprintf( sql, STR_LEN, "select %s", nextStr );
#endif

    std::vector<std::string> emptyBindVars;
    status = cmlGetStringValueFromSql( sql, seqStr, maxSeqStrLen, emptyBindVars, icss );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "cmlGetNextSeqStr cmlGetStringValueFromSql failure %d", status );
    }
    return status;
}

/* modified for various tests */
int cmlTest( icatSessionStruct *icss ) {
    int i, cValSize;
    char *cVal[2];
    char cValStr[MAX_INTEGER_SIZE + 10];
    char sql[100];

    strncpy( icss->databaseUsername, "schroede", DB_USERNAME_LEN );
    strncpy( icss->databasePassword, "", DB_PASSWORD_LEN );
    i = cmlOpen( icss );
    if ( i != 0 ) {
        return i;
    }

    cVal[0] = cValStr;
    cValSize = MAX_INTEGER_SIZE;
    snprintf( sql, sizeof sql,
              "select coll_id from R_COLL_MAIN where coll_name='a'" );

    i = cmlGetOneRowFromSql( sql, cVal, &cValSize, 1, icss );
    if ( i == 1 ) {
        printf( "result = %s\n", cValStr );
    }
    else {
        return i;
    }

    snprintf( sql, sizeof sql,
              "select data_id from R_DATA_MAIN where coll_id='1' and data_name='a'" );
    i = cmlGetOneRowFromSql( sql, cVal, &cValSize, 1, icss );
    if ( i == 1 ) {
        printf( "result = %s\n", cValStr );
        i = 0;
    }

    cmlGetCurrentSeqVal( icss );

    return i;

}

/*
  Check that a resource exists and user has 'accessLevel' permission.
  Return code is either an iRODS error code (< 0) or the collectionId.
*/
rodsLong_t
cmlCheckResc( const char *rescName, const char *userName, const char *userZone, const char *accessLevel,
              icatSessionStruct *icss ) {
    int status;
    rodsLong_t iVal{};

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckResc SQL 1 " );
    }

    std::vector<std::string> bindVars;
    bindVars.push_back( rescName );
    bindVars.push_back( userName );
    bindVars.push_back( userZone );
    bindVars.push_back( accessLevel );
    status = cmlGetIntegerValueFromSql(
                 "select resc_id from R_RESC_MAIN RM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where RM.resc_name=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = RM.resc_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
                 &iVal, bindVars, icss );
    if ( status ) {
        /* There was an error, so do another sql to see which
           of the two likely cases is problem. */

        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "cmlCheckResc SQL 2 " );
        }

        bindVars.clear();
        bindVars.push_back( rescName );
        status = cmlGetIntegerValueFromSql(
                     "select resc_id from R_RESC_MAIN where resc_name=?",
                     &iVal, bindVars, icss );
        if ( status ) {
            return CAT_UNKNOWN_RESOURCE;
        }
        return CAT_NO_ACCESS_PERMISSION;
    }

    return iVal;

}


// Check that a collection exists and user has 'accessLevel' permission.
// Return code is either an iRODS error code (< 0) or the collectionId.
//
// If admin_mode is true, only check that the collection exists.
rodsLong_t
cmlCheckDir(const char *dirName,
            const char *userName,
            const char *userZone,
            const char *accessLevel,
            icatSessionStruct *icss,
            bool admin_mode)
{
    int status;
    rodsLong_t iVal{};

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckDir SQL 1 " );
    }

    std::vector<std::string> bindVars;

    if (admin_mode) {
        bindVars.push_back(dirName);
        status = cmlGetIntegerValueFromSql("select coll_id from R_COLL_MAIN where coll_name = ?",
                                           &iVal, bindVars, icss);

        // No need to leave this code block. Either the collection exists or it doesn't.
        return (status < 0) ? CAT_UNKNOWN_COLLECTION : iVal;
    }

    bindVars.push_back( dirName );
    bindVars.push_back( userName );
    bindVars.push_back( userZone );
    bindVars.push_back( accessLevel );
    status = cmlGetIntegerValueFromSql(
         "select coll_id "
         "from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM "
         "where CM.coll_name = ? and "
               "UM.user_name = ? and "
               "UM.zone_name = ? and "
               "UM.user_type_name != 'rodsgroup' and "
               "UM.user_id = UG.user_id and "
               "OA.object_id = CM.coll_id and "
               "UG.group_user_id = OA.user_id and "
               "OA.access_type_id >= TM.token_id and "
               "TM.token_namespace ='access_type' and "
               "TM.token_name = ?",
         &iVal, bindVars, icss);

    if ( status ) {
        // There was an error, so do another sql to see which
        // of the two likely cases is the problem.

        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "cmlCheckDir SQL 2 " );
        }

        bindVars.clear();
        bindVars.push_back( dirName );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where coll_name=?",
                     &iVal, bindVars, icss );

        if ( status ) {
            return CAT_UNKNOWN_COLLECTION;
        }

        return CAT_NO_ACCESS_PERMISSION;
    }

    return iVal;
}


/*
  Check that a collection exists and user has 'accessLevel' permission.
  Return code is either an iRODS error code (< 0) or the collectionId.
  While at it, get the inheritance flag.
*/
rodsLong_t
cmlCheckDirAndGetInheritFlag( const char *dirName, const char *userName, const char *userZone,
                              const char *accessLevel, int *inheritFlag,
                              const char *ticketStr, const char *ticketHost,
                              icatSessionStruct *icss ) {
    int status;
    rodsLong_t iVal = 0;

    int cValSize[2];
    char *cVal[3];
    char cValStr1[MAX_INTEGER_SIZE + 10];
    char cValStr2[MAX_INTEGER_SIZE + 10];

    cVal[0] = cValStr1;
    cVal[1] = cValStr2;
    cValSize[0] = MAX_INTEGER_SIZE;
    cValSize[1] = MAX_INTEGER_SIZE;

    *inheritFlag = 0;

    if ( ticketStr != NULL && *ticketStr != '\0' ) {
        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "cmlCheckDirAndGetInheritFlag SQL 1 " );
        }
        std::vector<std::string> bindVars;
        bindVars.push_back( dirName );
        bindVars.push_back( ticketStr );
        status = cmlGetOneRowFromSqlBV( "select coll_id, coll_inheritance from R_COLL_MAIN CM, R_TICKET_MAIN TM where CM.coll_name=? and TM.ticket_string=? and TM.ticket_type = 'write' and TM.object_id = CM.coll_id",
                                        cVal, cValSize, 2, bindVars, icss );
    }
    else {
        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "cmlCheckDirAndGetInheritFlag SQL 2 " );
        }
        std::vector<std::string> bindVars;
        bindVars.push_back( dirName );
        bindVars.push_back( userName );
        bindVars.push_back( userZone );
        bindVars.push_back( accessLevel );
        status = cmlGetOneRowFromSqlBV( "select coll_id, coll_inheritance from R_COLL_MAIN CM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where CM.coll_name=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = CM.coll_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
                                        cVal, cValSize, 2, bindVars, icss );
    }
    if ( status == 2 ) {
        if ( *cVal[0] == '\0' ) {
            return CAT_NO_ROWS_FOUND;
        }
        iVal = strtoll( *cVal, NULL, 0 );
        if ( cValStr2[0] == '1' ) {
            *inheritFlag = 1;
        }
        status = 0;
    }

    if ( status ) {
        /* There was an error, so do another sql to see which
           of the two likely cases is problem. */

        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "cmlCheckDirAndGetInheritFlag SQL 3 " );
        }

        std::vector<std::string> bindVars;
        bindVars.push_back( dirName );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where coll_name=?",
                     &iVal, bindVars, icss );
        if ( status ) {
            return CAT_UNKNOWN_COLLECTION;
        }
        return CAT_NO_ACCESS_PERMISSION;
    }

    /*
     Also check the other aspects ticket at this point.
     */
    if ( ticketStr != NULL && *ticketStr != '\0' ) {
        status = checkObjIdByTicket( cValStr1, accessLevel, ticketStr,
                                     ticketHost, userName, userZone,
                                     icss );
        if ( status != 0 ) {
            return status;
        }
    }

    return iVal;
} // cmlCheckDirAndGetInheritFlag

/*
  Check that a collection exists and user has 'accessLevel' permission.
  Return code is either an iRODS error code (< 0) or the collectionId.
*/
rodsLong_t
cmlCheckDirId( const char *dirId, const char *userName, const char *userZone,
               const char *accessLevel, icatSessionStruct *icss ) {
    int status;
    rodsLong_t iVal;

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckDirId S-Q-L 1 " );
    }

    std::vector<std::string> bindVars;
    bindVars.push_back( userName );
    bindVars.push_back( userZone );
    bindVars.push_back( dirId );
    bindVars.push_back( accessLevel );
    status = cmlGetIntegerValueFromSql(
                 "select object_id from R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = ? and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
                 &iVal, bindVars, icss );
    if ( status ) {
        /* There was an error, so do another sql to see which
           of the two likely cases is problem. */

        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "cmlCheckDirId S-Q-L 2 " );
        }

        std::vector<std::string> bindVars;
        bindVars.push_back( dirId );
        status = cmlGetIntegerValueFromSql(
                     "select coll_id from R_COLL_MAIN where coll_id=?",
                     &iVal, bindVars, icss );
        if ( status ) {
            return CAT_UNKNOWN_COLLECTION;
        }
        return CAT_NO_ACCESS_PERMISSION;
    }

    return 0;
}

/*
  Check that a collection exists and user owns it
*/
rodsLong_t
cmlCheckDirOwn( const char *dirName, const char *userName, const char *userZone,
                icatSessionStruct *icss ) {
    int status;
    rodsLong_t iVal{};

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckDirOwn SQL 1 " );
    }

    std::vector<std::string> bindVars;
    bindVars.push_back( dirName );
    bindVars.push_back( userName );
    bindVars.push_back( userZone );
    status = cmlGetIntegerValueFromSql(
                 "select coll_id from R_COLL_MAIN where coll_name=? and coll_owner_name=? and coll_owner_zone=?",
                 &iVal, bindVars, icss );
    if ( status < 0 ) {
        return status;
    }
    return iVal;
}


// Check that a dataObj (iRODS file) exists and user has specified permission
// (but don't check the collection access, only its existence).
// Return code is either an iRODS error code (< 0) or the dataId.
//
// If admin_mode is true, only check that the data object and collection exist.
rodsLong_t
cmlCheckDataObjOnly(const char *dirName, const char *dataName,
                    const char *userName, const char *userZone,
                    const char *accessLevel, icatSessionStruct *icss,
                    bool admin_mode)
{
    int status;
    rodsLong_t iVal{};

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckDataObjOnly SQL 1 " );
    }

    std::vector<std::string> bindVars;

    if (admin_mode) {
        bindVars.push_back(dataName);
        bindVars.push_back(dirName);

        status = cmlGetIntegerValueFromSql(
            "select data_id from R_DATA_MAIN DM "
            "inner join R_COLL_MAIN CM on DM.coll_id = CM.coll_id "
            "where DM.data_name = ? and CM.coll_name = ?",
            &iVal, bindVars, icss);

        // No need to leave this code block. Either the data object and collection
        // exist or they don't.
        return (status < 0) ? CAT_UNKNOWN_FILE : iVal;
    }

    bindVars.push_back( dataName );
    bindVars.push_back( dirName );
    bindVars.push_back( userName );
    bindVars.push_back( userZone );
    bindVars.push_back( accessLevel );
    status = cmlGetIntegerValueFromSql(
        "select data_id "
        "from R_DATA_MAIN DM, R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM, R_COLL_MAIN CM "
        "where DM.data_name = ? and "
              "DM.coll_id = CM.coll_id and "
              "CM.coll_name = ? and "
              "UM.user_name = ? and "
              "UM.zone_name = ? and "
              "UM.user_type_name != 'rodsgroup' and "
              "UM.user_id = UG.user_id and "
              "OA.object_id = DM.data_id and "
              "UG.group_user_id = OA.user_id and "
              "OA.access_type_id >= TM.token_id and "
              "TM.token_namespace = 'access_type' and "
              "TM.token_name = ?",
        &iVal, bindVars, icss);

    if ( status ) {
        /* There was an error, so do another sql to see which
           of the two likely cases is problem. */
        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "cmlCheckDataObjOnly SQL 2 " );
        }

        bindVars.clear();
        bindVars.push_back( dataName );
        bindVars.push_back( dirName );
        status = cmlGetIntegerValueFromSql(
            "select data_id from R_DATA_MAIN DM, R_COLL_MAIN CM "
            "where DM.data_name=? and DM.coll_id=CM.coll_id and CM.coll_name=?",
            &iVal, bindVars, icss );

        if ( status ) {
            return CAT_UNKNOWN_FILE;
        }

        return CAT_NO_ACCESS_PERMISSION;
    }

    return iVal;
}

/*
  Check that a dataObj (iRODS file) exists and user owns it
*/
rodsLong_t
cmlCheckDataObjOwn( const char *dirName, const char *dataName, const char *userName,
                    const char *userZone, icatSessionStruct *icss ) {
    int status;
    rodsLong_t iVal, collId;
    char collIdStr[MAX_NAME_LEN];

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckDataObjOwn SQL 1 " );
    }
    std::vector<std::string> bindVars;
    bindVars.push_back( dirName );
    status = cmlGetIntegerValueFromSql(
                 "select coll_id from R_COLL_MAIN where coll_name=?",
                 &iVal, bindVars, icss );
    if ( status < 0 ) {
        return status;
    }
    collId = iVal;
    snprintf( collIdStr, MAX_NAME_LEN, "%lld", collId );

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckDataObjOwn SQL 2 " );
    }
    bindVars.clear();
    bindVars.push_back( dataName );
    bindVars.push_back( collIdStr );
    bindVars.push_back( userName );
    bindVars.push_back( userZone );
    status = cmlGetIntegerValueFromSql(
                 "select data_id from R_DATA_MAIN where data_name=? and coll_id=? and data_owner_name=? and data_owner_zone=?",
                 &iVal, bindVars, icss );

    if ( status ) {
        return status;
    }
    return iVal;
}


int cmlCheckUserInGroup( const char *userName, const char *userZone,
                         const char *groupName, icatSessionStruct *icss ) {
    int status;
    char sVal[MAX_NAME_LEN];
    rodsLong_t iVal;

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckUserInGroup SQL 1 " );
    }

    std::vector<std::string> bindVars;
    bindVars.push_back( userName );
    bindVars.push_back( userZone );
    status = cmlGetStringValueFromSql(
                 "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name!='rodsgroup'",
                 sVal, MAX_NAME_LEN, bindVars, icss );
    if ( status == CAT_NO_ROWS_FOUND ) {
        return CAT_INVALID_USER;
    }
    if ( status ) {
        return status;
    }

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckUserInGroup SQL 2 " );
    }

    bindVars.clear();
    bindVars.push_back( sVal );
    bindVars.push_back( groupName );
    status = cmlGetIntegerValueFromSql(
                 "select group_user_id from R_USER_GROUP where user_id=? and group_user_id = (select user_id from R_USER_MAIN where user_type_name='rodsgroup' and user_name=?)",
                 &iVal, bindVars, icss );
    if ( status ) {
        return status;
    }
    return 0;
}

/* check on additional restrictions on a ticket, return error if not
 * allowed */
int
cmlCheckTicketRestrictions( const char *ticketId, const char *ticketHost,
                            const char *userName, const char *userZone,
                            icatSessionStruct *icss ) {
    int status;
    int stmtNum = UNINITIALIZED_STATEMENT_NUMBER;
    int hostOK = 0;
    int userOK = 0;
    int groupOK = 0;

    /* first, check if there are any host restrictions, and if so
       return error if the connected client host is not in the list */
    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckTicketRestrictions SQL 1" );
    }
    std::vector<std::string> bindVars;
    bindVars.push_back( ticketId );
    status = cmlGetFirstRowFromSqlBV(
                 "select host from R_TICKET_ALLOWED_HOSTS where ticket_id=?",
                 bindVars, &stmtNum, icss );
    if ( status == CAT_NO_ROWS_FOUND ) {
        hostOK = 1;
    }
    else {
        if ( status != 0 ) {
            cllFreeStatement(icss, stmtNum);
            return status;
        }
    }

    for ( ; status != CAT_NO_ROWS_FOUND; ) {
        if ( strncmp( ticketHost,
                      icss->stmtPtr[stmtNum]->resultValue[0],
                      NAME_LEN ) == 0 ) {
            hostOK = 1;
        }
        status = cmlGetNextRowFromStatement( stmtNum, icss );
        if ( status != 0 && status != CAT_NO_ROWS_FOUND ) {
            cllFreeStatement(icss, stmtNum);
            return status;
        }
    }
    if ( hostOK == 0 ) {
        cllFreeStatement(icss, stmtNum);
        return CAT_TICKET_HOST_EXCLUDED;
    }
    cllFreeStatement(icss, stmtNum);

    /* Now check on user restrictions */
    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckTicketRestrictions SQL 2" );
    }
    bindVars.clear();
    bindVars.push_back( ticketId );
    status = cmlGetFirstRowFromSqlBV(
                 "select user_name from R_TICKET_ALLOWED_USERS where ticket_id=?",
                 bindVars,  &stmtNum, icss );
    if ( status == CAT_NO_ROWS_FOUND ) {
        cllFreeStatement(icss, stmtNum);
        userOK = 1;
    }
    else {
        if ( status != 0 ) {
            cllFreeStatement(icss, stmtNum);
            return status;
        }
    }
    std::string myUser( userName );
    myUser += "#";
    myUser += userZone;
    for ( ; status != CAT_NO_ROWS_FOUND; ) {
        if ( strncmp( userName,
                      icss->stmtPtr[stmtNum]->resultValue[0],
                      NAME_LEN ) == 0 ) {
            userOK = 1;
        }
        else {
            /* try user#zone */
            if ( strncmp( myUser.c_str(),
                          icss->stmtPtr[stmtNum]->resultValue[0],
                          NAME_LEN ) == 0 ) {
                userOK = 1;
            }
        }
        status = cmlGetNextRowFromStatement( stmtNum, icss );
        if ( status != 0 && status != CAT_NO_ROWS_FOUND ) {
            cllFreeStatement(icss, stmtNum);
            return status;
        }
    }
    if ( userOK == 0 ) {
        cllFreeStatement(icss, stmtNum);
        return CAT_TICKET_USER_EXCLUDED;
    }
    cllFreeStatement(icss, stmtNum);

    /* Now check on group restrictions */
    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckTicketRestrictions SQL 3" );
    }
    bindVars.clear();
    bindVars.push_back( ticketId );
    status = cmlGetFirstRowFromSqlBV(
                 "select group_name from R_TICKET_ALLOWED_GROUPS where ticket_id=?",
                 bindVars, &stmtNum, icss );
    if ( status == CAT_NO_ROWS_FOUND ) {
        groupOK = 1;
    }
    else {
        if ( status != 0 ) {
            cllFreeStatement(icss, stmtNum);
            return status;
        }
    }
    for ( ; status != CAT_NO_ROWS_FOUND; ) {
        int status2;
        status2 = cmlCheckUserInGroup( userName, userZone,
                                       icss->stmtPtr[stmtNum]->resultValue[0],
                                       icss );
        if ( status2 == 0 ) {
            groupOK = 1;
        }
        status = cmlGetNextRowFromStatement( stmtNum, icss );
        if ( status != 0 && status != CAT_NO_ROWS_FOUND ) {
            cllFreeStatement(icss, stmtNum);
            return status;
        }
    }
    if ( groupOK == 0 ) {
        cllFreeStatement(icss, stmtNum);
        return CAT_TICKET_GROUP_EXCLUDED;
    }
    cllFreeStatement(icss, stmtNum);
    return 0;
}

/* Check access via a Ticket to a data-object or collection */
int checkObjIdByTicket(const char* dataId,
                       const char* accessLevel,
                       const char* ticketStr,
                       const char* ticketHost,
                       const char* userName,
                       const char* userZone,
                       icatSessionStruct* icss)
{
    // Get the collection name of the object which has a id matching dataId.
    // The object can be a collection or data object.
    char original_collection_name[MAX_NAME_LEN];
    std::vector<std::string> bindVars;
    // "dataId" will match the ID of a collection or data object.
    // See the prepared statement to understand why "dataId" is passed twice.
    bindVars.emplace_back(dataId);
    bindVars.emplace_back(dataId);
    int cml_error = cmlGetStringValueFromSql(
        "select coll_name from R_COLL_MAIN "
        "where coll_id = ? or coll_id in (select coll_id from R_DATA_MAIN where data_id = ?)",
        original_collection_name, // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        MAX_NAME_LEN,
        bindVars,
        icss);
    if(0 != cml_error) {
        rodsLog(
            LOG_ERROR,
            "failed to determine collection name for object id [%s]",
            dataId);
        return cml_error;
    }

    int status, i;
    char *cVal[10];
    int iVal[10];
    char ticketId[NAME_LEN] = "";
    char usesCount[NAME_LEN] = "";
    char usesLimit[NAME_LEN] = "";
    char ticketExpiry[NAME_LEN] = "";
    char restrictions[NAME_LEN] = "";
    char writeFileCount[NAME_LEN] = "";
    char writeFileLimit[NAME_LEN] = "";
    char writeByteCount[NAME_LEN] = "";
    char writeByteLimit[NAME_LEN] = "";
    int iUsesCount = 0;
    int iUsesLimit = 0;
    int iWriteFileCount = 0;
    int iWriteFileLimit = 0;
    int iWriteByteCount = 0;
    int iWriteByteLimit = 0;
    char myUsesCount[NAME_LEN];
    char myWriteFileCount[NAME_LEN];
    rodsLong_t intDataId;
    static rodsLong_t previousDataId1 = 0;
    static rodsLong_t previousDataId2 = 0;

    static char prevTicketId[50] = "";

    for ( i = 0; i < 10; i++ ) {
        iVal[i] = NAME_LEN;
    }

    cVal[0] = ticketId;
    cVal[1] = usesLimit;
    cVal[2] = usesCount;
    cVal[3] = ticketExpiry;
    cVal[4] = restrictions;

    // accessLevel is a misleading parameter name.
    //
    // It primarily acts as a boolean which when true (i.e. is equal to "modify"), causes the write-file
    // count and write-file limit for a ticket to be fetched from the database. Assuming the ticket can be
    // used for writing.
    //
    // This code path is for when a client is attempting to write to a data object via a ticket and
    // ticket stats must be checked/updated.
    if ( strncmp( accessLevel, "modify", 6 ) == 0 ) {
        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "checkObjIdByTicket SQL 1 " );
        }

        cVal[5] = writeFileCount;
        cVal[6] = writeFileLimit;
        cVal[7] = writeByteCount;
        cVal[8] = writeByteLimit;
        const auto zone_path = fmt::format("/{}", userZone);
        boost::filesystem::path coll_path{original_collection_name};

        // Get the current write-file count and write-file limit for the ticket from the database.
        // Loop until we've reached the zone collection path.
        while (coll_path != zone_path) {
            std::vector<std::string> bindVars;
            bindVars.emplace_back(ticketStr);
            bindVars.emplace_back(dataId);
            bindVars.push_back(coll_path.string());
            status = cmlGetStringValuesFromSql(
                "select ticket_id, uses_limit, uses_count, ticket_expiry_ts, restrictions, write_file_count, "
                "write_file_limit, write_byte_count, write_byte_limit from R_TICKET_MAIN where ticket_type = 'write' "
                "and ticket_string = ? and (object_id = ? or object_id in (select coll_id from R_COLL_MAIN where "
                "coll_name = ?))",
                cVal,
                iVal,
                9, // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
                bindVars,
                icss);
            if (0 == status) {
                break;
            }

            // If we've reached this point, that means the query didn't find anything.
            // In this case, get the parent path and see if the ticket is associated with a parent collection.
            coll_path = coll_path.parent_path();
        }
    }
    else {
        // If we're in this branch, that means the client isn't attempting to modify any
        // data object or collection. Essentially, the client is performing a read or checking
        // the existence of a collection or data object.

        // Don't check ticket type, "read" or "write" is fine.
        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "checkObjIdByTicket SQL 2 " );
        }

        const auto zone_path = fmt::format("/{}", userZone);
        boost::filesystem::path coll_path{original_collection_name};

        // Get the current use count and limit for the ticket from the database.
        // Loop until we've reached the zone collection path.
        while (coll_path != zone_path) {
            std::vector<std::string> bindVars;
            bindVars.emplace_back(ticketStr);
            bindVars.emplace_back(dataId);
            bindVars.push_back(coll_path.string());
            status =
                cmlGetStringValuesFromSql("select ticket_id, uses_limit, uses_count, ticket_expiry_ts, restrictions "
                                          "from R_TICKET_MAIN where ticket_string = ? and (object_id = ? or object_id "
                                          "in (select coll_id from R_COLL_MAIN where coll_name = ?))",
                                          cVal,
                                          iVal,
                                          5, // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
                                          bindVars,
                                          icss);
            if (0 == status) {
                break;
            }

            // If we've reached this point, that means the query didn't find anything.
            // In this case, get the parent path and see if the ticket is associated with a parent collection.
            coll_path = coll_path.parent_path();
        }
    }

    if ( status != 0 ) {
        return CAT_TICKET_INVALID;
    }

    if ( strncmp( ticketId, prevTicketId, sizeof( prevTicketId ) ) != 0 ) {
        snprintf( prevTicketId, sizeof( prevTicketId ), "%s", ticketId );
    }

    if ( ticketExpiry[0] != '\0' ) {
        rodsLong_t ticketExp, now;
        char myTime[50];

        ticketExp = atoll( ticketExpiry );
        if ( ticketExp > 0 ) {
            getNowStr( myTime );
            now = atoll( myTime );
            if ( now > ticketExp ) {
                return CAT_TICKET_EXPIRED;
            }
        }
    }

    status = cmlCheckTicketRestrictions(ticketId, ticketHost, userName, userZone, icss);
    if ( status != 0 ) {
        return status;
    }

    if ( strncmp( accessLevel, "modify", 6 ) == 0 ) {
        iWriteByteLimit = atoi( writeByteLimit );
        if ( iWriteByteLimit > 0 ) {
            iWriteByteCount = atoi( writeByteCount );
            if ( iWriteByteCount > iWriteByteLimit ) {
                return CAT_TICKET_WRITE_BYTES_EXCEEDED;
            }
        }

        iWriteFileLimit = atoi( writeFileLimit );
        if ( iWriteFileLimit > 0 ) {
            iWriteFileCount = atoi( writeFileCount );
            if ( iWriteFileCount > iWriteFileLimit ) {
                return CAT_TICKET_WRITE_USES_EXCEEDED;
            }

            intDataId = atoll( dataId );

            // Don't update a second time if this id matches the last one.
            // Given this function can be called multiple times within the same operation, checking to
            // see if the data id is different keeps the server from updating the ticket information multiple times.
            if ( previousDataId1 != intDataId ) {
                iWriteFileCount++;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                snprintf(myWriteFileCount, sizeof(myWriteFileCount), "%d", iWriteFileCount);
                cllBindVars[cllBindVarCount++] = myWriteFileCount;
                cllBindVars[cllBindVarCount++] = ticketId;
                if ( logSQL_CML != 0 ) {
                    rodsLog( LOG_SQL, "checkObjIdByTicket SQL 3 " );
                }
                status = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set write_file_count=? where ticket_id=?", icss);
                // Notice we don't call commit or rollback after executing the update.
                // The reason for this is that this function is allowed to be called within a transaction.
                // The caller of this function is expected to handle committing or rolling back.
                if ( status != 0 ) {
                    return status;
                }

#ifndef ORA_ICAT
                cllCheckPending( "", 2, icss->databaseType );
#endif
            }

            previousDataId1 = intDataId;
        }
    }

    iUsesLimit = atoi( usesLimit );
    if ( iUsesLimit > 0 ) {
        iUsesCount = atoi( usesCount );
        if ( iUsesCount > iUsesLimit ) {
            return CAT_TICKET_USES_EXCEEDED;
        }

        intDataId = atoll( dataId );

        // Don't update a second time if this id matches the last one.
        // Given this function can be called multiple times within the same operation, checking to
        // see if the data id is different keeps the server from updating the ticket information multiple times.
        if ( previousDataId2 != intDataId ) {
            iUsesCount++;
            snprintf( myUsesCount, sizeof myUsesCount, "%d", iUsesCount );
            cllBindVars[cllBindVarCount++] = myUsesCount;
            cllBindVars[cllBindVarCount++] = ticketId;
            if ( logSQL_CML != 0 ) {
                rodsLog( LOG_SQL, "checkObjIdByTicket SQL 4 " );
            }
            status = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set uses_count=? where ticket_id=?", icss);
            // Notice we don't call commit or rollback after executing the update.
            // The reason for this is that this function is allowed to be called within a transaction.
            // The caller of this function is expected to handle committing or rolling back.
            if ( status != 0 ) {
                return status;
            }

#ifndef ORA_ICAT
            cllCheckPending( "", 2, icss->databaseType );
#endif
        }

        previousDataId2 = intDataId;
    }

    return 0;
} // checkObjIdByTicket

int cmlTicketUpdateWriteBytes(const char* ticketStr,
                              const char* dataSize,
                              const char* objectId,
                              icatSessionStruct* icss)
{
    // Get the collection name of the data object which has a data_id matching objectId.
    char original_collection_name[MAX_NAME_LEN];
    std::vector<std::string> bindVars;
    bindVars.push_back( objectId );
    int cml_error = cmlGetStringValueFromSql(
                 "select coll_name from R_COLL_MAIN where coll_id in (select coll_id from R_DATA_MAIN where data_id=?)",
                 original_collection_name, MAX_NAME_LEN, bindVars, icss );
    if(0 != cml_error) {
        rodsLog(
            LOG_ERROR,
            "failed to determine collection name for object id [%s]",
            objectId);
        return cml_error;
    }

    int status, i;
    char *cVal[10];
    int iVal[10];
    char ticketId[NAME_LEN] = "";
    char writeByteCount[NAME_LEN] = "";
    char writeByteLimit[NAME_LEN] = "";
    rodsLong_t iWriteByteCount = 0;
    rodsLong_t iWriteByteLimit = 0;
    rodsLong_t iDataSize;
    char myWriteByteCount[NAME_LEN];
    rodsLong_t iNewByteCount;

    iDataSize = atoll( dataSize );
    if ( iDataSize == 0 ) {
        return 0;
    }

    for ( i = 0; i < 10; i++ ) {
        iVal[i] = NAME_LEN;
    }

    cVal[0] = ticketId;
    cVal[1] = writeByteCount;
    cVal[2] = writeByteLimit;

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlTicketUpdateWriteBytes SQL 1 " );
    }

    boost::filesystem::path coll_path{original_collection_name};

    // Get the current byte count and byte limit for the ticket from the database.
    // Loop until we've reached the root collection path.
    while(coll_path != irods::get_virtual_path_separator()) {
        std::vector<std::string> bindVars;
        bindVars.push_back( ticketStr );
        bindVars.push_back( objectId );
        bindVars.push_back( objectId );
        status = cmlGetStringValuesFromSql(
                     "select ticket_id, write_byte_count, write_byte_limit from R_TICKET_MAIN where ticket_type = 'write' and ticket_string = ? and (object_id = ? or object_id in (select coll_id from R_DATA_MAIN where data_id = ?))",
                     cVal, iVal, 3, bindVars, icss );
        if(0 == status) {
            break;
        }

        // If we've reached this point, that means the query didn't find anything.
        // In this case, get the parent path and see if the ticket is associated with a parent collection.
        coll_path = coll_path.parent_path();
    }

    if ( status != 0 ) {
        return status;
    }
    iWriteByteLimit = atoll( writeByteLimit );
    iWriteByteCount = atoll( writeByteCount );

    if ( iWriteByteLimit == 0 ) {
        return 0;
    }

    // Calculate new byte count and update the ticket information in the database.
    iNewByteCount = iWriteByteCount + iDataSize;
    snprintf( myWriteByteCount, sizeof myWriteByteCount, "%lld", iNewByteCount );
    cllBindVars[cllBindVarCount++] = myWriteByteCount;
    cllBindVars[cllBindVarCount++] = ticketId;
    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlTicketUpdateWriteBytes SQL 2 " );
    }
    status = cmlExecuteNoAnswerSql("update R_TICKET_MAIN set write_byte_count=? where ticket_id=?", icss);
    // Notice we don't call commit or rollback after executing the update.
    // The reason for this is that this function is allowed to be called within a transaction.
    // The caller of this function is expected to handle committing or rolling back.
    if ( status != 0 ) {
        return status;
    }

#ifndef ORA_ICAT
    cllCheckPending( "", 2, icss->databaseType );
#endif

    return 0;
} // cmlTicketUpdateWriteBytes

/*
  Check that a user has the specified permission or better to a dataObj.
  Return value is either an iRODS error code (< 0) or success (0).
  TicketStr is an optional ticket for ticket-based access,
  TicketHost is an optional host (the connected client IP) for ticket checks.
*/
int cmlCheckDataObjId( const char *dataId, const char *userName,  const char *zoneName,
                       const char *accessLevel, const char *ticketStr, const char *ticketHost,
                       icatSessionStruct *icss ) {
    int status;
    rodsLong_t iVal;

    iVal = 0;
    if ( ticketStr != NULL && *ticketStr != '\0' ) {
        status = checkObjIdByTicket( dataId, accessLevel, ticketStr,
                                     ticketHost, userName, zoneName,
                                     icss );
        if ( status != 0 ) {
            return status;
        }
    }
    else {
        if ( logSQL_CML != 0 ) {
            rodsLog( LOG_SQL, "cmlCheckDataObjId SQL 1 " );
        }
        std::vector<std::string> bindVars;
        bindVars.push_back( dataId );
        bindVars.push_back( userName );
        bindVars.push_back( zoneName );
        bindVars.push_back( accessLevel );
        status = cmlGetIntegerValueFromSql(
                     "select object_id from R_OBJT_ACCESS OA, R_DATA_MAIN DM, R_USER_GROUP UG, R_USER_MAIN UM, R_TOKN_MAIN TM where OA.object_id=? and UM.user_name=? and UM.zone_name=? and UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and OA.object_id = DM.data_id and UG.group_user_id = OA.user_id and OA.access_type_id >= TM.token_id and  TM.token_namespace ='access_type' and TM.token_name = ?",
                     &iVal,
                     bindVars,
                     icss );
        if ( iVal == 0 ) {
            return CAT_NO_ACCESS_PERMISSION;
        }
    }
    if ( status != 0 ) {
        return CAT_NO_ACCESS_PERMISSION;
    }
    return status;
} // cmlCheckDataObjId

/*
 * Check that the user has group-admin permission.  The user must be of
 * type 'groupadmin' and in some cases a member of the specified group.
 */
int cmlCheckGroupAdminAccess( const char *userName, const char *userZone,
                              const char *groupName, icatSessionStruct *icss ) {
    int status;
    char sVal[MAX_NAME_LEN];
    rodsLong_t iVal;

    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckGroupAdminAccess SQL 1 " );
    }

    std::vector<std::string> bindVars;
    bindVars.push_back( userName );
    bindVars.push_back( userZone );
    status = cmlGetStringValueFromSql(
                 "select user_id from R_USER_MAIN where user_name=? and zone_name=? and user_type_name='groupadmin'",
                 sVal, MAX_NAME_LEN, bindVars, icss );
    if ( status == CAT_NO_ROWS_FOUND ) {
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }
    if ( status ) {
        return status;
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4772
    if ( groupName == NULL ) {
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }
    if ( *groupName == '\0' ) {
        return ( 0 );  /* caller is requesting no check for a particular group,
             so if the above check passed, the user is OK */
    }
    // =-=-=-=-=-=-=-
    if ( logSQL_CML != 0 ) {
        rodsLog( LOG_SQL, "cmlCheckGroupAdminAccess SQL 2 " );
    }

    bindVars.clear();
    bindVars.push_back( sVal );
    bindVars.push_back( groupName );
    status = cmlGetIntegerValueFromSql(
                 "select group_user_id from R_USER_GROUP where user_id=? and group_user_id = (select user_id from R_USER_MAIN where user_type_name='rodsgroup' and user_name=?)",
                 &iVal, bindVars, icss );
    if ( status == CAT_NO_ROWS_FOUND ) {
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }
    if ( status ) {
        return status;
    }
    return 0;
}

/*
 Get the number of users who are members of a user group.
 This is used in some groupadmin access checks.
 */
int cmlGetGroupMemberCount( const char *groupName, icatSessionStruct *icss ) {

    rodsLong_t iVal;
    int status;
    std::vector<std::string> bindVars;
    bindVars.push_back( groupName );
    status = cmlGetIntegerValueFromSql(
                 "select count(user_id) from R_USER_GROUP where group_user_id != user_id and group_user_id in (select user_id from R_USER_MAIN where user_name=? and user_type_name='rodsgroup')",
                 &iVal, bindVars, icss );
    if ( status == 0 ) {
        status = iVal;
    }
    return status;
}


