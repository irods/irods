/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include <irods/rods.h>
#include <irods/parseCommandLine.h>
#include <irods/rcMisc.h>
#include <irods/rodsPath.h>

#include <cstdlib>
#include <iostream>

void usage( char *prog );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsArguments_t myRodsArgs;
    rodsEnv myEnv;

    status = parseCmdLineOpt( argc, argv, "vVh", 0, &myRodsArgs );
    if ( status ) {
        std::cout << "Use -h for help" << std::endl;
        std::exit(1);
    }

    if ( myRodsArgs.help == True ) {
        usage( argv[0] );
        std::exit(0);
    }

    status = getRodsEnv( &myEnv );
    if ( status != 0 ) {
        std::cout << "Failed with error " << status << std::endl;
        std::exit(2);
    }

    auto* path = escape_path(myEnv.rodsCwd);

    std::cout << path << std::endl;

    std::free(path);

    std::exit(0);
}

void usage( char *prog ) {
    std::cout << "Shows your iRODS Current Working Directory.\n"
              << "Usage: " << prog << " [-vVh]\n"
              << " -v  verbose\n"
              << " -V  very verbose\n"
              << " -h  this help" << std::endl;
    printReleaseInfo( "ipwd" );
}
