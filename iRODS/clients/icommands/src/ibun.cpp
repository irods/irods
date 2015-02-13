/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * ibun - bundle files operation
*/
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "bunUtil.hpp"
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


    optStr = "bhR:cxD:fZ"; // JMC - backport 4643

    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs ); // JMC - backport 4643

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

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, UNKNOWN_OBJ_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        printf( "use -h for help.\n" );
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
        rcDisconnect( conn );
        exit( 7 );
    }

    status = bunUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

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
        "Usage : ibun -x [-hfb] [-R resource] structFilePath",
        "               irodsCollection",
        "Usage : ibun -c [-hf] [-R resource] [-D dataType] structFilePath",
        "               irodsCollection",
        "Usage : ibun --add [-h] structFilePath irodsCollection",

        " ",
        "Bundle file operations. This command allows structured files such as ",
        "tar files to be uploaded and downloaded to/from iRODS.",
        " ",
        "A tar file containing many small files can be created with normal unix",
        "tar command on the client and then uploaded to the iRODS server as a",
        "normal iRODS file. The 'ibun -x' command can then be used to extract/untar",
        "the uploaded tar file. The extracted subfiles and subdirectories will",
        "appeared as normal iRODS files and sub-collections. The 'ibun -c' command",
        "can be used to tar/bundle an iRODS collection into a tar file.",
        " ",
        "For example, to upload a directory mydir to iRODS:",
        " ",
        "    tar -chlf mydir.tar -C /x/y/z/mydir .",
        "    iput -Dtar mydir.tar .",
        "    ibun -x mydir.tar mydir",
        " ",
        "Note the use of -C option with the tar command which will tar the",
        "content of mydir but without including the directory mydir in the paths.",
        "The 'ibun -x' command extracts the tar file into the mydir collection.",
        "The target mydir collection does not have to exist nor be empty.",
        "If a subfile already exists in the target collection, the ingestion",
        "of this subfile will fail (unless the -f flag is set) but the process",
        "will continue.",
        " ",
        "It is generally a good practice to tag the tar file using the -Dtar flag",
        "when uploading the file using iput. But if the tag is not made,",
        "the server assumes it is a tar dataType. The dataType tag can be added",
        "afterward with the isysmeta command. For example:",
        "    isysmeta mod /tempZone/home/rods/mydir.tar datatype 'tar file'",
        " ",
        "The following command bundles the iRods collection mydir into a tar file:",
        " ",
        "    ibun -cDtar mydir1.tar mydir",
        " ",
        "If a copy of a file to be bundled does not exist on the target resource,",
        "a replica will automatically be made on the target resource.",
        "Again, if the -D flag is not used, the bundling will be done using tar.",
        " ",
        "The -b option when used with the -x option, specifies bulk registration",
        "which does up to 50 registrations at a time to reduce overhead.",
        " ",
        "Options are:",
        " -b  bulk registration when used with -x to reduce overhead",
        " -R  resource - specifies the resource to store to. This is optional",
        "     in your environment",
        " -D  dataType - the structFile data type. Valid only for -c option for",
        "     specifying the target data type. Valid dataTypes are - t|tar|'tar file'",
        "     for tar file. g|gzip|gzipTar for gzipped tar file, b|bzip2|bzip2Tar for",
        "     bzip2 file, and z|zip|zipFile for an archive using 'zip'.  If -D is not",
        "     specified, the default is the tar dataType",
        " -x  extract the structFile and register the extracted files and directories",
        "     under the input irodsCollection",
        " -c  bundle the files and sub-collection underneath the input irodsCollection",
        "     and store it in the structFilePath",
        " -f  force overwrite the structFile (-c) or the subfiles (-x).",
        " --add  add or append to existing structFile.",
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
    printReleaseInfo( "ibun" );
}
