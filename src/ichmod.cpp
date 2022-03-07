/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * ichmod - The irods chmod utility
*/

#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>


void usage();

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );


    rodsArguments_t myRodsArgs;
    int status = parseCmdLineOpt( argc, argv, "RrhvVM", 0, &myRodsArgs );
    if ( status ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLineOpt error. " );
        printf( "Use -h for help\n" );
        return 1;
    }
    if ( myRodsArgs.help == True ) {
        usage();
        return 0;
    }

    int nArgs = argc - myRodsArgs.optind;

    if ( nArgs < 2 ) {
        usage();
        return 2;
    }

    rodsEnv myEnv;
    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        return 3;
    }

    bool doingInherit = !strcmp( argv[myRodsArgs.optind], ACCESS_INHERIT ) ||
                        !strcmp( argv[myRodsArgs.optind], ACCESS_NO_INHERIT );
    int optind = doingInherit ? myRodsArgs.optind + 1 : myRodsArgs.optind + 2;

    rodsPathInp_t rodsPathInp;
    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, NO_INPUT_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        usage();
        return 4;
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    rErrMsg_t errMsg;
    rcComm_t * conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                                 myEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
        return 5;
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
        return 6;
    }

    modAccessControlInp_t modAccessControl;
    modAccessControl.recursiveFlag = myRodsArgs.recursive;
    modAccessControl.accessLevel = argv[myRodsArgs.optind];

    char zoneName[NAME_LEN];
    char userName[NAME_LEN];
    if ( doingInherit ) {
        modAccessControl.userName = "";
        modAccessControl.zone = "";
    }
    else {
        status = parseUserName( argv[myRodsArgs.optind + 1], userName, zoneName );
        if ( status != 0 ) {
            printf( "Invalid iRODS user name format: %s\n",
                    argv[myRodsArgs.optind + 1] );
            rcDisconnect( conn );
            return 7;
        }
        modAccessControl.userName = userName;
        modAccessControl.zone = zoneName;
    }
    for ( int i = 0; i < rodsPathInp.numSrc && status == 0; i++ ) {
        if ( rodsPathInp.numSrc > 1 && myRodsArgs.verbose != 0 ) {
            printf( "path %s\n", rodsPathInp.srcPath[i].outPath );
        }
        modAccessControl.path = rodsPathInp.srcPath[i].outPath;

        char rescAccessLevel[LONG_NAME_LEN];
        if ( myRodsArgs.resource ) {
            snprintf( rescAccessLevel, sizeof( rescAccessLevel ), "%s%s", MOD_RESC_PREFIX, argv[myRodsArgs.optind] );
            modAccessControl.accessLevel = rescAccessLevel; /* indicate resource*/
            modAccessControl.path = argv[optind]; /* just use the plain name */
        }
        char adminModeAccessLevel[LONG_NAME_LEN];
        if ( myRodsArgs.admin && i == 0 ) {  /* admin mode, add indicator */
            snprintf( adminModeAccessLevel, sizeof( adminModeAccessLevel ), "%s%s", MOD_ADMIN_MODE_PREFIX, modAccessControl.accessLevel );
            modAccessControl.accessLevel = adminModeAccessLevel;
        }
        status = rcModAccessControl( conn, &modAccessControl );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status, "rcModAccessControl failure %s",
                          errMsg.msg );
            if ( const rError_t *Err = conn->rError ) {
                int len = Err->len;
                for ( int j = 0; j < len; j++ ) {
                    rodsLog( LOG_ERROR, "Level %d: %s", j, Err->errMsg[j]->msg );
                }
            }
        }
    }

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
        return 8;
    }
    return 0;

}

void
usage() {
    char *msgs[] = {
        "Usage: ichmod [-rhvVM] null|read|write|own userOrGroup dataObj|Collection ...",
        " or    ichmod [-rhvVM] inherit Collection ...",
        " or    ichmod [-rhvVM] noinherit Collection ...",
        " -r  recursive - set the access level for all dataObjects",
        "             in the entered collection and subCollections under it",
        " -v  verbose",
        " -V  Very verbose",
        " -M  Admin Mode",
        " -h  this help",
        " ",
        "Modify access to dataObjects (iRODS files) and collections (directories).",
        " ",
        "When you store a file, you are the owner and have full control - you can",
        "read, write or delete it and, by default, no one else can.",
        "With chmod you can give access to other users or groups, either",
        "just read access, or read and write, or full ownership.",
        "You can only give (or remove) access to others if you own",
        "the file yourself.  But if you give 'own' to someone else, they",
        "can also give (and remove) access to others.",
        " ",
        "You can remove access by changing the access to 'null'.",
        " ",
        "Multiple paths can be entered on the command line.",
        " ",
        "If the entered path is a collection, then the access permissions to",
        "that collection will be modified.  For example, you can give write access",
        "to a user or group so they can store files into one of your collections.",
        "Access permissions on collections are not currently displayed via ils.",
        " ",
        "The inherit/noinherit form sets or clears the inheritance attribute of",
        "one or more collections.  When collections have this attribute set,",
        "new dataObjects and collections added to the collection inherit the",
        "access permisions (ACLs) of the collection.  'ils -A' displays ACLs",
        "and the inheritance status.",
        " ",
        "If you are the iRODS administrator, you can include the -M option to",
        "run in administrator mode and set the permissions on the collection(s)",
        "and/or data-objects as if you were the owner.  This is more convenient",
        "than aliasing as the user.",
        " ",
        "Like Unix, if you remove your own access to an object ('null'), you will",
        "not be able to read it, but you can restore access via another 'ichmod'",
        "because for ichmod, you are still the owner.",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ichmod" );
}
