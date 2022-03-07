/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * iget - The irods get utility
*/

#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/getUtil.h>
#include <irods/rcGlobalExtern.h>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/irods_parse_command_line_options.hpp>

void usage( FILE* );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    rodsPathInp_t rodsPathInp;
    int reconnFlag;


    rodsEnv myEnv;
    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        return 1;
    }

    int p_err = parse_opts_and_paths(
                    argc,
                    argv,
                    myRodsArgs,
                    &myEnv,
                    UNKNOWN_OBJ_T,
                    UNKNOWN_FILE_T,
                    0,
                    &rodsPathInp );
    if ( p_err < 0 ) {
        usage( stderr );
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
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, reconnFlag, &errMsg );

    if ( conn == NULL ) {
        return 2;
    }

    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            return 7;
        }
    }

    if ( myRodsArgs.progressFlag == True ) {
        gGuiProgressCB = ( guiProgressCallback ) iCommandProgStat;
    }

    status = getUtil( &conn, &myEnv, &myRodsArgs, &rodsPathInp );

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
        "Usage: iget [-fIKPQrUvVT] [-n replNumber] [-N numThreads] [-X restartFile]",
        "[-R resource] [--lfrestart lfRestartFile] [--retries count] [--purgec]",
        "[--rlock] srcDataObj|srcCollection ... destLocalFile|destLocalDir",
        " ",
        "Usage: iget [-fIKPQUvVT] [-n replNumber] [-N numThreads] [-X restartFile]",
        "[-R resource] [--lfrestart lfRestartFile] [--retries count] [--purgec]",
        "[--rlock] srcDataObj|srcCollection",
        " ",
        "Usage: iget [-fIKPQUvVT] [-n replNumber] [-N numThreads] [-X restartFile]",
        "[-R resource] [--lfrestart lfRestartFile] [--retries count] [--purgec]",
        "[--rlock] srcDataObj ... -",
        " ",
        "Get data-objects or collections from iRODS space, either to the specified",
        "local area or to the current working directory.",
        " ",
        "Data Objects with multiple replicas in iRODS may have one or more 'stale'",
        "replicas (will not display the & before the filename in ils -l or ils -L).",
        "iRODS will not serve a stale or 'dirty' replica unless asked specifically",
        "(by using the -R option). Without the -R option, a clean replica will be",
        "selected and served to the client.",
        " ",
        "If the destLocalFile is '-', the files read from the server will be ",
        "written to the standard output (stdout). Similar to the UNIX 'cat'",
        "command, multiple source files can be specified.",
        " ",
        "The -X option specifies that the restart option is on and the operation",
        "is restartable. The restartFile input specifies a local file that contains",
        "the restart info. If the restartFile does not exist, it will be created",
        "and used for recording subsequent restart info. If it exists and is not",
        "empty, the restart info contained in this file will be used to restart",
        "the operation. Note that the operation is not restarted automatically",
        "when it failed. Another iget -X run must be made to continue from where",
        "it left off using the restart info. But the -X option can be used in",
        "conjunction with the --retries option to automatically restart the operation",
        "in case of failure. Also note that the restart operation only works for",
        "downloading directories and the path input must be identical to the one",
        "that generated the restart file.",
        " ",
        "The --lfrestart option specifies that the large file restart option is on",
        "and the lfRestartFile input specifies a local file that contains the restart",
        "info.",
        " ",
        "The --lfrestart option can be used together with the -X option to do large",
        "file transfer restart as part of the overall collection download restart.",
        " ",
        "Large files are defined by the 'maximum_size_for_single_buffer_in_megabytes'",
        "advanced configuration setting.",
        " ",
        "The -Q option specifies the use of the RBUDP transfer mechanism which uses",
        "the UDP protocol for data transfer. The UDP protocol is very efficient",
        "if the network is very robust with few packet losses. Two environment",
        "variables - rbudpSendRate and rbudpPackSize are used to tune the RBUDP",
        "data transfer. rbudpSendRate is used to throttle the send rate in ",
        "kbits/sec. The default rbudpSendRate is 600,000. rbudpPackSize is used",
        "to set the packet size. The default rbudpPackSize is 8192. The -V option",
        "can be used to show the loss rate of the transfer. If the lost rate is",
        "more than a few %, the sendrate should be reduced.",
        " ",
        "The -T option will renew the socket connection between the client and ",
        "server after 10 minutes of connection. This gets around the problem of",
        "sockets getting timed out by the firewall as reported by some users.",
        " ",
        "Options are:",

        " -f  force - write local files even it they exist already (overwrite them)",
        " -I  redirect connection - redirect the connection to connect directly",
        "       to the best (determiined by the first 10 data objects in the input",
        "       collection) resource server.",
        " -K  verify the checksum",
        " -n  replNumber - retrieve the copy with the specified replica number ",
        " -N  numThreads - the number of thread to use for the transfer. A value of",
        "       0 means no threading. By default (-N option not used) the server ",
        "       decides the number of threads to use.",
        " --purgec - Purge the staged cache copy after downloading a COMPOUND object",
        " -P  output the progress of the download.",
        " -r  recursive - retrieve subcollections",
        " -R  resource - Specify a resource from which to get the data. If no such",
        "       resource exists or a replica does not exist on the specified resource,",
        "       an error will be returned.",
        " -T  renew socket connection after 10 minutes",
        " -Q  use RBUDP (datagram) protocol for the data transfer",
        " -v  verbose",
        " -V  Very verbose",
        " -X  restartFile - specifies that the restart option is on and the",
        "     restartFile input specifies a local file that contains the restart info.",
        "--retries count - Retry the iget in case of error. The 'count' input",
        "     specifies the number of times to retry. It must be used with the",
        "     -X option",
        " --lfrestart lfRestartFile - specifies that the large file restart option is",
        "      on and the lfRestartFile input specifies a local file that contains",
        "      the restart info.",
        " -t  ticket - ticket (string) to use for ticket-based access.",
        " --rlock - use advisory read lock for the download",
        " --kv_pass - pass quoted key-value strings through to the resource hierarchy,",
        "             of the form key1=value1;key2=value2",
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
    printReleaseInfo( "iget" );
}
