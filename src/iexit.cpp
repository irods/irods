/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include <irods/rods.h>
#include <irods/parseCommandLine.h>
#include <irods/rcMisc.h>

void usage( char *prog );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    rodsArguments_t myRodsArgs;

    int status = parseCmdLineOpt( argc, argv, "Vvh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage( argv[0] );
        exit( 0 );
    }

    rodsEnv myEnv;
    getRodsEnv( &myEnv );
    char* envFile = getRodsEnvFileName();
    status = unlink( envFile );
    if ( myRodsArgs.verbose == True ) {
        printf( "Deleting (if it exists) session envFile: %s\n", envFile );
        printf( "unlink status [%d]\n", status );
        // prompt before removing
        obfRmPw( 0 );
    }
    else {
        // do not prompt
        obfRmPw( 1 );
    }

    exit( 0 );
}


void usage( char *prog ) {
    printf( "Exits iRODS session (cwd) and removes\n" );
    printf( "the scrambled password file produced by iinit.\n" );
    printf( "Usage: %s [-vVh]\n", prog );
    printf( " -v  verbose\n" );
    printf( " -V  very verbose\n" );
    printf( " -h  this help\n" );
    printReleaseInfo( "iexit" );
}
