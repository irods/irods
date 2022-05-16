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
    int status = parseCmdLineOpt( argc, argv, "rhvVM", 0, &myRodsArgs );
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
        "Usage: ichmod [-rhvVM] ACCESS_LEVEL USER_OR_GROUP LOGICAL_PATH ...",
        " or    ichmod [-rhvVM] inherit COLLECTION ...",
        " or    ichmod [-rhvVM] noinherit COLLECTION ...",
        " ",
        "Options:",
        " -r  Recursively set the ACCESS_LEVEL for all DataObjects",
        "     and Collections in LOGICAL_PATH.  LOGICAL_PATH must",
        "     represent a Collection.",
        " -v  verbose",
        " -V  Very verbose",
        " -M  Admin Mode",
        " -h  this help",
        " ",
        "Modify access to dataObjects and Collections.",
        " ",
        "By default, the original creator of a dataObject has 'own' permission.",
        " ",
        "The iRODS Permission Model allows for multiple ownership.",
        "  ",
        "The iRODS Permission Model is linear with 10 levels.",
        "A granted ACCESS_LEVEL also includes access to all lower ACCESS_LEVELs.",
        " ",
        " - 'own'",
        " - 'delete_object'",
        " - 'write', 'modify_object'",
        " - 'create_object'",
        " - 'delete_metadata'",
        " - 'modify_metadata'",
        " - 'create_metadata'",
        " - 'read', 'read_object'",
        " - 'read_metadata'",
        " - 'null'",
        " ",
        "Granting 'own' to another USER_OR_GROUP will allow them to grant",
        "permission to and revoke permission from others (including you).",
        " ",
        "Setting the ACCESS_LEVEL to 'null' will remove access for that USER_OR_GROUP.",
        " ",
        "Multiple LOGICAL_PATHs can be entered at the same time.",
        " ",
        "If the entered path is a Collection, then the ACCESS_LEVEL to",
        "that Collection will be modified.  For example, 'write' granted to a",
        "USER_OR_GROUP will allow creating and modification of dataObjects",
        "in that Collection.",
        " ",
        "The inherit/noinherit form sets or clears the inheritance attribute of",
        "one or more Collections.  When Collections have this attribute set,",
        "new dataObjects and subCollections added to the Collection inherit the",
        "access permissions (ACLs) of the Collection.  'ils -A' displays ACLs",
        "and the inheritance status.",
        " ",
        "The -M option allows a rodsadmin to set an ACCESS_LEVEL without having 'own'.",
        " ",
        "The original owner of a dataObject can still set an ACCESS_LEVEL, even",
        "if their ACCESS_LEVEL has been set to 'null'.",
        " ",
        "Example Operations:",
        " - irm - requires 'delete_object' or greater",
        " - imv - requires 'delete_object' or greater, due to removal of old name",
        " - iput - requires 'modify_object' or greater",
        " - iget - requires 'read_object' or greater",
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
