/*
 * iclienthints - produce json configuration clienthints
*/
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "lsUtil.h"
#include "irods_buffer_encryption.hpp"
#include <string>
#include <iostream>


void usage() {
    const char *msgs[] = {
        "Usage: iclienthints",
        "iclienthints introspects the entire iRODS Zone for information about installed plugins, configured rules, and the like.",
        "This configuration information will be generated in the form of a JSON",
        "document which will validate using schemas found at https://schemas.irods.org.",
        " ",
        "Configuration files that are included in the zone report are base64 encoded.",
        "They can be decoded from the command line by piping into 'base64 --decode'.",
        "",
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
    printReleaseInfo( "iclienthints" );
}

int
main( int _argc, char** argv ) {

    signal( SIGPIPE, SIG_IGN );

    if ( _argc > 1 ) {
        usage();
        return 0;
    }

    rodsArguments_t myRodsArgs;
    char* optStr = "h";
    int status = parseCmdLineOpt(_argc, argv, optStr, 0, &myRodsArgs);
    if (status) {
        printf("Use -h for help.\n");
        exit(1);
    }
    if (True == myRodsArgs.help) {
        usage();
        exit(0);
    }

    rodsEnv myEnv;
    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    rErrMsg_t errMsg;
    rcComm_t *conn;
    conn = rcConnect(
               myEnv.rodsHost,
               myEnv.rodsPort,
               myEnv.rodsUserName,
               myEnv.rodsZone,
               0, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl = irods::get_pack_table();
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            exit( 7 );
        }
    }

    void *tmp_out = NULL;
    status = procApiRequest( conn, CLIENT_HINTS_AN, NULL, NULL,
                             &tmp_out, NULL );
    if ( status < 0 ) {
        printf( "\n\nERROR - failed in call to rcClientHints - %d\n", status );
        return 0;
    }
    else {
        bytesBuf_t* bbuf = static_cast< bytesBuf_t* >( tmp_out );

        // may not be properly null terminated
        std::string s( ( char* )bbuf->buf, bbuf->len );
        printf( "\n%s\n", s.c_str() );
    }

    rcDisconnect( conn );
}

