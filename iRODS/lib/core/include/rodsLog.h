/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* definitions for rodsLog routines */

#include "rodsError.h"

#define LOG_SQL 11
/* This is for logging SQL statements.  These are only logged when
   specifically requested and so are a high priority at this level. */

#define LOG_DEBUG1  10
#define LOG_DEBUG2  9
#define LOG_DEBUG3  8
#define LOG_DEBUG   7
/*
  The DEBUG messages are for the software engineer to analyze and
  debug operations.  These are typically added during development and
  debug of the code, but can be left in place for possible future
  use.  In many cases, one would be able start seeing these messages
  via a command-line argument to adjust the rodsLog verbosity level.
*/

#define LOG_NOTICE  5
/* This is informational only, part of the normal operation but will
   often be of interest. */

#define LOG_ERROR  3
/* This means that the function cannot complete what it was asked to
   do, probably because of bad input from the user (an invalid host
   name, for example). */

#define LOG_SYS_WARNING 2
/* This means a system-level problem occurred that is not fatal to the
   whole system (it can continue to run), but does indicate an internal
   inconsistency of some kind.  An example is a file with a physical
   size that is different than that recorded in the database.
*/
#define LOG_SYS_FATAL 1
/* This is used of errors that mean that the system (not just one
   server, client, or user) cannot continue.  An example is when the
   server is unable to talk to the database. */

#ifdef  __cplusplus
extern "C" {
#endif

void rodsLog(int level, char *formatStr, ...);
void rodsLogAndErrorMsg (int level, rError_t *myError, int status,
char *formatStr, ...);
void rodsLogLevel(int level);
void rodsLogSqlReq(int onOrOff);
void rodsLogSql(char *sql);
void rodsLogSqlResult(char *stat);
char *rodsErrorName(int errorValue, char **subName);
void rodsLogErrorOld(int level, int errCode, char *textStr);
void rodsLogError(int level, int errCode, char *formatStr, ...);
int getRodsLogLevel ();

#ifdef  __cplusplus
}
#endif

