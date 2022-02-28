/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*******************************************************************************

   This file contains all the  externs of globals used by ICAT

*******************************************************************************/

#ifndef ICAT_MIDLEVEL_ROUTINES_HPP
#define ICAT_MIDLEVEL_ROUTINES_HPP

#include "irods/rodsType.h"
#include "irods/icatStructs.hpp"

#include <vector>
#include <string>

const int UNINITIALIZED_STATEMENT_NUMBER = -1;

int cmlOpen( icatSessionStruct *icss );

int cmlClose( icatSessionStruct *icss );

int cmlExecuteNoAnswerSql( const char *sql,
                           icatSessionStruct *icss );

int cmlGetRowFromSql( const char *sql,
                      char *cVal[],
                      int cValSize[],
                      int numOfCols,
                      icatSessionStruct *icss );

int cmlGetOneRowFromSqlV2( const char *sql,
                           char *cVal[],
                           int maxCols,
                           std::vector<std::string> &bindVars,
                           icatSessionStruct *icss );

int cmlGetRowFromSqlV3( const char *sql,
                        char *cVal[],
                        int cValSize[],
                        int numOfCols,
                        icatSessionStruct *icss );

int cmlFreeStatement( int statementNumber,
                      icatSessionStruct *icss );

int cmlGetFirstRowFromSql( const char *sql,
                           int *statement,
                           int skipCount,
                           icatSessionStruct *icss );

int cmlGetFirstRowFromSqlBV( const char *sql,
                             std::vector<std::string> &bindVars,
                             int *statement,
                             icatSessionStruct *icss );

int cmlGetNextRowFromStatement( int stmtNum,
                                icatSessionStruct *icss );

int cmlGetStringValueFromSql( const char *sql,
                              char *cVal,
                              int cValSize,
                              std::vector<std::string> &bindVars,
                              icatSessionStruct *icss );

int cmlGetStringValuesFromSql( const char *sql,
                               char *cVal[],
                               int cValSize[],
                               int numberOfStringsToGet,
                               std::vector<std::string> &bindVars,
                               icatSessionStruct *icss );

int cmlGetMultiRowStringValuesFromSql( const char *sql,
                                       char *returnedStrings,
                                       int maxStringLen,
                                       int maxNumberOfStringsToGet,
                                       std::vector<std::string> &bindVars,
                                       icatSessionStruct *icss );

int cmlGetIntegerValueFromSql( const char *sql,
                               rodsLong_t *iVal,
                               std::vector<std::string> &bindVars,
                               icatSessionStruct *icss );

int cmlGetIntegerValueFromSqlV3( const char *sql,
                                 rodsLong_t *iVal,
                                 icatSessionStruct *icss );

int cmlCheckNameToken( const char *nameSpace,
                       const char *tokenName,
                       icatSessionStruct *icss );

int cmlGetRowFromSingleTable( const char *tableName,
                              char *cVal[],
                              int cValSize[],
                              const char *selectCols[],
                              const char *whereCols[],
                              const char *whereConds[],
                              int numOfSels,
                              int numOfConds,
                              icatSessionStruct *icss );

int cmlModifySingleTable( const char *tableName,
                          const char *updateCols[],
                          const char *updateValues[],
                          const char *whereColsAndConds[],
                          const char *whereValues[],
                          int numOfUpdates,
                          int numOfConds,
                          icatSessionStruct *icss );

int cmlDeleteFromSingleTable( const char *tableName,
                              const char *selectCols[],
                              const char *selectConds[],
                              int numOfConds,
                              icatSessionStruct *icss );

int cmlInsertIntoSingleTable( const char *tableName,
                              const char *insertCols[],
                              const char *insertValues[],
                              int numOfCols,
                              icatSessionStruct *icss );

int cmlInsertIntoSingleTableV2( const char *tableName,
                                const char *insertCols,
                                const char *insertValues[],
                                int numOfCols,
                                icatSessionStruct *icss );

int cmlGetOneRowFromSqlBV( const char *sql,
                           char *cVal[],
                           int cValSize[],
                           int numOfCols,
                           std::vector<std::string> &bindVars,
                           icatSessionStruct *icss );

int cmlGetOneRowFromSql( const char *sql,
                         char *cVal[],
                         int cValSize[],
                         int numOfCols,
                         icatSessionStruct *icss );

rodsLong_t cmlGetNextSeqVal( icatSessionStruct *icss );

rodsLong_t cmlGetCurrentSeqVal( icatSessionStruct *icss );

int cmlGetNextSeqStr( char *seqStr, int maxSeqStrLen, icatSessionStruct *icss );

rodsLong_t cmlCheckDir( const char *dirName, const char *userName, const char *userZone,
                        const char *accessLevel, icatSessionStruct *icss, bool admin_mode = false );

rodsLong_t cmlCheckResc( const char *rescName, const char *userName, const char *userZone,
                         const char *accessLevel, icatSessionStruct *icss );

rodsLong_t cmlCheckDirAndGetInheritFlag( const char *dirName, const char *userName,
        const char *userZone, const char *accessLevel,
        int *inheritFlag, const char *ticketStr, const char *ticketHost,
        icatSessionStruct *icss );

rodsLong_t cmlCheckDirId( const char *dirId, const char *userName, const char *userZone,
                          const char *accessLevel, icatSessionStruct *icss );

rodsLong_t cmlCheckDirOwn( const char *dirName, const char *userName, const char *userZone,
                           icatSessionStruct *icss );

int cmlCheckDataObjId( const char *dataId, const char *userName, const char *zoneName,
                       const char *accessLevel, const char *ticketStr,
                       const char *ticketHost,
                       icatSessionStruct *icss );

int cmlTicketUpdateWriteBytes( const char *ticketStr,
                               const char *dataSize, const char *objectId,
                               icatSessionStruct *icss );

rodsLong_t cmlCheckDataObjOnly( const char *dirName, const char *dataName, const char *userName,
                                const char *userZone, const char *accessLevel, icatSessionStruct *icss,
                                bool admin_mode = false);

rodsLong_t cmlCheckDataObjOwn( const char *dirName, const char *dataName, const char *userName,
                               const char *userZone, icatSessionStruct *icss );

int cmlCheckGroupAdminAccess( const char *userName, const char *userZone,
                              const char *groupName, icatSessionStruct *icss );

int cmlGetGroupMemberCount( const char *groupName, icatSessionStruct *icss );

int cmlDebug( int mode );

#endif /* ICAT_MIDLEVEL_ROUTINES_H */
