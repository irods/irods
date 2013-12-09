/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  header file for the Oracle version of the icat low level routines.
 */

#ifndef CLL_ORA_HPP
#define CLL_ORA_HPP

#include "oci.hpp"

#include "rods.hpp"
#include "icatMidLevelRoutines.hpp"

#define MAX_BIND_VARS 120 // JMC - backport 4848 ( 40->120 )

extern int cllBindVarCount;
extern char *cllBindVars[MAX_BIND_VARS];


int cllOpenEnv( icatSessionStruct *icss );
int cllCloseEnv( icatSessionStruct *icss );
int cllConnect( icatSessionStruct *icss );
int cllDisconnect( icatSessionStruct *icss );
int cllExecSqlNoResult( icatSessionStruct *icss, char *sql );
int cllExecSqlNoResultBV( icatSessionStruct *icss, char *sql, char *bindVar1,
                          char *bindVar2, char *bindVar3 );
int cllExecSqlWithResult( icatSessionStruct *icss, int *stmtNum, char *sql );
int cllExecSqlWithResultBV( icatSessionStruct *icss, int *stmtNum, char *sql,
                            const char *bindVar1, const char *bindVar2,
                            const char *bindVar3, const char *bindVar4,
                            const char *bindVar5, const char *bindVar6 );
int cllGetRow( icatSessionStruct *icss, int statementNumber );
int cllFreeStatement( icatSessionStruct *icss, int statementNumber );
int cllNextValueString( char *itemName, char *outString, int maxSize );
int cllTest( char *userArg, char *pwArg );
int cllCurrentValueString( char *itemName, char *outString, int maxSize );
int cllGetRowCount( icatSessionStruct *icss, int statementNumber );

int cllConnectRda( icatSessionStruct *icss );
int cllConnectDbr( icatSessionStruct *icss, char *unused );
int cllGetLastErrorMessage( char *msg, int maxChars );
#endif	/* CLL_ORA_H */
