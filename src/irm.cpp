/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * irm - The irods rm utility
 */

#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/rmUtil.h>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>

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


    optStr = "hfruvVf:Z"; // JMC - backport 4552

    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs ); // JMC - backport 4552
    if ( status < 0 ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    if ( argc - optind <= 0 ) {
        rodsLog( LOG_ERROR, "irm: no input" );
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
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
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

    status = rmUtil( conn, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( ( USER_SOCK_CONNECT_ERR - 1000 ) < status && status <= USER_SOCK_CONNECT_ERR ) {
        printf( "Remote resource may be unavailable.\n" );
    }

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
        "Usage: irm [-rUfvVh] [-n replNum] [--empty] dataObj|collection ... ", // JMC - backport 4552
        " ",
        "Remove one or more data objects and/or collections from the iRODS namespace. ",
        "By default, the data objects are moved to the trash collection (/myZone/trash) unless",
        "the -f option is used.",
        " ",
        "Registered non-vault replicas are never deleted from the filesystem. They ",
        "are unregistered and left on disk as-is.",
        " ",
        "To remove a specific replica, use itrim.",
        " ",
        "To unregister a data object without removing the physical data, use iunreg.",
        " ",
        "The irmtrash command should be used to delete data objects in the trash",
        "collection.",
        " ",
        "Options are:",
        " -f  force - Immediate removal of data objects without putting them in trash.",
        " -r  recursive - Recursively remove the target collection, all data objects in the ",
        "                 collection, and all subcollections.",
        " -v  verbose",
        " -V  Very verbose",
        " --empty  If the file to be removed is a bundle file (generated with iphybun)", // JMC - backport 4552
        "     remove it only if all the subfiles of the bundle have been removed.",
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
    printReleaseInfo( "irm" );
}

