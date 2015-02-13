/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * itrim - The irods replica trimming utility
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "trimUtil.hpp"
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


    optStr = "hMrvVn:N:S:Z";

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
        rodsLog( LOG_ERROR, "itrim: no input" );
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

    status = trimUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

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
        "Usage : itrim [-hMrvV] [--age age_in_minutes] [--dryrun] [-n replNum]|[-S srcResource] [-N numCopies] dataObj|collection ... ",
        " ",
        "Reduce the number of replicas of a dataObject in iRODS by deleting some replicas.",
        "Nothing will be done if this is less than or equal to numCopies. The -n and",
        "the -S options are used to specified which copies to delete. If neither of",
        "these options are used, the replicas will be trimmed until there are",
        "numCopies left. The old copies will be trimmed first, then the 'cache'",
        "class copies.",
        " ",
        "Options are:",
        " -M  admin - admin user uses this option to trim other users files",
        " -n  replNum  - the replica number of the replica to be deleted",
        " -N  numCopies - minimum number of most current copies to keep. The default",
        "     value is 2.",
        " -r  recursive - trim the whole subtree",
        " -S  srcResource - specifies the resource of the replica to be deleted.",
        "     If specified, only copies stored in this resource will be candidate",
        "     for the deletion.",
        " -v  verbose",
        " -V  Very verbose",
        "--age age_in_minutes - The minimum age of the copy in minutes for trimming.",
        "     i.e., a copy will not be trimmed if its age is less.",
        "--dryrun - Do a dry run. No copy will atually be trimmed.",
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
    printReleaseInfo( "itrim" );
}
