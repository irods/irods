/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  header file for the ODBC version of the icat low level routines,
  which is for Postgres or MySQL.
 */

#ifndef CLL_PSQ_HPP
#define CLL_PSQ_HPP

#include "sql.h"
#include "sqlext.h"

#include "rods.h"
#include "mid_level.hpp"

#include <vector>
#include <string>

#define MAX_BIND_VARS  120 // JMC - backport 4848 ( 40->120 )

extern int cllBindVarCount;
extern const char *cllBindVars[MAX_BIND_VARS];


/* The name in the various 'odbc.ini' files for the catalog: */
#ifdef UNIXODBC_DATASOURCE
#define CATALOG_ODBC_ENTRY_NAME UNIXODBC_DATASOURCE
#else
#define CATALOG_ODBC_ENTRY_NAME "PostgreSQL"
#endif

/* The name in the various 'odbc.ini' files for the RDA: */
#define RDA_ODBC_ENTRY_NAME "iRODS_RDA"

int cllOpenEnv( icatSessionStruct *icss );
int cllCloseEnv( icatSessionStruct *icss );
int cllConnect( icatSessionStruct *icss );
int cllConnectRda( icatSessionStruct *icss );
int cllDisconnect( icatSessionStruct *icss );
int cllExecSqlNoResult( icatSessionStruct *icss, const char *sql );
int cllExecSqlWithResult( icatSessionStruct *icss, int *stmtNum, const char *sql );
int cllExecSqlWithResultBV( icatSessionStruct *icss, int *stmtNum, const char *sql,
                            std::vector<std::string> &bindVars );
int cllGetRow( icatSessionStruct *icss, int statementNumber );
int cllFreeStatement( icatSessionStruct *icss, int statementNumber );
int cllNextValueString( const char *itemName, char *outString, int maxSize );
extern "C" int cllTest( const char *userArg, const char *pwArg );
int cllCurrentValueString( const char *itemName, char *outString, int maxSize );
int cllGetRowCount( icatSessionStruct *icss, int statementNumber );
int cllCheckPending( const char *sql, int option, int dbType );
int cllGetLastErrorMessage( char *msg, int maxChars );

#endif	/* CLL_PSQ_H */
