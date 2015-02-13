/*** Copyright (c), The Regents of the University of California            ***
 *** For more infophybunation please refer to files in the COPYRIGHT directory ***/
/*
 * iphybun - The irods phybun utility
*/
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "phybunUtil.hpp"
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


    optStr = "hD:N:KR:S:s:"; // JMC - backport 4528, 4658, 4771

    status = parseCmdLineOpt( argc, argv, optStr, 0, &myRodsArgs );
    if ( status < 0 ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    if ( argc - optind <= 0 ) {
        rodsLog( LOG_ERROR, "iphybun: no input" );
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

    status = phybunUtil( conn, &myRodsArgs, &rodsPathInp );

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
        "Usage : iphybun [-hK] [-D dataType] [-S srcResource] [-R resource] [-s maxSize_in_GB] [-N numOfSubFiles] collection ... ",
        "iphybun allows system admin to physically bundle files in a collection into",
        "a number of tar files to make it more efficient to store these files on tape.",
        "The tar files are placed into the /myZone/bundle/.... collection with file",
        "names - collection.aRandomNumber. A new tar file will be created whenever",
        "the number of subfiles exceeds 5120 (default value or the value given by",
        "the -N option) or the total size of the subfiles exceeds 4 GBytes (default value or the value given by",
        "the -s option).",
        "A replica is registered for each bundled sub-files with",
        "a fictitious resource - 'bundleResc' and a physical file path pointing to",
        "the logical path of the tar bundle file. Whenever this copy of the subfile",
        "is accessed, the tar file is untarred and staged automatically to the ",
        "'cache' resource. Each extracted file is registered as a replica of its",
        "corresponding subfiles.",
        "",
        "A tar bundle file can be replicated or trimmed independently from its",
        "corresponding subfiles. But it cannot be renamed or moved to trash.",
        "It can be removed with the 'irm -f' command. But this  will trigger the ",
        "staging of the subfiles before the tar file is removed.",
        "The -R flag specifies the resource of the bundle tar file. This input is",
        "mandatory. The input resource must be a 'cache' type resource.",
        "",
        "Options are:",
        " -D  dataType - the target struct file dataType. Valid dataTypes are -",
        "     t|tar|'tar file' for tar file, g|gzip|gzipTar for gziped tar file,",
        "     b|bzip2|bzip2Tar for bzip2 file, and z|zip|zipFile for archive using",
        "     'zip'.  If -D is not specified, the default is a tar file type",
        " -N  numOfSubFiles - maximum number of subfiles that are contained in the",
        "     tar file. If this option is not given, the default value will be 5120.",
        "     Note that if this number is too high, it can cause some significant",
        "     overhead for operations like retrieving a single file within a tar file",
        "     (stage, untar and register in iRODS lots of files).",
        " -R  resource - The resource where the bundle file is located",
        " -S  srcResource - Only files in this resource will be bundled",
        " -K  compute and register checksum value for the bundled subfiles and the",
        "     bundle file.",
        " -s  maxSize_in_GB - maximum size for the tar bundle file. This is given ",
        "     in GB. If the option is not given, the default value will be 4 GBs.",
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
    printReleaseInfo( "iphybun" );
}

