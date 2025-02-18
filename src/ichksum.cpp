#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/chksumUtil.h>
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
    char* optStr;

    optStr = "hKfarMR:vVn:Z";

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

    rodsPathInp_t rodsPathInp{};
    irods::at_scope_exit freePath{[&rodsPathInp] { freeRodsPathInpMembers(&rodsPathInp); }};
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
        print_error_stack_to_file(conn->rError, stderr);
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
        "Usage: ichksum [-haMrvV] [-f|K|--verify] [-n replNum|-R resource] [--silent]",
        "           dataObj|collection ... ",
        " ",
        "Checksum one or more data objects or collections.",
        " ",
        "The checksum algorithm used by this function is determined by the client and server's",
        "configuration, but defaults to SHA256. Checksum values are only calculated on",
        "the part of a replica indicated by the size stored in the catalog, not necessarily",
        "the entirety of the file under management.",
        " ",
        "There are two modes of operation:",
        " - Lookup/Update (default): Computes and updates checksum information.",
        " - Verification: Detects and reports issues related to checksum information.",
        " ",
        "In Lookup/Update mode, the following operations are performed:",
        " 1. Compute a new checksum and updates the catalog.",
        " 2. Reports if the replicas do NOT share identical checksums.",
        " ",
        "Lookup/Update mode will process the highest voted replica unless -a is present. If -f",
        "is not present and the replica already has a checksum, that checksum will be printed",
        "to the terminal. If a specific replica is targeted, step 2 is not performed.",
        " ",
        "Lookup/Update mode supports operations on replicas marked as good or stale.",
        " ",
        "In Verification mode, the following operations are performed:",
        " 1. Reports replicas with mismatched size information (physical vs catalog).",
        " 2. Reports replicas with missing checksums. (Halts if missing checksums are found)",
        " 3. Reports replicas with mismatched checksums (computed vs catalog).",
        " 4. Reports if the replicas do NOT share identical checksums.",
        " ",
        "Verification mode operates on all replicas unless a specific replica is targeted. If a",
        "specific replica is targeted, step 4 is not performed.",
        " ",
        "Verification mode supports operations on replicas marked as good.",
        " ",
        "Operations that target multiple replicas will only affect replicas that are marked as",
        "good. This means intermediate, locked, and stale replicas will be ignored.",
        " ",
        "Operations that target a specific replica are allowed to operate on stale replicas",
        "unless stated otherwise.",
        " ",
        "Options:",
        "-f        Computes and stores a checksum for one or more replicas. This option always",
        "          results in a catalog update.",
        "-a        Apply operation to all good replicas.",
        "-K, --verify",
        "          Verifies the state of the checksums for one or more replicas.",
        "--no-compute",
        "          When used with -K, no checksums are computed. This option causes step 3",
        "          of -K to be skipped. This option is provided as a way to avoid long running",
        "          checksum computations when a size check is adequate.",
        "-M        Run the command as an administrator.",
        "-n REPLICA_NUMBER",
        "          The replica number of the replica to checksum or verify.",
        "-R RESOURCE_NAME",
        "          The leaf resource that contains the replica to checksum or verify.",
        "-r        Checksum data objects recursively.",
        "--silent  Suppresses output of checksums and output related to -r.",
        "-v        Verbose.",
        "-V        Very verbose.",
        "-h        Prints this message.",
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
