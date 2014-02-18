/*
 * iapitest - test pluggable apis
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "lsUtil.hpp"
#include "irods_buffer_encryption.hpp"
#include <string>
#include <iostream>



void usage();

int
main( int argc, char **argv ) {

    rodsEnv myEnv;
    int status = getRodsEnv( &myEnv );
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
    init_api_table( RcApiTable, ApiPackTable );

    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            exit( 7 );
        }
    }

    status = procApiRequest( conn, 1300, NULL, NULL,
                             ( void ** ) NULL, NULL );
    if ( status < 0 ) {
        printf( "\n\nXXXX - failed to call our api\n\n\n" );
        return 0;
    }
}

