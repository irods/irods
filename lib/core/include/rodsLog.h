/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* definitions for rodsLog routines */
#ifndef RODS_LOG_H__
#define RODS_LOG_H__

#include "rodsError.h"

#define LOG_SQL 11
/* This is for logging SQL statements.  These are only logged when
   specifically requested and so are a high priority at this level. */


#define LOG_DEBUG1   10
#define LOG_DEBUG2   9
#define LOG_DEBUG3   8
/* Legacy DEBUG levels - leaving here for backwards compatibility */

#define LOG_DEBUG10  10
#define LOG_DEBUG9   9
#define LOG_DEBUG8   8
#define LOG_DEBUG7   7
#define LOG_DEBUG    7
#define LOG_DEBUG6   6
/*
  The DEBUG messages are for the software engineer to analyze and
  debug operations.  These are typically added during development and
  debug of the code, but can be left in place for possible future
  use.  In many cases, one would be able start seeing these messages
  via a command-line argument to adjust the rodsLog verbosity level.
*/

#define LOG_NOTICE  5
#define LOG_STATUS  5
/* These are informational only, part of the normal operation but will
   often be of interest. */

#define LOG_WARNING    4
/* This means that the system is probably not performing as expected.
   This is not harmful, but should probably be looked into by the
   administrator. */

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
/* This is used for errors that mean that the system (not just one
   server, client, or user) cannot continue.  An example is when the
   server is unable to talk to the database. */

#ifdef __cplusplus
extern "C" {
#endif
void rodsLog( int level, const char *formatStr, ... );
void rodsLogAndErrorMsg( int level, rError_t *myError, int status,
                         const char *formatStr, ... );
void rodsLogLevel( int level );
void rodsLogSqlReq( int onOrOff );
void rodsLogSql( const char *sql );
void rodsLogSqlResult( const char *stat );
const char *rodsErrorName( int errorValue, char **subName );
void rodsLogErrorOld( int level, int errCode, const char *textStr );
void rodsLogError( int level, int errCode, const char *formatStr, ... );
int getRodsLogLevel();
void generateLogTimestamp( char *ts, int tsLen );

#define TRACE_LOG() rodsLog(LOG_NOTICE, "[%s:%d]", __FUNCTION__, __LINE__);

#ifdef __cplusplus
}
#endif

#endif // RODS_LOG_H__
