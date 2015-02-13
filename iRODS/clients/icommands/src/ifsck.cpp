/*** Copyright (c) 2010 Data Intensive Cyberinfrastructure Foundation. All rights reserved.    ***
 *** For full copyright notice please refer to files in the COPYRIGHT directory                ***/
/* Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation */
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "fsckUtil.hpp"
void usage();

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    char *optStr, hostname[LONG_NAME_LEN];
    rodsPathInp_t rodsPathInp;


    optStr = "hrK";

    status = parseCmdLineOpt( argc, argv, optStr, 0, &myRodsArgs );

    if ( status < 0 ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_FILE_T, NO_INPUT_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        usage();
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }

    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            exit( 7 );
        }
    }

    status = gethostname( hostname, LONG_NAME_LEN );
    if ( status < 0 ) {
        printf( "cannot resolve server name, aborting!\n" );
        exit( 4 );
    }

    status = fsckObj( conn, &myRodsArgs, &rodsPathInp, hostname );

    printErrorStack( conn->rError );

    rcDisconnect( conn );

    exit( status );

}

void
usage() {
    char *msgs[] = {
        "Usage : ifsck [-rhK] srcPhysicalFile|srcPhysicalDirectory ... ",
        "Check if a local data object or a local collection content is",
        "consistent in size (or optionally its checksum) with its",
        "registered size (and optionally its checksum) in iRODS.",
        "It allows to detect iRODS files which have been corrupted or",
        "modified outside the iRODS framework on the local filesytem.",
        "srcPhysicalFile or srcPhysicalDirectory must be a full path name.",
        "Options are:",
        " -K  verify the checksum of the local file wrt the one registered in iRODS.",
        "     Only relevant if the checksum has been computed for the iRODS objects.",
        " -r  recursive - scan local subdirectories",
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
    printReleaseInfo( "ifsck" );
}
