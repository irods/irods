/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * ireg - The irods reg utility
 */

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "regUtil.hpp"
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
    int nArgv;


    optStr = "D:fhkKCG:R:vVZ";

    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs );

    if ( status < 0 ) {
        printf( "use -h for help.\n" );
        exit( 1 );
    }

    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    nArgv = argc - optind;

    if ( nArgv != 2 ) {    /* must have 2 inputs */
        usage();
        exit( 1 );
    }

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    if ( ( *argv[optind] != '/' && strcmp( argv[optind], UNMOUNT_STR ) != 0 ) ||
            *argv[optind + 1] != '/' ) {
        rodsLog( LOG_ERROR,
                 "Input path must be absolute" );
        exit( 1 );
    }

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_FILE_T, UNKNOWN_OBJ_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        printf( "use -h for help.\n" );
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

    status = regUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

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
        "Usage : ireg [-hfCkKvV] [--repl] [-D dataType] [-R resource]",
        "               physicalFilePath, irodsPath",
        " ",
        "Register a file or a directory of files and subdirectory into iRODS.",
        "The file or the directory of files must already exist on the server where",
        "the resource is located.  The full path must be supplied for both the",
        "physicalFilePath and the irodsPath.",
        " ",
        "With the -C option, the entire content beneath the physicalFilePath",
        "(files and subdirectories) will be recursively registered beneath the",
        "irodsPath. For example, the command:",
        " ",
        "    ireg -C /tmp/src1 /tempZone/home/myUser/src1",
        " ",
        "grafts all files and subdirectories beneath the directory /tmp/src1 to",
        "the collection /tempZone/home/myUser/src1",
        " ",
        "By default, only rodsAdmin users are allowed to register files or directories.",
        "More permissive registration policies can be set in the server's rule base",
        "using the acNoChkFilePathPerm() rule.",
        " ",
        "It is considered best practice for the target resource be a single local",
        "storage resource.  A side effect of not following this best practice could be",
        "unwanted extra replicas of a mounted collection or other structured file.",
        "Registered files should be well-understood by the administrator so the line",
        "between iRODS-managed data and user-managed data can be properly maintained.",
        " ",
        "Options are:",
        " -R  resource - specifies the target storage resource. This can also be specified",
        "     in your environment or via a rule set up by the administrator.",
        " -C  the specified path is a directory. The default assumes the path is a file.",
        " -f  Force. If the target collection already exists, register the files and",
        "     subdirectories that have not already been registered in the directory.",
        " -k  calculate a checksum on the iRODS client and store with the file details.",
        " -K  calculate a checksum on the iRODS server and store with the file details.",
        " --repl  register the physical path as a replica.",
        " --exclude-from filename - don't register files matching patterns contained",
        "     in the specified file. The file must be readable on the server where the",
        "     resource is located and must be an absolute pathname.",
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
    printReleaseInfo( "ireg" );
}
