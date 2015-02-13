/*** Copyright (c), The Regents of the University of California            ***
 *** For more infomation please refer to files in the COPYRIGHT directory ***/
/*
 * ichmod - The irods chmod utility
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
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
    int i, nArgs;
    modAccessControlInp_t modAccessControl;
    char userName[NAME_LEN];
    char zoneName[NAME_LEN];
    int doingInherit;
    char rescAccessLevel[LONG_NAME_LEN];
    char adminModeAccessLevel[LONG_NAME_LEN];

    optStr = "RrhvVM";

    status = parseCmdLineOpt( argc, argv, optStr, 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLineOpt error. " );
        printf( "Use -h for help\n" );
        exit( 2 );
    }

    nArgs = argc - myRodsArgs.optind;

    if ( nArgs < 2 ) {
        usage();
        exit( 3 );
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 3 );
    }

    optind = myRodsArgs.optind + 2;
    doingInherit = 0;
    if ( strcmp( argv[myRodsArgs.optind], ACCESS_INHERIT ) == 0 ||
            strcmp( argv[myRodsArgs.optind], ACCESS_NO_INHERIT ) == 0 ) {
        doingInherit = 1;
        optind = myRodsArgs.optind + 1;
    }

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, NO_INPUT_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        usage();
        exit( 4 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
        exit( 5 );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
        exit( 6 );
    }

    modAccessControl.recursiveFlag = myRodsArgs.recursive;
    modAccessControl.accessLevel = argv[myRodsArgs.optind];

    if ( doingInherit ) {
        modAccessControl.userName = "";
        modAccessControl.zone = "";
    }
    else {
        status = parseUserName( argv[myRodsArgs.optind + 1], userName, zoneName );
        if ( status != 0 ) {
            printf( "Invalid iRODS user name format: %s\n",
                    argv[myRodsArgs.optind + 1] );
            exit( 7 );
        }
    }
    modAccessControl.userName = userName;
    modAccessControl.zone = zoneName;
    for ( i = 0; i < rodsPathInp.numSrc && status == 0; i++ ) {
        if ( rodsPathInp.numSrc > 1 && myRodsArgs.verbose != 0 ) {
            printf( "path %s\n", rodsPathInp.srcPath[i].outPath );
        }
        modAccessControl.path = rodsPathInp.srcPath[i].outPath;

        if ( myRodsArgs.resource ) {
            strncpy( rescAccessLevel, MOD_RESC_PREFIX, LONG_NAME_LEN );
            strncat( rescAccessLevel, argv[myRodsArgs.optind], LONG_NAME_LEN - strlen( rescAccessLevel ) );
            modAccessControl.accessLevel = rescAccessLevel; /* indicate resource*/
            modAccessControl.path = argv[optind]; /* just use the plain name */
        }
        if ( myRodsArgs.admin && i == 0 ) {  /* admin mode, add indicator */
            strncpy( adminModeAccessLevel, MOD_ADMIN_MODE_PREFIX, LONG_NAME_LEN );
            strncat( adminModeAccessLevel, modAccessControl.accessLevel,
                     LONG_NAME_LEN - strlen( adminModeAccessLevel ) );
            modAccessControl.accessLevel = adminModeAccessLevel;
        }
        status = rcModAccessControl( conn, &modAccessControl );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status, "rcModAccessControl failure %s",
                          errMsg.msg );
            if ( conn->rError ) {
                rError_t *Err;
                rErrMsg_t *ErrMsg;
                int i, len;
                Err = conn->rError;
                len = Err->len;
                for ( i = 0; i < len; i++ ) {
                    ErrMsg = Err->errMsg[i];
                    rodsLog( LOG_ERROR, "Level %d: %s", i, ErrMsg->msg );
                }
            }
        }
    }

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
        exit( 8 );
    }
    else {
        exit( 0 );
    }

}

void
usage() {
    char *msgs[] = {
        "Usage: ichmod [-rhvVM] null|read|write|own userOrGroup dataObj|Collection ...",
        " or    ichmod [-rhvVM] inherit Collection ...",
        " or    ichmod [-rhvVM] noinherit Collection ...",
        " or    ichmod [-R] null|read|write|own userOrGroup DBResource",
        " -r  recursive - set the access level for all dataObjects",
        "             in the entered collection and subCollections under it",
        " -v  verbose",
        " -V  Very verbose",
        " -R  Resource (Database Resource)",
        " -M  Admin Mode",
        " -h  this help",
        " ",
        "Modify access to dataObjects (iRODS files), collections (directories),",
        "and Database Resources.",
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
        "As normally configured, all users can read all collections (i.e. see",
        "dataObj names within).",
        " ",
        "The inherit/noinherit form sets or clears the inheritance attribute of",
        "one or more collections.  When collections have this attribute set,",
        "new dataObjects and collections added to the collection inherit the",
        "access permisions (ACLs) of the collection.  'ils -A' displays ACLs",
        "and the inheritance status.",
        " ",
        "If you are the irods administrator, you can include the -M option to",
        "run in administrator mode and set the permissions on the collection(s)",
        "and/or data-objects as if you were the owner.  This is more convenient",
        "than aliasing as the user.",
        " ",
        "To execute a Database Object (DBO) on a Database Resource (DBR), users",
        "need to have access permissions ('read', 'write', or better).  The admin",
        "or owner can use ichmod to set these.  See 'ilsresc' for listing these.",
        "See the Database Resources page on the irods web site for more information.",
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
