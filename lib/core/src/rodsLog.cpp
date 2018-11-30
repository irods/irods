/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

//Creates irods_error_map
#define MAKE_IRODS_ERROR_MAP
#include "rodsErrorTable.h"
const static std::map<const int, const std::string> irods_error_map = irods_error_map_construction::irods_error_map;

#include "rods.h"
#include "irods_socket_information.hpp"

#ifdef SYSLOG
#ifndef windows_platform
#include <syslog.h>
#endif
#endif

#include "rodsLog.h"
#include "rcGlobalExtern.h"
#include "rcMisc.h"
#include <time.h>
#include <map>
#include <string>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <sys/time.h>
#include "irods_exception.hpp"
#include "irods_logger.hpp"

#ifndef windows_platform
#include <unistd.h>
#endif

#ifdef windows_platform
#include "irodsntutil.hpp"
#endif

#define BIG_STRING_LEN MAX_NAME_LEN+300
#include <stdarg.h>

static int verbosityLevel = LOG_ERROR;
static int sqlVerbosityLevel = 0;
pid_t myPid = 0;

#ifdef windows_platform
static void rodsNtElog( char *msg );
#endif

std::string create_log_error_prefix() {
    std::string ret("remote addresses: ");
    try {
        std::vector<int> fds = get_open_socket_file_descriptors();
        std::vector<std::string> remote_addresses;
        remote_addresses.reserve(fds.size());
        for (const auto fd : fds) {
            std::string remote_address = socket_fd_to_remote_address(fd);
            if (remote_address != "") {
                remote_addresses.push_back(std::move(remote_address));
            }
        }
        if (remote_addresses.size() == 0) {
            ret = "";
            return ret;
        }
        std::sort(remote_addresses.begin(), remote_addresses.end());
        auto new_end = std::unique(remote_addresses.begin(), remote_addresses.end());
        remote_addresses.erase(new_end, remote_addresses.end());
        for (size_t i=0; i<remote_addresses.size()-1; ++i) {
            ret += remote_addresses[i];
            ret += ", ";
        }
        ret += remote_addresses.back();
    } catch (const irods::exception& e) {
        ret = "";
        return ret;
    }

    return ret;
}

void forward_to_syslog(int _log_level, const std::string& _msg)
{
    if (CLIENT_PT == ::ProcessType) {
        return;
    }

    // clang-format off
    using log_level   = int;
    using msg_handler = std::function<void(const std::string&)>;
    using log         = irods::experimental::log;

    static const auto trace    = [](const auto& _msg) { log::legacy::trace(_msg); };
    static const auto debug    = [](const auto& _msg) { log::legacy::debug(_msg); };
    static const auto info     = [](const auto& _msg) { log::legacy::info(_msg); };
    static const auto warn     = [](const auto& _msg) { log::legacy::warn(_msg); };
    static const auto error    = [](const auto& _msg) { log::legacy::error(_msg); };
    static const auto critical = [](const auto& _msg) { log::legacy::critical(_msg); };

    static const std::unordered_map<log_level, msg_handler> msg_handlers{
        {LOG_DEBUG10,     trace},
        {LOG_DEBUG9,      trace},
        {LOG_DEBUG8,      trace},
        {LOG_DEBUG7,      debug},
        {LOG_DEBUG6,      debug},
        {LOG_NOTICE,      info},
        {LOG_WARNING,     warn},
        {LOG_ERROR,       error},
        {LOG_SYS_WARNING, critical},
        {LOG_SYS_FATAL,   critical}
    };
    // clang-format on

    if (const auto iter = msg_handlers.find(_log_level); std::end(msg_handlers) != iter) {
        (iter->second)(_msg);
    }
    else {
        info(_msg);
    }
}

/*
 Log or display a message.  The level argument indicates how severe
 the message is, and depending on the verbosityLevel may or may not be
 recorded.  This is used by both client and server code.
 */
void
rodsLog( int level, const char *formatStr, ... ) {
    time_t timeValue;
    FILE *errOrOut;
    va_list ap;

#ifdef SYSLOG
    char *myZone = getenv( "spProxyRodsZone" );
#endif
    int okToLog = 0;

    char extraInfo[100];
#ifdef windows_platform
    char nt_log_msg[2048];
#endif

    if ( level <= verbosityLevel ) {
        okToLog = 1;
    }

    if ( level == LOG_SQL ) {
        okToLog = 1;
    }

    if ( !okToLog ) {
        return;
    }

    va_start( ap, formatStr );
    char * message = ( char * )malloc( sizeof( char ) * BIG_STRING_LEN );
    int len = vsnprintf( message, BIG_STRING_LEN, formatStr, ap );
    if ( len + 1 > BIG_STRING_LEN ) {
        va_end( ap );
        va_start( ap, formatStr );
        free( message );
        message = ( char * )malloc( sizeof( char ) * ( len + 1 ) );
        vsnprintf( message, len + 1, formatStr, ap );
    }
    va_end( ap );

    extraInfo[0] = '\0';
#ifndef windows_platform
    errOrOut = stdout;
#endif
    if ( ProcessType == SERVER_PT || ProcessType == AGENT_PT ||
            ProcessType == RE_SERVER_PT ) {
        char timeBuf[100];
        time( &timeValue );
        rstrcpy( timeBuf, ctime( &timeValue ), 90 );
        timeBuf[19] = '\0';
        myPid = getpid();
        snprintf( extraInfo, 100 - 1, "%s pid:%d ", timeBuf + 4, myPid );
    }
    else {
#ifndef windows_platform
        if ( level <= LOG_ERROR || level == LOG_SQL ) {
            errOrOut = stderr;
        }
#endif
    }

    std::string prefix = "";
    if ( level == LOG_SQL ) {
        prefix = "LOG_SQL";
    }
    if ( level == LOG_SYS_FATAL ) {
        prefix = "SYSTEM FATAL";
    }
    if ( level == LOG_SYS_WARNING ) {
        prefix = "SYSTEM WARNING";
    }
    if ( level == LOG_ERROR ) {
        prefix = create_log_error_prefix();
        prefix += " ERROR";
    }
    if ( level == LOG_NOTICE ) {
        prefix = "NOTICE";
    }
    if ( level == LOG_WARNING ) {
        prefix = "WARNING";
    }
#ifdef SYSLOG
    if ( level == LOG_DEBUG ) {
        prefix = "DEBUG";
    }
    if ( level == LOG_DEBUG10 ) {
        prefix = "DEBUG1";
    }
    if ( level == LOG_DEBUG9 ) {
        prefix = "DEBUG2";
    }
    if ( level == LOG_DEBUG8 ) {
        prefix = "DEBUG3";
    }
    if ( ProcessType == SERVER_PT || ProcessType == AGENT_PT ||
            ProcessType == RE_SERVER_PT )
#else
    if ( level == LOG_DEBUG ) {
        prefix = "DEBUG";
    }
    if ( level == LOG_DEBUG10 ) {
        prefix = "DEBUG1";
    }
    if ( level == LOG_DEBUG9 ) {
        prefix = "DEBUG2";
    }
    if ( level == LOG_DEBUG8 ) {
        prefix = "DEBUG3";
    }
    if ( message[strlen( message ) - 1] == '\n' )
#endif
    {
#ifdef SYSLOG
#ifdef SYSLOG_FACILITY_CODE
        syslog( SYSLOG_FACILITY_CODE | LOG_NOTICE, "%s - %s: %s", myZone, prefix.c_str(), message );
#else
        syslog( LOG_DAEMON | LOG_NOTICE, "%s - %s: %s", myZone, prefix.c_str(), message );
#endif
#else
#ifndef windows_platform
        fprintf( errOrOut, "%s%s: %s", extraInfo, prefix.c_str(), message );
#else
        sprintf( nt_log_msg, "%s%s: %s", extraInfo, prefix.c_str(), message );
        rodsNtElog( nt_log_msg );
#endif
#endif
    }
    else {
#ifndef windows_platform
        fprintf( errOrOut, "%s%s: %s\n", extraInfo, prefix.c_str(), message );
#else
        sprintf( nt_log_msg, "%s%s: %s\n", extraInfo, prefix.c_str(), message );
        rodsNtElog( nt_log_msg );
#endif
    }
#ifndef windows_platform
    fflush( errOrOut );
#endif

    forward_to_syslog(level, message);

    free( message );
}

/* same as rodsLog plus putting the msg in myError too. Need to merge with
 * rodsLog
 */

void
rodsLogAndErrorMsg( int level, rError_t *myError, int status,
                    const char *formatStr, ... ) {
    char *prefix;
    time_t timeValue;
    FILE *errOrOut;
    va_list ap;
    char errMsg[ERR_MSG_LEN];

    char extraInfo[100];
#ifdef windows_platform
    char nt_log_msg[2048];
#endif

    if ( level > verbosityLevel ) {
        return;
    }


    va_start( ap, formatStr );
    char * message = ( char * )malloc( sizeof( char ) * BIG_STRING_LEN );
    int len = vsnprintf( message, BIG_STRING_LEN, formatStr, ap );
    if ( len + 1 > BIG_STRING_LEN ) {
        va_end( ap );
        va_start( ap, formatStr );
        free( message );
        message = ( char * )malloc( sizeof( char ) * ( len + 1 ) );
        vsnprintf( message, len + 1, formatStr, ap );
    }
    va_end( ap );

    extraInfo[0] = '\0';
    errOrOut = stdout;
    if ( ProcessType == SERVER_PT || ProcessType == AGENT_PT ||
            ProcessType == RE_SERVER_PT ) {
        char timeBuf[100];
        time( &timeValue );
        rstrcpy( timeBuf, ctime( &timeValue ), 90 );
        timeBuf[19] = '\0';
        myPid = getpid();
        snprintf( extraInfo, 100 - 1, "%s pid:%d ", timeBuf + 4, myPid );
    }
    else {
        if ( level <= LOG_ERROR || level == LOG_SQL ) {
            errOrOut = stderr;
        }
    }

    prefix = "";
    if ( level == LOG_SQL ) {
        prefix = "LOG_SQL";
    }
    if ( level == LOG_SYS_FATAL ) {
        prefix = "SYSTEM FATAL";
    }
    if ( level == LOG_SYS_WARNING ) {
        prefix = "SYSTEM WARNING";
    }
    if ( level == LOG_ERROR ) {
        prefix = "ERROR";
    }
    if ( level == LOG_NOTICE ) {
        prefix = "NOTICE";
    }
    if ( level == LOG_WARNING ) {
        prefix = "WARNING";
    }
    if ( level <= LOG_DEBUG ) {
        prefix = "DEBUG";
    }
    const size_t message_len = strlen( message );
    if ( message_len > 0 && message[message_len - 1] == '\n' ) {
#ifndef windows_platform
        fprintf( errOrOut, "%s%s: %s", extraInfo, prefix, message );
        if ( myError != NULL ) {
            snprintf( errMsg, ERR_MSG_LEN,
                      "%s: %s", prefix, message );
            addRErrorMsg( myError, status, errMsg );
        }
#else
        sprintf( nt_log_msg, "%s%s: %s", extraInfo, prefix, message );
        rodsNtElog( nt_log_msg );
#endif
    }
    else {
#ifndef windows_platform
        fprintf( errOrOut, "%s%s: %s\n", extraInfo, prefix, message );
        if ( myError != NULL ) {
            snprintf( errMsg, ERR_MSG_LEN,
                      "%s: %s\n", prefix, message );
            addRErrorMsg( myError, status, errMsg );
        }
#else
        sprintf( nt_log_msg, "%s%s: %s\n", extraInfo, prefix, message );
        rodsNtElog( nt_log_msg );
#endif
    }

#ifndef windows_platform
    fflush( errOrOut );
#endif

    forward_to_syslog(level, message);

    free( message );
}

/*
 Change the verbosityLevel of reporting.
 The input value is the new minimum level of message to report.
 */
void
rodsLogLevel( int level ) {
    verbosityLevel = level;
}

int
getRodsLogLevel() {
    return ( verbosityLevel );
}

/*
 Request sql logging.
 */
void
rodsLogSqlReq( int onOrOff ) {
    sqlVerbosityLevel = onOrOff;
}

void
rodsLogSql( const char *sql ) {
    myPid = getpid();
    if ( sqlVerbosityLevel ) rodsLog( LOG_SQL, "pid: %d sql: %s",
                                          myPid, sql );
}
void
rodsLogSqlResult( const char *stat ) {
    myPid = getpid();
    if ( sqlVerbosityLevel ) rodsLog( LOG_SQL, "pid: %d result: %s",
                                          myPid, stat );
}

/*
Convert an iRODS error code to the corresponding name.
 */
const char *
rodsErrorName( int errorValue, char **subName ) {
    int testVal = errorValue / 1000;

    if ( subName ) {
        int subCode = errorValue - ( testVal * 1000 );
        *subName = subCode && errorValue < 0 ?
                   strdup( strerror( -subCode ) ) :
                   strdup( "" );
    }

    const std::map<const int, const std::string>::const_iterator search = irods_error_map.find( testVal * 1000 );
    if ( search == irods_error_map.end() ) {
        return ( "Unknown iRODS error" );
    }
    return search->second.c_str();
}

/*
 Convert an error code to a string and log it.
 This was originally called rodsLogError, but was renamed when we
 created the new rodsLogError below.  This is no longer used (
 rodsLogError can be called with the same arguments).
 */
void
rodsLogErrorOld( int level, int rodsErrorCode, const char *textStr ) {
    char *errSubName = NULL;

    if ( level < verbosityLevel ) {
        return;
    }

    const char * errName = rodsErrorName( rodsErrorCode, &errSubName );
    if ( textStr && strlen( textStr ) > 0 ) {
        rodsLog( level, "%s Error: %d: %s, %s", textStr, rodsErrorCode,
                 errName, errSubName );
    }
    else {
        rodsLog( level, "Error: %d: %s, %s", rodsErrorCode,
                 errName, errSubName );
    }
    free( errSubName );
}

/* Like rodsLogError but with full rodsLog functionality too.
   Converts the errorcode to a string, and possibly a subcode string,
   and includes that at the end of a regular log message (with
   variable arguments).
 */
void
rodsLogError( int level, int rodsErrorCode, const char *formatStr, ... ) {
    char *errSubName = NULL;
    va_list ap;

    if ( level > verbosityLevel ) {
        return;
    }


    va_start( ap, formatStr );
    char * message = ( char * )malloc( sizeof( char ) * BIG_STRING_LEN );
    int len = vsnprintf( message, BIG_STRING_LEN, formatStr, ap );
    if ( len + 1 > BIG_STRING_LEN ) {
        va_end( ap );
        va_start( ap, formatStr );
        free( message );
        message = ( char * )malloc( sizeof( char ) * ( len + 1 ) );
        vsnprintf( message, len + 1, formatStr, ap );
    }
    va_end( ap );

    const char * errName = rodsErrorName( rodsErrorCode, &errSubName );
    if ( strlen( errSubName ) > 0 ) {
        rodsLog( level, "%s status = %d %s, %s", message, rodsErrorCode,
                 errName, errSubName );
    }
    else {
        rodsLog( level, "%s status = %d %s", message, rodsErrorCode,
                 errName );
    }
    free( message );
    free( errSubName );
}

#ifdef windows_platform
static void rodsNtElog( char *msg ) {
    char log_fname[1024];
    int fd;
    int t;

    if ( ProcessType == CLIENT_PT ) {
        fprintf( stderr, "%s", msg );
        return;
    }

    t = strlen( msg );
    if ( msg[t - 1] == '\n' ) {
        msg[t - 1] = '\0';
        t = t - 1;
    }

    if ( iRODSNtServerRunningConsoleMode() ) {
        t = strlen( msg );
        if ( msg[t - 1] == '\n' ) {
            fprintf( stderr, "%s", msg );
        }
        else {
            fprintf( stderr, "%s\n", msg );
        }
        return;
    }

    t = strlen( msg );
    if ( msg[t - 1] != '\n' ) {
        msg[t] = '\n';
        msg[t + 1] = '\0';
        t = t + 1;
    }

    iRODSNtGetLogFilenameWithPath( log_fname );
    fd = iRODSNt_open( log_fname, O_APPEND | O_WRONLY, 1 );
    _write( fd, msg, t );
    _close( fd );
}
#endif

/*
 * This function will generate an ISO 8601 formatted
 * date/time stamp for use in log messages. The format
 * will be 'YYYYMMDDThhmmss.uuuuuuZ' where:
 *
 * YYYY - is the year
 *   MM - is the month (01-12)
 *   DD - is the day   (01-31)
 *   hh - is the hour (00-24)
 *   mm - is the minute (00-59)
 *   ss - is the second (00-59)
 *   u+ - are the number of microseconds.
 *
 * The date/time stamp is in UTC time.
 */
void
generateLogTimestamp( char *ts, int tsLen ) {
    struct timeval tv;
    struct tm utc;
    char timestamp[TIME_LEN];

    if ( ts == NULL ) {
        return;
    }

    gettimeofday( &tv, NULL );
    gmtime_r( &tv.tv_sec, &utc );
    strftime( timestamp, TIME_LEN, "%Y%m%dT%H%M%S", &utc );

    /* 8 characters of '.uuuuuuZ' + nul */
    if ( tsLen < ( int )strlen( timestamp ) + 9 ) {
        return;
    }

    snprintf( ts, strlen( timestamp ) + 9, "%s.%06dZ", timestamp, ( int )tv.tv_usec );
}
