/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * ienv - The irods print environment utility
 */

#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>

void usage();

int
main( int argc, char **argv ) {
    signal( SIGPIPE, SIG_IGN );

    char* optStr = "h";
    rodsArguments_t myRodsArgs;
    int status = parseCmdLineOpt( argc, argv, optStr, 0, &myRodsArgs );

    if ( status < 0 ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }

    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    rodsLog( LOG_NOTICE, "Release Version = %s, API Version = %s",
             RODS_REL_VERSION, RODS_API_VERSION );

    status = printRodsEnv( stdout );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    return 0;
}

void
usage() {
    char *msgs[] = {
        "Usage: ienv [-h]",
        "Display current irods environment. Equivalent to iinit -l.",
        "Options are:",
        " -h  this help",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ienv" );
}


