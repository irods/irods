/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * icp - The irods cp utility
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "cpUtil.hpp"
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
    int reconnFlag;


    optStr = "QhfkKN:p:PrR:TvVX:";

    status = parseCmdLineOpt( argc, argv, optStr, 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    if ( argc - optind <= 1 ) {
        rodsLog( LOG_ERROR, "icp: no input" );
        printf( "Use -h for help.\n" );
        exit( 2 );
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, UNKNOWN_OBJ_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        printf( "Use -h for help.\n" );
        exit( 1 );
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
        exit( 2 );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
        exit( 7 );
    }

    if ( myRodsArgs.progressFlag == True ) {
        gGuiProgressCB = ( guiProgressCallback ) iCommandProgStat;
    }

    status = cpUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

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
    int i;

    char *msgs[] = {
        "Usage : icp [-fkKPQrTvV] [-N numThreads] [-p physicalPath] [-R resource]",
        "-X restartFile] srcDataObj|srcColl ...  destDataObj|destColl",
        "icp copies an irods data-object (file) or collection (directory) to another",
        "data-object or collection.",
        " ",
        "The -Q option specifies the use of the RBUDP transfer mechanism which uses",
        "the UDP protocol for data transfer. The UDP protocol is very efficient",
        "if the network is very robust with few packet losses. Two environment",
        "variables - rbudpSendRate and rbudpPackSize are used to tune the RBUDP",
        "data transfer. rbudpSendRate is used to throttle the send rate in ",
        "kbits/sec. The default rbudpSendRate is 600,000. rbudpPackSize is used",
        "to set the packet size. The default rbudpPackSize is 8192.",
        " ",
        "The -X option specifies that the restart option is on and the restartFile",
        "input specifies a local file that contains the restart info. If the ",
        "restartFile does not exist, it will be created and used for recording ",
        "subsequent restart info. If it exists and is not empty, the restart info",
        "contained in this file will be used for restarting the operation.",
        "Note that the restart operation only works for uploading directories and",
        "the path input must be identical to the one that generated the restart file",
        " ",
        "The -T option will renew the socket connection between the client and ",
        "server after 10 minutes of connection. This gets around the problem of",
        "sockets getting timed out by the firewall as reported by some users.",
        " ",
        "Options are:",
        " -Q  use RBUDP (datagram) protocol for the data transfer",
        "-f force - write data-object even it exists already; overwrite it",
        "-k checksum - calculate a checksum on the data",
        "-K verify checksum - calculate and verify the checksum on the data",
        "-N number  specifies the number of I/O threads to use, by default a rule",
        "           is used to determine the best value.",
        "-p physicalPath  specifies the path on the storage resource on which to store.",
        "       Normally, you let the irods system automatically determine this.",
        " -P  output the progress of the copy.",
        "-R resource - specifies the resource to store to. This can also be specified",
        "       in your environment or via a rule set up by the administrator.",
        "-r recursive - copy the whole subtree",
        "-T  renew socket connection after 10 minutes",
        "-v verbose - display various messages while processing",
        "-V very verbose",
        " -X  restartFile - specifies that the restart option is on and the",
        "     restartFile input specifies a local file that contains the restart info.",
        " -h  this help",
        ""
    };
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "icp" );
}
