/*** Copyright (c) 2010 Data Intensive Cyberinfrastructure Foundation. All rights reserved.    ***
 *** For full copyright notice please refer to files in the COPYRIGHT directory                ***/
/* Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation */
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/scanUtil.h>
void usage();

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    rodsArguments_t myRodsArgs;
    int status = parseCmdLineOpt( argc, argv, "dhr", 0, &myRodsArgs );

    if ( status < 0 ) {
        printf( "Use -h for help\n" );
        return 1;
    }
    if ( myRodsArgs.help == True ) {
        usage();
        return 0;
    }

    rodsEnv myEnv;
    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        return 1;
    }

    objType_t srcType;
    if ( myRodsArgs.dataObjects ) {
        srcType = UNKNOWN_OBJ_T;
    }
    else {
        srcType = UNKNOWN_FILE_T;
    }

    rodsPathInp_t rodsPathInp;
    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               srcType, NO_INPUT_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        usage();
        return 1;
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    rErrMsg_t errMsg;
    rcComm_t *conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                                myEnv.rodsZone, 0, &errMsg );

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

    char hostname[LONG_NAME_LEN];
    status = gethostname( hostname, LONG_NAME_LEN );
    if ( status < 0 ) {
        printf( "cannot resolve server name, aborting!\n" );
        return 4;
    }

    status = scanObj( conn, &myRodsArgs, &rodsPathInp, hostname );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    return status;

}

void
usage() {
    char *msgs[] = {
        "Usage: iscan [-rhd] srcPhysicalFile|srcPhysicalDirectory|srcDataObj|srcCollection",
        " ",
        "If the input is a local data file or a local directory, iscan",
        "checks if the content is registered in iRODS.",
        " ",
        "Full path must be used for local files and directories.",
        " ",
        "If the input is an iRODS file or an iRODS collection, iscan",
        "checks if the physical files corresponding to the iRODS object",
        "exists on the data servers. Scanning data objects and",
        "collections may only be performed by a rodsadmin.",
        " ",
        "If the operation is successful, nothing will be output",
        "   and 0 will be returned.",
        " ",
        "Options are:",
        " -r  recursive - scan local subdirectories or subcollections",
        " -h  this help",
        " -d  scan data objects in iRODS (default is scan local objects)",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "iscan" );
}
