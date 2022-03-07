/*
 * ils - The irods ls utility
*/

#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/lsUtil.h>

#include <irods/irods_buffer_encryption.hpp>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>

#include <string>
#include <iostream>

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

    // -=-=-=-=-=- JMC - backport 4536 -=-=-=-=-=-
    optStr = "hAdrlLvt:VZ";
    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs );
    // -=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    if ( status < 0 ) {
        printf( "Use -h for help\n" );
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

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, NO_INPUT_T, ALLOW_NO_SRC_FLAG, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        usage();
        exit( 1 );
    }

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
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );
    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            exit( 7 );
        }
    }

    status = lsUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
        exit( 4 );
    }
    else {
        exit( 0 );
    }
}

void
usage() {
    char *msgs[] = {
        "Usage: ils [-ArlLv] dataObj|collection ... ",
        "Usage: ils --bundle [-r] dataObj|collection ... ",
        "Display data objects and collections stored in iRODS.",
        " ",
        "The following is typical output for ils with no arguments:",
        "    $ ils",
        "    /tempZone/public:",
        "      big",
        "      foo",
        "The only information displayed are the names of data objects in the current",
        "working collection.",
        " ",
        "The long format (given by -l) will display some more information:",
        "    $ ils -l",
        "    /tempZone/public:",
        "      rods              0 demoResc              41943040 2021-04-29.17:57 & big",
        "      rods              2 comp;arch             40236581 2021-04-19.03:00 X big",
        "      alice             2 repl1;child1               283 2021-04-29.17:54 ? foo",
        "      alice             3 repl1;child2               283 2021-04-29.17:56 ? foo",
        "      alice             4 repl1;child3               283 2021-04-29.17:57 ? foo",
        "This displays replica information and system metadata for the replicas of",
        "the data objects in the current working collection. This information includes",
        "the owner of the data object and the replica's number, resource hierarchy, size,",
        "last modified time, and status.",
        " ",
        "The very long format (given by -L) will display even more information:",
        " ",
        "    $ ils -L",
        "    /tempZone/public:",
        "      rods              0 demoResc              41943040 2021-04-29.17:57 & big",
        "        sha2:gKNyEYjkAhiwiyZ3a8U72ugeR4T/9x1xRQoZcxnLoRM=    generic    /var/lib/irods/Vault/public/big",
        "      rods              2 comp;arch             40236581 2021-04-19.03:00 X big",
        "        sha2:KNyEYjkhiwyZ3a8U72ugeR4T/9x1xRQoZcxnLoRMigg=    generic    /var/lib/irods/archVault/public/big",
        "      alice             2 repl1;child1               283 2021-04-29.17:54 ? foo",
        "            generic    /var/lib/irods/child1vault/public/foo",
        "      alice             3 repl1;child2               283 2021-04-29.17:56 ? foo",
        "            generic    /var/lib/irods/child2vault/public/foo",
        "      alice             4 repl1;child3               283 2021-04-29.17:57 ? foo",
        "            generic    /var/lib/irods/child3vault/public/foo",
        "This displays even more replica information and system metadata such as each ",
        "replica's checksum, data type, and physical location in the storage resource.",
        " ",
        "The different replica statuses are represented by the following characters",
        "which can be seen between the last modified time and the data object name.",
        "   0. X: stale        - The replica is no longer known to be good. Usually",
        "                        indicates that a more recently written-to replica exists.",
        "                        This does not necessarily mean that the data is incorrect",
        "                        but merely that it may not reflect the \"truth\" of this",
        "                        data object.",
        "   1. &: good         - The replica is good. Usually the latest replica to be",
        "                        written. When a replica is good, all bytes and system",
        "                        metadata (size and checksum, if present) are understood",
        "                        to have been recorded correctly.",
        "   2. ?: intermediate - The replica is actively being written to. The state of",
        "                        the replica cannot be determined because the client",
        "                        writing to it still has the replica opened. Replicas",
        "                        which are intermediate cannot be opened for read or",
        "                        write, nor can they be unlinked or renamed.",
        "   3. ?: write-locked - One of this replica's sibling replicas is actively",
        "                        being written to but is itself at rest. Replicas which",
        "                        are write-locked cannot be opened for read or write",
        "                        nor can they be unlinked or renamed.",
        "These replica statuses correspond with the data_is_dirty column in r_data_main.",
        " ",
        "Options are:",
        " -A  ACL (access control list) and inheritance format",
        " -d  List collections themselves, not their contents",
        " -l  long format",
        " -L  very long format",
        " -r  recursive - show subcollections",
        " -t  ticket - use a read (or write) ticket to access collection information",
        " -v  verbose",
        " -V  Very verbose",
        " -h  this help",
        " --bundle - list the subfiles in the bundle file (usually stored in the",
        "     /myZone/bundle collection) created by iphybun command.",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ils" );
}
