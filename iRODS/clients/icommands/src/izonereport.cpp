/*
 * izonereport - produce json configuration zonereport
*/
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "lsUtil.hpp"
#include "irods_buffer_encryption.hpp"
#include "zone_report.hpp"
#include <string>
#include <iostream>

int
main( int, char** ) {

    signal( SIGPIPE, SIG_IGN );


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
    status = procApiRequest( conn, ZONE_REPORT_AN, NULL, NULL,
                             &tmp_out, NULL );
    if ( status < 0 ) {
        printf( "\n\nERROR - failed in call to rcZoneReport" );
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

