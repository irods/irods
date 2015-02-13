/*** Copyright (c), The Regents of the University of California            ***
 *** For more informtrashation please refer to files in the COPYRIGHT directory ***/
/*
 * irmtrash - The irods rmtrash utility
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "rmtrashUtil.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

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


    optStr = "hru:vVz:MZ";

    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs );
    if ( status < 0 ) {
        printf( "Use -h for help.\n" );
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

    /* use the trash home as cwd */
    snprintf( myEnv.rodsCwd, LONG_NAME_LEN, "/%s/trash/home/%s",
              myEnv.rodsZone, myEnv.rodsUserName );
    rstrcpy( myEnv.rodsHome, myEnv.rodsCwd, LONG_NAME_LEN );
    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, NO_INPUT_T, 0, &rodsPathInp );

    if ( status < 0 && status != USER__NULL_INPUT_ERR ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 1, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
        exit( 7 );
    }

    status = rmtrashUtil( conn, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
        exit( 3 );
    }
    else {
        exit( 0 );
    }

}

void
usage() {
    char *msgs[] = {
        "Usage : irmtrash [-hMrvV] [--orphan] [-u user] [-z zoneName] [--age age_in_minutes] [dataObj|collection ...] ",
        "Remove one or more data-object or collection from a RODS trash bin.",
        "If the input dataObj|collection is not specified, the entire trash bin",
        "of the user (/$myZone/trash/$myUserName) will be removed.",
        " ",
        "The dataObj|collection can be used to specify the paths of dataObj|collection",
        "in the trash bin to be deleted. If the path is relative (does not start",
        "with '/'), the path assumed to be relative to be the user's trash home path",
        "e.g., /myZone/trash/home/myUserName.",
        " ",
        "An admin user can use the -M option to delete other users' trash bin.",
        "The -u option can be used by an admin user to delete the trash bin of",
        "a specific user. If the -u option is not used, the trash bins of all",
        "users will be deleted.",
        " ",
        "Options are:",
        " -r  recursive - remove the whole subtree; the collection, all data-objects",
        "     in the collection, and any subcollections and sub-data-objects in the",
        "     collection.",
        " -M  admin mode",
        " --orphan  remove the orphan files in the /myZone/trash/orphan collection",
        "     It must be used with the -M option.",
        " -u  user - Used with the -M option allowing an admin user to delete a",
        "     specific user's trash bin.",
        " -v  verbose",
        " -V  Very verbose",
        " -z  zoneName - the zone where the rm trash will be carried out",
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
    printReleaseInfo( "irmtrash" );
}

