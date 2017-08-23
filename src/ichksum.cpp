/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * ichksum - The irods chksum utility
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "chksumUtil.h"
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


    optStr = "hKfarR:vVn:Z";

    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs );
    if ( status < 0 ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    if ( argc - optind <= 0 ) {
        rodsLog( LOG_ERROR, "ichksum: no input" );
        printf( "Use -h for help.\n" );
        exit( 2 );
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, NO_INPUT_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
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

    status = chksumUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

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
        "Usage: ichksum [-harvV] [-K|f] [-n replNum] [-R resource] [--silent]",
        "           dataObj|collection ... ",
        "Checksum one or more data-object or collection from iRODS space.",
        " ",
        "By default, objects are silently ignored if they already have a catalog checksum.",
        "Any checksum failure is logged to stderr, the command's exit code will be non-zero.",
        "If the command cannot execute its tasks for any reason, it may stop processing",
        "entirely.",
        " ",
        "Options are:",
        " -f  force - checksum data-objects even if a checksum already exists in iCAT",
        "     Overwrites checksum in catalog to achieve a consistent state, in case",
        "     file checksum and catalog checksum differ. (This event is not logged.)",
        " -a  checksum all replicas. ils -L should be used to list the values of all replicas",
        "     If used together with -K, and the replicas have different checksums,",
        "     an error message is displayed, but no checksum is updated in the catalog.",
        " -K  verify the checksum value in iCAT. If the checksum value does not exist,",
        "     compute and register one. If the catalog checksum exists and the file checksum",
        "     differs, returns USER_CHKSUM_MISMATCH and does not update in the catalog.",
        " --verify  - Requires -K. Verifies that the size in the vault is equal to the size",
        "     in the catalog. When verifying checksums with -K, the checksum of the entire",
        "     file in the vault is computed, even if the vault size is larger than the",
        "     database size. This causes -K to return success if e.g. the full file is",
        "     present in the vault, but the catalog thinks the file is empty. Using",
        "     --verify adds an explicit size check after the checksum check.",
        " -n  replNum  - the replica to checksum; use -a to checksum all replicas.",
        "     Return CAT_NO_ROWS_FOUND if the given replica number does not exist.",
        "     If used in combination with -R, the replica number will take precedence.",
        " -R  resource  - the resource of the replica to checksum,",
        "     If used in combination with -n, the replica number will take precedence.",
        " -r  recursive - checksum the whole subtree; the collection, all data-objects",
        "     in the collection, and any subcollections and sub-data-objects in the",
        "     collection.",
        " --silent  - No checksum output except error (in recursive mode, all directory",
        "     names are still listed)",
        " -v  verbose",
        " -V  Very verbose",
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
    printReleaseInfo( "ichksum" );
}
