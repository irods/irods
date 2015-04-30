/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  header file for the Oracle version of the icat low level routines.
 */

#ifndef CLL_ORA_HPP
#define CLL_ORA_HPP

#include "oci.h"

#include "rods.h"
#include "mid_level.hpp"

#define MAX_BIND_VARS 120 // JMC - backport 4848 ( 40->120 )

extern int cllBindVarCount;
extern const char *cllBindVars[MAX_BIND_VARS];


int cllOpenEnv( icatSessionStruct *icss );
int cllCloseEnv( icatSessionStruct *icss );
int cllConnect( icatSessionStruct *icss );
int cllDisconnect( icatSessionStruct *icss );
int cllExecSqlNoResult( icatSessionStruct *icss, const char *sql );
int cllExecSqlNoResultBV( icatSessionStruct *icss, const char *sql, std::vector<std::string> bindVars );
int cllExecSqlWithResult( icatSessionStruct *icss, int *stmtNum, const char *sql );
int cllExecSqlWithResultBV( icatSessionStruct *icss, int *stmtNum, const char *sql,
                            std::vector<std::string> &bindVars );
int cllGetRow( icatSessionStruct *icss, int statementNumber );
int cllFreeStatement( icatSessionStruct *icss, int statementNumber );
int cllNextValueString( const char *itemName, char *outString, int maxSize );
extern "C" int cllTest( const char *userArg, const char *pwArg );
int cllCurrentValueString( const char *itemName, char *outString, int maxSize );
int cllGetRowCount( icatSessionStruct *icss, int statementNumber );

int cllConnectRda( icatSessionStruct *icss );
int cllGetLastErrorMessage( char *msg, int maxChars );
#endif	/* CLL_ORA_H */
