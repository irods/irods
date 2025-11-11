#include "utility.hpp"
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/scanUtil.h>

#include <cstdio>

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
        status = utils::authenticate_client(conn, myEnv);
        if ( status != 0 ) {
            print_error_stack_to_file(conn->rError, stderr);
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
        "If the input is a local physical data file or a local directory,",
        "iscan checks if the named content is registered in iRODS.",
        "A full physical path must be used for local files and directories.",
        " ",
        "If the input is a logical path representing an iRODS data object",
        "or an iRODS collection, iscan checks if the associated registered",
        "physical file entries exist on the data servers.",
        "Scanning data objects and collections may only be performed by a rodsadmin.",
        " ",
        "If the operation is successful, nothing will be output",
        "and 0 will be returned.",
        " ",
        "Options are:",
        " -r  recursive - scan local subdirectories or logical subcollections",
        " -h  this help",
        " -d  scan data objects in iRODS (default is scan local files/directories)",
        " ",
        "Examples:",
        "# scan a single physical path to check its presence in the catalog",
        "$ iscan /var/lib/irods/not_in_catalog",
        "/var/lib/irods/not_in_catalog is not registered in iRODS",
        " ",
        "# recursively scan a physical path to check for unregistered local files",
        "$ iscan -r /var/lib/irods/Vault/home/rods/scanned",
        "/var/lib/irods/Vault/home/rods/scanned/does_not_exist2 is not registered in iRODS",
        "/var/lib/irods/Vault/home/rods/scanned/does_not_exist1 is not registered in iRODS",
        " ",
        "# recursively scan a physical path to check for unregistered local files",
        "$ iscan -r /data/projects/myproject",
        "remote addresses: 127.0.0.1 ERROR: scanObj: /data/projects/myproject does not exist",
        " ",
        "# check that a single relative logical path exists in the catalog",
        "$ iscan -d not_a_dataobject",
        "Could not find the requested data object or collection in iRODS.",
        " ",
        "# recursively scan a relative logical path for missing physical paths",
        "$ iscan -rd scanme",
        "Physical file /var/lib/irods/Vault/home/rods/scanme/does_not_exist on server irods.example.org is missing, corresponding to iRODS object /tempZone/home/rods/scanme/does_not_exist",
        "Physical file /var/lib/irods/Vault/home/rods/scanme/missing on server irods.example.org is missing, corresponding to iRODS object /tempZone/home/rods/scanme/missing",
        " ",
        "# rodsadmin required to scan logical paths",
        "$ iscan -d test.txt",
        "User must be a rodsadmin to scan iRODS data objects.",
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
