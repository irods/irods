/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "rods.hpp"
#include "rodsClient.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

rcComm_t *Conn;

int lastCommandStatus = 0;
int printCount = 0;

char myZone[50];

void usage( char *subOpt );

/*
 print the results of a general query for the showGroup function below
 */
void
printGenQueryResultsForGroup( genQueryOut_t *genQueryOut ) {
    int i, j;
    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        char *tResult;
        for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
            tResult = genQueryOut->sqlResult[j].value;
            tResult += i * genQueryOut->sqlResult[j].len;
            if ( j > 0 ) {
                printf( "#%s", tResult );
            }
            else {
                printf( "%s", tResult );
            }
        }
        printf( "\n" );
    }
}


/*
 Via a general query, show group information
 */
void
showGroup( char *groupName ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int selectIndexes[10];
    int selectValues[10];
    int conditionIndexes[10];
    char *conditionValues[10];
    char conditionString1[200];
    char conditionString2[200];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    if ( groupName != NULL && *groupName != '\0' ) {
        printf( "Members of group %s:\n", groupName );
    }
    selectIndexes[0] = COL_USER_NAME;
    selectValues[0] = 0;
    selectIndexes[1] = COL_USER_ZONE;
    selectValues[1] = 0;
    genQueryInp.selectInp.inx = selectIndexes;
    genQueryInp.selectInp.value = selectValues;
    if ( groupName != NULL && *groupName != '\0' ) {
        genQueryInp.selectInp.len = 2;
    }
    else {
        genQueryInp.selectInp.len = 1;
    }

    conditionIndexes[0] = COL_USER_TYPE;
    snprintf( conditionString1, sizeof( conditionString1 ), "='rodsgroup'" );
    conditionValues[0] = conditionString1;

    genQueryInp.sqlCondInp.inx = conditionIndexes;
    genQueryInp.sqlCondInp.value = conditionValues;
    genQueryInp.sqlCondInp.len = 1;

    if ( groupName != NULL && *groupName != '\0' ) {

        snprintf( conditionString1, sizeof( conditionString1 ), "!='rodsgroup'" );

        conditionIndexes[1] = COL_USER_GROUP_NAME;
        snprintf( conditionString2, sizeof( conditionString2 ), "='%s'", groupName );
        conditionValues[1] = conditionString2;
        genQueryInp.sqlCondInp.len = 2;
    }

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        printf( "No rows found\n" );
        return;
    }
    else {
        printGenQueryResultsForGroup( genQueryOut );
    }

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printGenQueryResultsForGroup( genQueryOut );
        }
    }
    return;
}

/*
 Prompt for input and parse into tokens
*/
void
getInput( char *cmdToken[], int maxTokens ) {
    int lenstr, i;
    static char ttybuf[500];
    int nTokens;
    int tokenFlag; /* 1: start reg, 2: start ", 3: start ' */
    char *cpTokenStart;
    char *stat;

    memset( ttybuf, 0, sizeof( ttybuf ) );
    fputs( "groupadmin>", stdout );
    stat = fgets( ttybuf, sizeof( ttybuf ), stdin );
    if ( stat == 0 ) {
        printf( "\n" );
        rcDisconnect( Conn );
        if ( lastCommandStatus != 0 ) {
            exit( 4 );
        }
        exit( 0 );
    }
    lenstr = strlen( ttybuf );
    for ( i = 0; i < maxTokens; i++ ) {
        cmdToken[i] = "";
    }
    cpTokenStart = ttybuf;
    nTokens = 0;
    tokenFlag = 0;
    for ( i = 0; i < lenstr; i++ ) {
        if ( ttybuf[i] == '\n' ) {
            ttybuf[i] = '\0';
            cmdToken[nTokens++] = cpTokenStart;
            return;
        }
        if ( tokenFlag == 0 ) {
            if ( ttybuf[i] == '\'' ) {
                tokenFlag = 3;
                cpTokenStart++;
            }
            else if ( ttybuf[i] == '"' ) {
                tokenFlag = 2;
                cpTokenStart++;
            }
            else if ( ttybuf[i] == ' ' ) {
                cpTokenStart++;
            }
            else {
                tokenFlag = 1;
            }
        }
        else if ( tokenFlag == 1 ) {
            if ( ttybuf[i] == ' ' ) {
                ttybuf[i] = '\0';
                cmdToken[nTokens++] = cpTokenStart;
                cpTokenStart = &ttybuf[i + 1];
                tokenFlag = 0;
            }
        }
        else if ( tokenFlag == 2 ) {
            if ( ttybuf[i] == '"' ) {
                ttybuf[i] = '\0';
                cmdToken[nTokens++] = cpTokenStart;
                cpTokenStart = &ttybuf[i + 1];
                tokenFlag = 0;
            }
        }
        else if ( tokenFlag == 3 ) {
            if ( ttybuf[i] == '\'' ) {
                ttybuf[i] = '\0';
                cmdToken[nTokens++] = cpTokenStart;
                cpTokenStart = &ttybuf[i + 1];
                tokenFlag = 0;
            }
        }
    }
}

/*
 Perform an rcUserAdmin call
 */
int
userAdmin( char *arg0, char *arg1, char *arg2, char *arg3,
           char *arg4, char *arg5, char *arg6, char *arg7 ) {
    userAdminInp_t userAdminInp;
    int status;
    char *mySubName;
    char *myName;
    char *funcName;

    userAdminInp.arg0 = arg0;
    userAdminInp.arg1 = arg1;
    userAdminInp.arg2 = arg2;
    userAdminInp.arg3 = arg3;
    userAdminInp.arg4 = arg4;
    userAdminInp.arg5 = arg5;
    userAdminInp.arg6 = arg6;
    userAdminInp.arg7 = arg7;
    userAdminInp.arg8 = "";
    userAdminInp.arg9 = "";
    status = rcUserAdmin( Conn, &userAdminInp );
    funcName = "rcUserAdmin";

    if ( Conn->rError ) {
        rError_t *Err;
        rErrMsg_t *ErrMsg;
        int i, len;
        Err = Conn->rError;
        len = Err->len;
        for ( i = 0; i < len; i++ ) {
            ErrMsg = Err->errMsg[i];
            printf( "Level %d message: %s\n", i, ErrMsg->msg );
        }
    }
    if ( status < 0 ) {
        myName = rodsErrorName( status, &mySubName );
        rodsLog( LOG_ERROR, "%s failed with error %d %s %s", funcName,
                 status, myName, mySubName );
        if ( status == CAT_INVALID_USER_TYPE ) {
            printf( "See 'lt user_type' for a list of valid user types.\n" );
        }
    }
    return status;
}

char *
setScrambledPw( char *inPass ) {
    int len, lcopy, i;
    char buf0[MAX_PASSWORD_LEN + 10];
    char buf1[MAX_PASSWORD_LEN + 10];
    /* this is an string pattern used to pad, arbitrary, but must match
       the server side: */
    char rand[] = "1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs";
    static char buf2[MAX_PASSWORD_LEN + 10];
    strncpy( buf0, inPass, MAX_PASSWORD_LEN );
    buf0[MAX_PASSWORD_LEN] = '\0';
    len = strlen( inPass );
    lcopy = MAX_PASSWORD_LEN - 10 - len;
    if ( lcopy > 15 ) {
        /* server will look for 15 characters
        		  of random string */
        strncat( buf0, rand, lcopy );
    }
    i = obfGetPw( buf1 );
    if ( i != 0 ) {
        printf( "Error getting current password\n" );
        exit( 1 );
    }
    obfEncodeByKey( buf0, buf1, buf2 );

    return buf2;
}


/* handle a command,
   return code is 0 if the command was (at least partially) valid,
   -1 for quitting,
   -2 for if invalid
   -3 if empty.
 */
int
doCommand( char *cmdToken[] ) {
    if ( strcmp( cmdToken[0], "help" ) == 0 ||
            strcmp( cmdToken[0], "h" ) == 0 ) {
        usage( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "quit" ) == 0 ||
            strcmp( cmdToken[0], "q" ) == 0 ) {
        return -1;
    }

    if ( strcmp( cmdToken[0], "lg" ) == 0 ) {
        showGroup( cmdToken[1] );
        return 0;
    }

    if ( strcmp( cmdToken[0], "mkuser" ) == 0 ) {
        userAdmin( "mkuser", cmdToken[1], setScrambledPw( cmdToken[2] ),
                   cmdToken[3], "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "atg" ) == 0 ) {
        userAdmin( "modify", "group", cmdToken[1], "add", cmdToken[2],
                   cmdToken[3], "", "" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "mkgroup" ) == 0 ) {
        userAdmin( "mkgroup", cmdToken[1], "rodsgroup",
                   myZone, "", "", "", "" );

        return 0;
    }

    if ( strcmp( cmdToken[0], "rfg" ) == 0 ) {
        userAdmin( "modify", "group", cmdToken[1], "remove", cmdToken[2],
                   cmdToken[3], "", "" );
        return 0;
    }
    if ( *cmdToken[0] != '\0' ) {
        printf( "unrecognized command, try 'help'\n" );
        return -2;
    }
    return -3;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status, i, j;
    rErrMsg_t errMsg;
    rodsEnv myEnv;

    rodsArguments_t myRodsArgs;

    char *mySubName;
    char *myName;

    int argOffset;

    int maxCmdTokens = 20;
    char *cmdToken[20];
    int keepGoing;
    int firstTime;

    rodsLogLevel( LOG_ERROR );

    status = parseCmdLineOpt( argc, argv, "vVh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage( "" );
        exit( 0 );
    }

    argOffset = myRodsArgs.optind;

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }
    strncpy( myZone, myEnv.rodsZone, sizeof( myZone ) );

    for ( i = 0; i < maxCmdTokens; i++ ) {
        cmdToken[i] = "";
    }
    j = 0;
    for ( i = argOffset; i < argc; i++ ) {
        cmdToken[j++] = argv[i];
    }

    if ( strcmp( cmdToken[0], "help" ) == 0 ||
            strcmp( cmdToken[0], "h" ) == 0 ) {
        usage( cmdToken[1] );
        exit( 0 );
    }

    if ( strcmp( cmdToken[0], "spass" ) == 0 ) {
        char scrambled[MAX_PASSWORD_LEN + 100];
        if ( strlen( cmdToken[1] ) > MAX_PASSWORD_LEN - 2 ) {
            printf( "Password exceeds maximum length\n" );
        }
        else {
            obfEncodeByKey( cmdToken[1], cmdToken[2], scrambled );
            printf( "Scrambled form is:%s\n", scrambled );
        }
        exit( 0 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        myName = rodsErrorName( errMsg.status, &mySubName );
        rodsLog( LOG_ERROR, "rcConnect failure %s (%s) (%d) %s",
                 myName,
                 mySubName,
                 errMsg.status,
                 errMsg.msg );

        exit( 2 );
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        rcDisconnect( Conn );
        exit( 3 );
    }

    keepGoing = 1;
    firstTime = 1;
    while ( keepGoing ) {
        int status;
        status = doCommand( cmdToken );
        if ( status == -1 ) {
            keepGoing = 0;
        }
        if ( firstTime ) {
            if ( status == 0 ) {
                keepGoing = 0;
            }
            if ( status == -2 ) {
                keepGoing = 0;
                lastCommandStatus = -1;
            }
            firstTime = 0;
        }
        if ( keepGoing ) {
            getInput( cmdToken, maxCmdTokens );
        }
    }

    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    if ( lastCommandStatus != 0 ) {
        exit( 4 );
    }
    exit( 0 );
}

void
printMsgs( char *msgs[] ) {
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            return;
        }
        printf( "%s\n", msgs[i] );
    }
}

void usageMain() {
    char *Msgs[] = {
        "Usage: igroupadmin [-hvV] [command]",
        " ",
        "This is for users of type 'groupadmin' which are allowed to perform",
        "certain administrative functions.  It can also be used by regular",
        "users to list groups ('lg') and by the admin for all operations.",
        " ",
        "A blank execute line invokes the interactive mode, where it",
        "prompts and executes commands until 'quit' or 'q' is entered.",
        "Single or double quotes can be used to enter items with blanks.",
        "Commands are:",
        " lg [name] (list group info (user member list))",
        " mkuser Name Password (make a user and set the initial password)",
        " atg groupName userName[#Zone] (add to group - add a user to a group)",
        " rfg groupName userName[#Zone] (remove from group - remove a user from a group)",
        " mkgroup groupName[#Zone] (make a new group)",
        " help (or h) [command] (this help, or more details on a command)",
        ""
    };
    printMsgs( Msgs );
    printReleaseInfo( "igroupadmin" );
}

void
usage( char *subOpt ) {
    char *mkuserMsgs[] = {
        " mkuser Name Group Password (make user by a group-admin)",
        "Create a new iRODS user in the ICAT database",
        "",
        "Name is the user name to create",
        "Group is the group to also add the user to",
        "Password is the user's initial password",
        "",
        "The user type will be automatically set to 'rodsuser' and the zone local.",
        ""
    };

    char *lgMsgs[] = {
        " lg [name] (list group info (user member list))",
        "Just 'lg' briefly lists the defined groups.",
        "If you include a group name, it will list users who are",
        "members of that group.  Users are listed in the user#zone format.",
        "In addition to 'rodsadmin', any user can use this sub-command; this is",
        "of most value to 'groupadmin' users who can also atg, rfg, and mkuser.",
        ""
    };

    char *atgMsgs[] = {
        " atg groupName userName[#userZone] (add to group - add a user to a group)",
        "For remote-zone users, include the userZone.",
        "Also see mkgroup, rfg and rmgroup.",
        "In addition to the 'rodsadmin', users of type 'groupadmin' can atg and rfg",
        "for groups they are members of.  They can see group membership via iuserinfo.",
        ""
    };

    char *rfgMsgs[] = {
        " rfg groupName userName[#userZone] (remove from group - remove a user from a group)",
        "For remote-zone users, include the userZone.",
        "Also see mkgroup, afg and rmgroup.",
        "In addition to the 'rodsadmin', users of type 'groupadmin' can atg and rfg",
        "for groups they are members of.  They can see group membership via iuserinfo.",
        ""
    };

    char *mkgroupMsgs[] = {
        " mkgroup groupName[#Zone] (make a new group)",
        "Make a new group.  You will need to add yourself to the new group to then",
        "be able to add and remove others (when the group is empty groupadmins are",
        "allowed to add themselves.)",
        ""
    };

    char *helpMsgs[] = {
        " help (or h) [command] (general help, or more details on a command)",
        " If you specify a command, a brief description of that command",
        " will be displayed.",
        ""
    };

    char *subCmds[] = {
        "lg",
        "mkuser",
        "atg",
        "rfg",
        "mkgroup",
        "help", "h",
        ""
    };

    char **pMsgs[] = {
        lgMsgs,
        mkuserMsgs,
        atgMsgs,
        rfgMsgs,
        mkgroupMsgs,
        helpMsgs, helpMsgs
    };

    if ( *subOpt == '\0' ) {
        usageMain();
    }
    else {
        int i;
        for ( i = 0;; i++ ) {
            if ( strlen( subCmds[i] ) == 0 ) {
                break;
            }
            if ( strcmp( subOpt, subCmds[i] ) == 0 ) {
                printMsgs( pMsgs[i] );
            }
        }
    }
}
