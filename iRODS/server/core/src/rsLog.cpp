/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsLog.c - Routines for log files.
 */

#include "rsLog.hpp"
#include "rsGlobalExtern.hpp"

static time_t LogfileLastChkTime = 0;

char *
getLogDir() {
#ifndef windows_platform
    char *myDir;

    if ( ( myDir = ( char * ) getenv( "irodsLogDir" ) ) != ( char * ) NULL ) {
        return myDir;
    }
    return DEF_LOG_DIR;
#else
    return iRODSNtServerGetLogDir;
#endif
}

void
getLogfileName( char **logFile, char *logDir, char *logFileName ) {
#ifndef _WIN32
    time_t myTime;
    struct tm *mytm;
    char *logfilePattern; // JMC - backport 4793
    char *logfileIntStr;
    int logfileInt;
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
    if ( ( logfileIntStr = getenv( LOGFILE_INT ) ) == NULL ||
            ( logfileInt = atoi( logfileIntStr ) ) < 1 ) {
        logfileInt = DEF_LOGFILE_INT;
    }

    tm_mday = ( mytm->tm_mday / logfileInt ) * logfileInt + 1;
    if ( tm_mday > mytm->tm_mday ) {
        tm_mday -= logfileInt;
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


#else /* for Windows */
    char tmpstr[1024];
    iRODSNtGetLogFilenameWithPath( tmpstr );
    *logFile = _strdup( tmpstr );
#endif
}

#ifndef _WIN32
int
chkLogfileName( char *logDir, char *logFileName ) {
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

#endif

