/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsLog.c - Routines for log files.
 */

#include "rsLog.hpp"
#include "rsGlobalExtern.hpp"

static time_t LogfileLastChkTime = 0;

char *
getLogDir() {
    char *myDir;

    if ( ( myDir = ( char * ) getenv( "irodsLogDir" ) ) != ( char * ) NULL ) {
        return myDir;
    }
    return DEF_LOG_DIR;
}

int get_log_file_rotation_time() {
    char* rotation_time_str = getenv(LOGFILE_INT);
    if ( rotation_time_str ) {
        const int rotation_time = atoi(rotation_time_str);
        if( rotation_time >= 1 ) {
            return rotation_time;
        }
    }

    try {
        const int rotation_time = irods::get_advanced_setting<const int>(irods::DEFAULT_LOG_ROTATION_IN_DAYS);
        if(rotation_time >= 1) {
            return rotation_time;
        }

    }
    catch( irods::exception& _e ) {
    }

    return DEF_LOGFILE_INT;

} // get_log_file_rotation_time

void
getLogfileName( char **logFile, const char *logDir, const char *logFileName ) {
    time_t myTime;
    struct tm *mytm = nullptr;
    char *logfilePattern = nullptr; // JMC - backport 4793
    int tm_mday = 1;
    char logfileSuffix[MAX_NAME_LEN]; // JMC - backport 4793
    char myLogDir[MAX_NAME_LEN];

    /* Put together the full pathname of the logFile */

    if ( logDir == NULL ) {
        snprintf( myLogDir, MAX_NAME_LEN, "%-s", getLogDir() );
    }
    else {
        snprintf( myLogDir, MAX_NAME_LEN, "%-s", logDir );
    }
    *logFile = ( char * ) malloc( strlen( myLogDir ) + strlen( logFileName ) + 24 );




    LogfileLastChkTime = myTime = time( 0 );
    mytm = localtime( &myTime );
    const int rotation_time = get_log_file_rotation_time();

    tm_mday = ( mytm->tm_mday / rotation_time ) * rotation_time + 1;
    if ( tm_mday > mytm->tm_mday ) {
        tm_mday -= rotation_time;
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4793
    if ( ( logfilePattern = getenv( LOGFILE_PATTERN ) ) == NULL ) {
        logfilePattern = DEF_LOGFILE_PATTERN;
    }
    mytm->tm_mday = tm_mday;
    strftime( logfileSuffix, MAX_NAME_LEN, logfilePattern, mytm );
    sprintf( *logFile, "%-s/%-s.%-s", myLogDir, logFileName, logfileSuffix );
    // =-=-=-=-=-=-=-


}

int
chkLogfileName( const char *logDir, const char *logFileName ) {
    time_t myTime;
    char *logFile = NULL;
    int i;

    myTime = time( 0 );
    if ( myTime < LogfileLastChkTime + LOGFILE_CHK_INT ) {
        /* not time yet */
        return 0;
    }

    getLogfileName( &logFile, logDir, logFileName );

    if ( CurLogfileName != NULL && strcmp( CurLogfileName, logFile ) == 0 ) {
        free( logFile );
        return 0;
    }

    /* open the logfile */

    if ( ( i = open( logFile, O_CREAT | O_RDWR, 0644 ) ) < 0 ) {
        fprintf( stderr, "Unable to open logFile %s\n", logFile );
        free( logFile );
        return -1;
    }
    else {
        lseek( i, 0, SEEK_END );
    }

    if ( CurLogfileName != NULL ) {
        free( CurLogfileName );
    }

    CurLogfileName = logFile;

    close( 0 );
    close( 1 );
    close( 2 );
    ( void ) dup2( i, 0 );
    ( void ) dup2( i, 1 );
    ( void ) dup2( i, 2 );
    ( void ) close( i );

    return 0;
}
