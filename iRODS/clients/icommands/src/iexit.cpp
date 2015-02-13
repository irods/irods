/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rods.hpp"
#include "parseCommandLine.hpp"
#include "rcMisc.hpp"

void usage( char *prog );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status, ix;
    rodsArguments_t myRodsArgs;
    rodsEnv myEnv;
    char *envFile;

    status = parseCmdLineOpt( argc, argv, "Vvh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage( argv[0] );
        exit( 0 );
    }
    ix = myRodsArgs.optind;

    status = getRodsEnv( &myEnv );

    envFile = getRodsEnvFileName();
    status = unlink( envFile );
    if ( myRodsArgs.verbose == True ) {
        printf( "Deleting (if it exists) session envFile:%s\n", envFile );
        printf( "unlink status = %d\n", status );
    }

    if ( ix < argc ) {
        if ( strcmp( argv[ix], "full" ) == 0 ) {
            if ( myRodsArgs.verbose == True ) {
                obfRmPw( 0 );
            }
            else {
                obfRmPw( 1 );
            }
        }
        else {
            printf( "option %s is unrecognized\n", argv[ix] );
        }
    }

    exit( 0 );
}


void usage( char *prog ) {
    printf( "Exits iRODS session (cwd) and optionally removes\n" );
    printf( "the scrambled password file produced by iinit.\n" );
    printf( "Usage: %s [-vh] [full]\n", prog );
    printf( "If 'full' is included the scrambled password is also removed.\n" );
    printf( " -v  verbose\n" );
    printf( " -V  very verbose\n" );
    printf( " -h  this help\n" );
    printReleaseInfo( "iexit" );
}
