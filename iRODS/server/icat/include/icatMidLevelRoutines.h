/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*******************************************************************************

   This file contains all the  externs of globals used by ICAT

*******************************************************************************/

#ifndef ICAT_MIDLEVEL_ROUTINES_H
#define ICAT_MIDLEVEL_ROUTINES_H

#include "rodsType.h"
#include "icatStructs.h"

int cmlOpen(icatSessionStruct *icss);

int cmlClose( icatSessionStruct *icss);

int cmlExecuteNoAnswerSql( char *sql, 
			   icatSessionStruct *icss);

int cmlGetRowFromSql (char *sql, 
		   char *cVal[], 
		   int cValSize[], 
		   int numOfCols,
		   icatSessionStruct *icss);

int cmlGetOneRowFromSqlV2 (char *sql, 
			   char *cVal[], 
			   int maxCols,
			   char *bindVar1,
			   char *bindVar2,
			   icatSessionStruct *icss);

int cmlGetRowFromSqlV3 (char *sql, 
		   char *cVal[], 
		   int cValSize[], 
		   int numOfCols,
		   icatSessionStruct *icss);

int cmlFreeStatement(int statementNumber, 
		     icatSessionStruct *icss);

int cmlGetFirstRowFromSql (char *sql, 
			   int *statement,
			   int skipCount, 
			   icatSessionStruct *icss);

int cmlGetFirstRowFromSqlBV (char *sql, 
			     char *arg1,
			     char *arg2,
			     char *arg3,
			     char *arg4,
			   int *statement,
			   icatSessionStruct *icss);

int cmlGetNextRowFromStatement (int stmtNum, 
				icatSessionStruct *icss);

int cmlGetStringValueFromSql (char *sql, 
			      char *cVal,  
			      int cValSize,
			      char *bindVar1,
			      char *bindVar2,
			      char *bindVar3,
			      icatSessionStruct *icss);

int cmlGetStringValuesFromSql (char *sql, 
			      char *cVal[],  
			      int cValSize[],
			      int numberOfStringsToGet, 
			      char *bindVar1,
			      char *bindVar2,
			      char *bindVar3,
			      icatSessionStruct *icss);

int cmlGetMultiRowStringValuesFromSql (char *sql, 
			      char *returnedStrings,  
			      int maxStringLen,
			      int maxNumberOfStringsToGet, 
			      char *bindVar1,
			      char *bindVar2,
			      icatSessionStruct *icss);

int cmlGetIntegerValueFromSql (char *sql, 
			    rodsLong_t *iVal,
                            char *bindVar1,
			    char *bindVar2,
                            char *bindVar3,
			    char *bindVar4,
			    char *bindVar5,
			    icatSessionStruct *icss);

int cmlGetIntegerValueFromSqlV3 (char *sql, 
			    rodsLong_t *iVal,
			    icatSessionStruct *icss);

int cmlCheckNameToken(char *nameSpace, 
		      char *tokenName, 
		      icatSessionStruct *icss);

int cmlGetRowFromSingleTable (char *tableName, 
			   char *cVal[], 
			   int cValSize[],
			   char *selectCols[], 
			   char *whereCols[],
			   char *whereConds[], 
			   int numOfSels, 
			   int numOfConds, 
			   icatSessionStruct *icss);

int cmlModifySingleTable (char *tableName, 
		       char *updateCols[],
		       char *updateValues[],
		       char *whereColsAndConds[],
		       char *whereValues[],
		       int numOfUpdates, 
		       int numOfConds, 
		       icatSessionStruct *icss);

int cmlDeleteFromSingleTable (char *tableName, 
			   char *selectCols[],
			   char *selectConds[],
			   int numOfConds, 
			   icatSessionStruct *icss);

int cmlInsertIntoSingleTable (char *tableName,
			   char *insertCols[], 
			   char *insertValues[], 
			   int numOfCols,
			   icatSessionStruct *icss);

int cmlInsertIntoSingleTableV2 (char *tableName,
				char *insertCols, 
				char *insertValues[], 
				int numOfCols,
				icatSessionStruct *icss);

int cmlGetOneRowFromSqlBV (char *sql,
                   char *cVal[],
                   int cValSize[],
                   int numOfCols,
                   char *bindVar1,
                   char *bindVar2,
                   char *bindVar3,
                   char *bindVar4,
                   char *bindVar5,
                   icatSessionStruct *icss);

int cmlGetOneRowFromSql (char *sql,
                   char *cVal[],
                   int cValSize[],
                   int numOfCols,
                   icatSessionStruct *icss);

rodsLong_t cmlGetNextSeqVal(icatSessionStruct *icss);

rodsLong_t cmlGetCurrentSeqVal(icatSessionStruct *icss);

int cmlGetNextSeqStr(char *seqStr, int maxSeqStrLen, icatSessionStruct *icss);

rodsLong_t cmlCheckDir( char *dirName, char *userName, char *userZone, 
			char *accessLevel, icatSessionStruct *icss);

rodsLong_t cmlCheckResc( char *rescName, char *userName, char *userZone, 
			char *accessLevel, icatSessionStruct *icss);

rodsLong_t cmlCheckDirAndGetInheritFlag( char *dirName, char *userName,
			char *userZone, char *accessLevel, 
			int *inheritFlag, icatSessionStruct *icss);

rodsLong_t cmlCheckDirId( char *dirId, char *userName, char *userZone,
			  char *accessLevel, icatSessionStruct *icss);

rodsLong_t cmlCheckDirOwn( char *dirName, char *userName, char *userZone,
			   icatSessionStruct *icss);

int cmlCheckDataObjId( char *dataId, char *userName,  char *zoneName, 
		       char *accessLevel, icatSessionStruct *icss);

rodsLong_t cmlCheckDataObjOnly( char *dirName, char *dataName, char *userName,
				char *userZone,
				char *accessLevel, icatSessionStruct *icss);

rodsLong_t cmlCheckDataObjOwn( char *dirName, char *dataName, char *userName,
			       char *userZone, icatSessionStruct *icss);

int cmlCheckGroupAdminAccess(char *userName, char *userZone, 
			     char *groupName, icatSessionStruct *icss);

int cmlDebug(int mode);

int cmlAudit1(int actionId, char *clientUser, char *zone, char *targetUser, 
	      char *comment, icatSessionStruct *icss);

int cmlAudit2(int actionId, char *dataID, char *userName, char *zoneName, 
	      char *accessLevel, icatSessionStruct *icss);

int cmlAudit3(int actionId, char *dataId, char *clientUser, char *zone, 
	      char *comment, icatSessionStruct *icss);

int cmlAudit4(int actionId, char *sql, char *sqlParm, char *clientUser, 
	      char *zone, char *comment, icatSessionStruct *icss);

int cmlAudit5(int actionId, char *objId, char *userID, char *comment, 
	     icatSessionStruct *icss);

#endif /* ICAT_MIDLEVEL_ROUTINES_H */
