/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  Simple command to get the misc server info.
  Tests connecting to the server.
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

void usage();

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsEnv myEnv;
    rcComm_t *Conn;
    rErrMsg_t errMsg;
    miscSvrInfo_t *miscSvrInfo;
    rodsArguments_t myRodsArgs;

    status = parseCmdLineOpt( argc, argv,  "hvV", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        exit( 2 );
    }

    status = rcGetMiscSvrInfo( Conn, &miscSvrInfo );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "rcGetMiscSvrInfo failed" );
        exit( 3 );
    }

    if ( miscSvrInfo->serverType == RCAT_NOT_ENABLED ) {
        printf( "RCAT_NOT_ENABLED\n" );
    }
    if ( miscSvrInfo->serverType == RCAT_ENABLED ) {
        printf( "RCAT_ENABLED\n" );
    }
    printf( "relVersion=%s\n", miscSvrInfo->relVersion );
    printf( "apiVersion=%s\n", miscSvrInfo->apiVersion );
    printf( "rodsZone=%s\n", miscSvrInfo->rodsZone );

    if ( miscSvrInfo->serverBootTime > 0 ) {
        uint upTimeSec, min, hr, day;

        upTimeSec = time( 0 ) - miscSvrInfo->serverBootTime;
        min = upTimeSec / 60;
        hr = min / 60;
        min = min % 60;
        day = hr / 24;
        hr = hr % 24;
        printf( "up %d days, %d:%d\n", day, hr, min );
    }
    rcDisconnect( Conn );

    exit( 0 );
}

void
usage() {
    char *msgs[] = {
        "Usage: imiscsrvinfo [-hvV]",
        " -v  verbose",
        " -V  Very verbose",
        " -h  this help",
        "Connect to the server and retrieve some basic server information.",
        "Can be used as a simple test for connecting to the server.",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "imiscsvrinfo" );
}
