#include "utility.hpp"
#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/trimUtil.h>
#include <irods/rmUtil.h>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>

#include <cstdio>

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
        rodsLog( LOG_ERROR, "iunreg: no input" );
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

    status = utils::authenticate_client(conn, myEnv);
    if ( status != 0 ) {
        print_error_stack_to_file(conn->rError, stderr);
        rcDisconnect( conn );
        exit( 7 );
    }

    const auto replica_targeted = [&myRodsArgs]()
    {
        return True == myRodsArgs.replNum ||
               True == myRodsArgs.number ||
               True == myRodsArgs.srcResc ||
               True == myRodsArgs.age;
    }();

    myRodsArgs.unmount = True;

    if (replica_targeted) {
        status = trimUtil(conn, &myEnv, &myRodsArgs, &rodsPathInp);
    }
    else {
        status = rmUtil(conn, &myRodsArgs, &rodsPathInp);
    }

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
        "Usage: iunreg [-hMrvV] dataObj|collection ... ",
        " ",
        "Unregister all replicas of one or more data-object or collection from iRODS space.",
        "All replicas will remain on disk but the entry in the catalog will be removed.",
        "This usage replaces the previously supported -U option for irm.",
        " ",
        "Usage: iunreg [-hMrvV] [--age age_in_minutes] [--dryrun] [-n replNum]|[-S srcResource] [-N numCopies] dataObj|collection ... ",
        " ",
        "Reduce the number of replicas of a dataObject in iRODS by unregistering some replicas.",
        "Nothing will be done if this is less than or equal to numCopies. The -n and",
        "the -S options are used to specify which replicas to unregister.",
        " ",
        "Note that if both are used at the same time, the server will return an error",
        "if -n violates the requirements of -N.",
        " ",
        "Note that -S and -n are incompatible.",
        " ",
        "Options are:",
        " -M  admin - admin user uses this option to unregister other users files",
        " -n  replNum - the replica number of the replica to be unregistered",
        " -N  numCopies - minimum number of most current replicas to keep. The default",
        "     value is 2.",
        " -r  recursive - unregister all data objects in the whole subtree",
        " -S  srcResource - specifies the resource of the replica to be unregistered.",
        "     If specified, only replicas stored in this resource will be candidate",
        "     for the unregistration.",
        " -v  verbose",
        " -V  Very verbose",
        "--age age_in_minutes - The minimum age of the replica in minutes for unregistering.",
        "     i.e., a replica will not be unregistered if its age is less.",
        "--dryrun - Do a dry run. No replica will actually be unregistered.",
        "       NOTE: Option only valid when targeting specific replicas.",
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
    printReleaseInfo( "iunreg" );
}
