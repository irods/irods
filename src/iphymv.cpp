/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * iphymv - The irods physical move utility
 */

#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/phymvUtil.h>
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


    optStr = "hMrvVp:n:R:S:";

    status = parseCmdLineOpt( argc, argv, optStr, 0, &myRodsArgs );

    if ( status < 0 ) {
        printf( "Use -h for help.\n" );
        return 1;
    }

    if ( myRodsArgs.help == True ) {
        usage();
        return 0;
    }

    if ( argc - optind <= 0 ) {
        rodsLog( LOG_ERROR, "iphymv: no input" );
        printf( "Use -h for help.\n" );
        return 1;
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        return 1;
    }

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, NO_INPUT_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        printf( "Use -h for help.\n" );
        return 1;
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 1, &errMsg );

    if ( conn == NULL ) {
        // Failed to connect to the server
        return 2;
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        // Failed to authenticate as the configured user
        rcDisconnect( conn );
        return 3;
    }

    status = phymvUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    // The following checks various error codes and returns different exit codes
    // so that the caller can take actions based on said codes (e.g. examine
    // error code for feasibility of a retry).
    const auto irods_error = getIrodsErrno(status);
    switch (irods_error) {
        case USER_CHKSUM_MISMATCH:
            // The checksum on the destination replica is wrong, indicating corruption.
            return 4;

        case USER_INPUT_PATH_ERR:
            // The specified logical path does not exist, indicating a user error.
            return 5;

        case UNIX_FILE_CREATE_ERR:
        case UNIX_FILE_OPEN_ERR:
        case UNIX_FILE_WRITE_ERR:
            // Return a special error code if there is no space left on the device.
            // All other UNIX filesystem-related errors get a separate, but still special error code.
            return ENOSPC == getErrno(status) ? 6 : 7;

        default:
            break;
    }

    // For any other errors, return 3 as has been done historically for minimal surprises.
    return status < 0 ? 3 : 0;
} // main

void
usage() {

    char *msgs[] = {
        "Usage: iphymv [-hMrvV] [-n replNum] [-S srcResource] [-R destResource] ",
        "dataObj|collection ... ",
        " ",
        "Physically move a file in iRODS to another storage resource.",
        " ",
        "The source replica must be specified by its full hierarchy and it must",
        "exist at the provided resource hierarchy location. The resource hierarchy",
        "for the destination replica must also be specified in full, but may or may",
        "not exist at the specified location. Specifying a leaf resource is acceptable",
        "in lieu of a full resource hierarchy as it can be reverse-resolved to the root.",
        "See these example usages:",
        "  iphymv -S demoResc -R 'pt1;r1;s1' /tempZone/home/alice/test.txt",
        "  iphymv -S demoResc -R 's1' /tempZone/home/alice/test.txt",
        " ",
        "If the leaf resource targeted for a phymv refers to the archive of a compound",
        "resource hierarchy, DIRECT_ARCHIVE_ACCESS will be returned because direct access",
        "to an archive resource is not allowed.",
        " ",
        "iphymv cannot overwrite a replica which is not marked stale.",
        "iphymv cannot move a replica which is not at rest.",
        "iphymv cannot move a replica which is part of an already locked data object.",
        " ",
        "iphymv returns a few unique error codes based on the type of error which",
        "occurred within the system:",
        "  0: Success",
        "  1: There was a problem with the client configuration or parsing the user input",
        "  2: There was a problem connecting to the iRODS server (rcConnect failed)",
        "  3: Historic exit code returned for all errors other than those listed above and below",
        "  4: There was a mismatch between the source and destination replica's checksums",
        "  5: There was a problem with the supplied source logical path",
        "  6: The resource to host the destination replica has run out of space (ENOSPC)",
        "  7: The source or destination replica failed to be opened for any other reason",
        " ",
        "Note that if the source copy has a checksum value associated with it,",
        "a checksum will be computed for the replicated copy and compared with",
        "the source value for verification.",
        " ",
        "Options are:",
        " -r  recursive - phymove the whole subtree",
        " -M  admin - admin user uses this option to phymove other users files",
        " -n  replNum  - the replica to be phymoved, typically not needed",
        " -S  srcResource - specifies the source resource for the move.",
        "     If specified, the replica stored on this resource will be moved.",
        " -R  destResource - specifies the destination resource for the move.",
        "     This can also be specified, in your environment or via a rule",
        "     set up by the administrator.",
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
    printReleaseInfo( "iphymv" );
}
