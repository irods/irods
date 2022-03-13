/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  header file for the ODBC version of the icat low level routines,
  which is for Postgres or MySQL.
 */

#ifndef CLL_ODBC_HPP
#define CLL_ODBC_HPP

#include <sql.h>
#include <sqlext.h>

#include "irods/rods.h"
#include "irods/private/mid_level.hpp"

#include <vector>
#include <string>

#define MAX_BIND_VARS 32000

extern int cllBindVarCount;
extern const char *cllBindVars[MAX_BIND_VARS];

int cllOpenEnv( icatSessionStruct *icss );
int cllCloseEnv( icatSessionStruct *icss );
int cllConnect( icatSessionStruct *icss );
int cllDisconnect( icatSessionStruct *icss );
int cllExecSqlNoResult( icatSessionStruct *icss, const char *sql );
int cllExecSqlWithResult( icatSessionStruct *icss, int *stmtNum, const char *sql );
int cllExecSqlWithResultBV( icatSessionStruct *icss, int *stmtNum, const char *sql,
                            std::vector<std::string> &bindVars );
int cllGetRow( icatSessionStruct *icss, int statementNumber );
int cllFreeStatement( icatSessionStruct *icss, int& statementNumber );
int cllNextValueString( const char *itemName, char *outString, int maxSize );
int cllCurrentValueString( const char *itemName, char *outString, int maxSize );
int cllGetRowCount( icatSessionStruct *icss, int statementNumber );
int cllCheckPending( const char *sql, int option, int dbType );
int cllGetLastErrorMessage( char *msg, int maxChars );

#endif	/* CLL_ODBC_HPP */
