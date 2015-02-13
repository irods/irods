/*
 * ils - The irods ls utility
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "lsUtil.hpp"


#include "irods_buffer_encryption.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

#include <string>
#include <iostream>



void usage();

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    char *optStr;
    rodsPathInp_t rodsPathInp;

    // -=-=-=-=-=- JMC - backport 4536 -=-=-=-=-=-
    optStr = "hArlLvt:VZ";
    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs );
    // -=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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
                               UNKNOWN_OBJ_T, NO_INPUT_T, ALLOW_NO_SRC_FLAG, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        usage();
        exit( 1 );
    }

    conn = rcConnect(
               myEnv.rodsHost,
               myEnv.rodsPort,
               myEnv.rodsUserName,
               myEnv.rodsZone,
               0, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }
#if 1
    init_client_api_table();
#else
    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );
#endif
    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            exit( 7 );
        }
    }

    status = lsUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
        exit( 4 );
    }
    else {
        exit( 0 );
    }

}

void
usage() {
    char *msgs[] = {
        "Usage : ils [-ArlLv] dataObj|collection ... ",
        "Usage : ils --bundle [-r] dataObj|collection ... ",
        "Display data Objects and collections stored in irods.",
        "Options are:",
        " -A  ACL (access control list) and inheritance format",
        " -l  long format",
        " -L  very long format",
        " -r  recursive - show subcollections",
        " -t  ticket - use a read (or write) ticket to access collection information",
        " -v  verbose",
        " -V  Very verbose",
        " -h  this help",
        " --bundle - list the subfiles in the bundle file (usually stored in the",
        "     /myZone/bundle collection) created by iphybun command.",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ils" );
}
