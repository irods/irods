/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * iput - The irods put utility
*/

#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/putUtil.h>
#include <irods/rcGlobalExtern.h>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/irods_parse_command_line_options.hpp>

#include <cstdio>

void usage( FILE* );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    rodsPathInp_t rodsPathInp;
    int reconnFlag;

    rodsEnv myEnv;
    int status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        return status;
    }

    int p_err = parse_opts_and_paths(
                    argc,
                    argv,
                    myRodsArgs,
                    &myEnv,
                    UNKNOWN_FILE_T,
                    UNKNOWN_OBJ_T,
                    0,
                    &rodsPathInp );
    if ( p_err < 0 ) {

        // Issue 4016: The return value SYS_NO_PATH_PERMISSION comes from a
        // boost::filesystem exception being thrown, with a specific reason.
        // The error message has already been displayed on stderr (for example,
        // "... Too many levels of symbolic links: ...".  Adding a usage
        // message which is over 100 lines long in addition, is confusing,
        // and not useful to the user.
        if (p_err != SYS_NO_PATH_PERMISSION)
        {
            usage( stderr );
        }
        return EXIT_FAILURE;

    }
    else if ( myRodsArgs.help ) {
        usage( stdout );
        return EXIT_SUCCESS;
    }

    if ( myRodsArgs.reconnect == True ) {
        reconnFlag = RECONN_TIMEOUT;
    }
    else {
        reconnFlag = NO_RECONN;
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, reconnFlag, &errMsg );

    if ( conn == NULL ) {
        return 2;
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        print_error_stack_to_file(conn->rError, stderr);
        rcDisconnect( conn );
        return 7;
    }

    if ( myRodsArgs.progressFlag == True ) {
        gGuiProgressCB = ( guiProgressCallback ) iCommandProgStat;
    }

    status = putUtil( &conn, &myEnv, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
        return 3;
    }
    else {
        return 0;
    }

}

void
usage( FILE* _fout ) {
    if ( !_fout ) {
        fprintf( stderr, "usage - invalid file pointer\n" );
        return;
    }

    char *msgs[] = {
        "Usage: iput [-abfIkKPQrtTUvV] [-D dataType] [-N numThreads] [-n replNum]",
        "             [-p physicalPath] [-R resource] [-X restartFile] [--ignore-symlinks]",
        "             [--lfrestart lfRestartFile] [--retries count] [--wlock]",
        "             [--purgec] [--kv_pass=key-value-string] [--metadata=avu-string]",
        "             [--acl=acl-string]  localSrcFile|localSrcDir ...  destDataObj|destColl",
        "Usage: iput [-abfIkKPQtTUvV] [-D dataType] [-N numThreads] [-n replNum] ",
        "             [-p physicalPath] [-R resource] [-X restartFile] [--ignore-symlinks]",
        "             [--lfrestart lfRestartFile] [--retries count] [--wlock]",
        "             [--purgec] [--kv_pass=key-value-string] [--metadata=avu-string]",
        "             [--acl=acl-string]  localSrcFile",
        " ",
        "Store a file into iRODS.  If the destination data-object or collection are",
        "not provided, the current iRODS directory and the input file name are used.",
        " ",
        "The -X option specifies that the restart option is on and the operation",
        "is restartable. The restartFile input specifies a local file that contains",
        "the restart information. If the restartFile does not exist, it will be created",
        "and used for recording subsequent restart information. If it exists and is not",
        "empty, the restart information contained in this file will be used to restart",
        "the operation. Note that the operation is not restarted automatically",
        "when it failed. Another iput -X run must be made to continue from where",
        "it left off using the restart information. But the -X option can be used in",
        "conjunction with the --retries option to automatically restart the operation",
        "in case of failure. Also note that the restart operation only works for",
        "uploading directories and the path input must be identical to the one",
        "that generated the restart file.",
        " ",
        "The --lfrestart option specifies that the large file restart option is on",
        "and the lfRestartFile input specifies a local file that contains the restart",
        "information. Currently, only files larger than 32 Mbytes will be restarted.",
        "The --lfrestart option can be used together with the -X option to do large",
        "file transfer restart as part of the overall directory upload restart.",
        " ",
        "If the -f option is used to overwrite an existing data-object, the copy",
        "in the resource specified by the -R option will be picked if it exists.",
        "Otherwise, one of the copies in the other resources will be picked for the",
        "overwrite. Note that a copy will not be made in the specified resource",
        "if a copy in the specified resource does not already exist. The irepl",
        "command should be used to make a replica of an existing copy.",
        " ",
        "The -I option specifies the redirection of the connection so that it can",
        "be connected directly to the resource server. This option can improve",
        "the performance of uploading a large number of small (<32 Mbytes) files.",
        "This option is only effective if the source is a directory and the -f ",
        "option is not used.",
        " ",
        "The -Q option specifies the use of the RBUDP transfer mechanism which uses",
        "the UDP protocol for data transfer. The UDP protocol is very efficient",
        "if the network is very robust with few packet losses. Two environment",
        "variables - rbudpSendRate and rbudpPackSize are used to tune the RBUDP",
        "data transfer. rbudpSendRate is used to throttle the send rate in ",
        "kbits/sec. The default rbudpSendRate is 600,000. rbudpPackSize is used",
        "to set the packet size. The default rbudpPackSize is 8192. The -V option",
        "can be used to show the loss rate of the transfer. If the loss rate is",
        "more than a few %, the rbudpSendRate should be reduced.",
        " ",
        "The -T option will renew the socket connection between the client and ",
        "server after 10 minutes of connection. This gets around the problem of",
        "sockets getting timed out by the server firewall as reported by some users.",
        " ",
        "The -b option specifies the bulk upload operation which can do up to 50 uploads",
        "at a time to reduce overhead. The -b and -f options are incompatible parameters.",
        "To overwrite a collection using bulk upload, the existing collection should be",
        "removed or renamed beforehand.",
        " ",
        "Options are:",
        " -a  all - update all existing copies",
        " -b  bulk upload to reduce overhead",
        " -D  dataType - the data type string",
        " -f  force - write data-object even it exists already; overwrite it",
        " -I  redirect connection - redirect the connection to connect directly",
        "       to the resource server.",
        " -k  checksum - calculate a checksum on the data server-side, and store",
        "       it in the catalog.",
        " -K  verify checksum - calculate and verify the checksum on the data, both",
        "       client-side and server-side, and store it in the catalog.",
        " --link - [Deprecated] ignore symlinks. Use --ignore-symlinks.",
        " --ignore-symlinks - ignore symlinks.",
        " -n  replNum  - the replica to be replaced, typically not needed",
        " -N  numThreads - the number of threads to use for the transfer. A value of",
        "       0 means no threading. By default (-N option not used) the server ",
        "       decides the number of threads to use.",
        " --purgec  Purge the staged cache copy after uploading an object to a",
        "     COMPOUND resource",
        " -p  physicalPath - the absolute physical path of the uploaded file on the server ",
        " -P  output the progress of the upload.",
        " -Q  use RBUDP (datagram) protocol for the data transfer",
        " -R  resource - specifies the resource to store to. This can also be specified",
        "     in your environment or via a rule set up by the administrator.",
        " -r  recursive - store the whole subdirectory",
        " -t  ticket - ticket (string) to use for ticket-based access",
        " -T  renew socket connection after 10 minutes",
        " -v  verbose",
        " -V  Very verbose",
        " -X  restartFile - specifies that the restart option is on and the restartFile",
        "       input specifies a local file that contains the restart information.",
        "--retries count - Retry the iput in case of error. The 'count' input",
        "       specifies the number of times to retry. It must be used with the",
        "       -X option",
        " --lfrestart lfRestartFile - specifies that the large file restart option is",
        "       on and the lfRestartFile input specifies a local file that contains",
        "       the restart information.",
        " --wlock - use advisory write (exclusive) lock for the upload",
        " --kv_pass - pass quoted key-value strings through to the resource hierarchy,",
        "             of the form key1=value1;key2=value2",
        " --metadata - atomically assign metadata after a data object is registered in",
        "              the catalog. Metadata is encoded into a quoted string of the",
        "              form attr1;val1;unit1;attr2;val2;unit2;",
        " --acl - atomically apply ACLs of the form",
        "          'perm user_or_group;perm user_or_group;'",
        "          where 'perm' is defined as null|read|write|own",
        " -h  this help",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        fprintf( _fout, "%s\n", msgs[i] );
    }
    printReleaseInfo( "iput" );
}
