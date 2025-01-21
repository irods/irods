/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * ienv - The irods print environment utility
 */

#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>

#include <cstdlib>
#include <iostream>

void usage();

int
main( int argc, char **argv ) {
    signal( SIGPIPE, SIG_IGN );

    rodsArguments_t myRodsArgs;

    int status = parseCmdLineOpt(argc, argv, "h", 0, &myRodsArgs);

    if ( status < 0 ) {
        std::cout << "Use -h for help" << std::endl;
        std::exit(1);
    }

    if ( myRodsArgs.help == True ) {
        usage();
        std::exit(0);
    }

    rodsLog( LOG_NOTICE, "Release Version = %s, API Version = %s",
             RODS_REL_VERSION, RODS_API_VERSION );

    status = printRodsEnv( stdout );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        std::exit(1);
    }

    return 0;
}

void
usage() {
    std::cout << "Displays current irods environment. Equivalent to iinit -l.\n"
              << "Usage: ienv [-h]\n"
              << " -h  this help" << std::endl;
    printReleaseInfo( "ienv" );
}


