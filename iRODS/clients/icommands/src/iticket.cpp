/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is an interface to the Ticket management system.
*/

#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "rods.hpp"
#include "rodsClient.hpp"

#define MAX_SQL 300
#define BIG_STR 3000

extern int get64RandomBytes( char *buf );

char cwd[BIG_STR];

int debug = 0;
int longMode = 0; /* more detailed listing */

char zoneArgument[MAX_NAME_LEN + 2] = "";

rcComm_t *Conn;
rodsEnv myEnv;

int lastCommandStatus = 0;
int printCount = 0;
int printedRows = 0;

int usage( char *subOpt );

void showRestrictions( char *inColumn );


/*
 print the results of a general query.
 */
void
printResultsAndSubQuery( rcComm_t *Conn, int status, genQueryOut_t *genQueryOut,
                         char *descriptions[], int subColumn, int dashOpt ) {
    int i, j;
    lastCommandStatus = status;
    if ( status == CAT_NO_ROWS_FOUND ) {
        lastCommandStatus = 0;
    }
    if ( status != 0 && status != CAT_NO_ROWS_FOUND ) {
        printError( Conn, status, "rcGenQuery" );
    }
    else {
        if ( status == CAT_NO_ROWS_FOUND ) {
            if ( printCount == 0 ) {
                printf( "No rows found\n" );
            }
        }
        else {
            for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
                printedRows++;
                char *subCol = "";
                if ( i > 0 && dashOpt > 0 ) {
                    printf( "----\n" );
                }
                for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[j].value;
                    tResult += i * genQueryOut->sqlResult[j].len;
                    if ( subColumn == j ) {
                        subCol = tResult;
                    }
                    if ( *descriptions[j] != '\0' ) {
                        if ( strstr( descriptions[j], "time" ) != 0 ) {
                            char localTime[TIME_LEN];
                            getLocalTimeFromRodsTime( tResult, localTime );
                            if ( strcmp( tResult, "0" ) == 0 || *tResult == '\0' ) {
                                strcpy( localTime, "none" );
                            }
                            printf( "%s: %s\n", descriptions[j],
                                    localTime );
                        }
                        else {
                            printf( "%s: %s\n", descriptions[j], tResult );
                            printCount++;
                        }
                    }
                }
                if ( subColumn >= 0 ) {
                    showRestrictions( subCol );
                }
            }
        }
    }
}

void
showRestrictionsByHost( char *inColumn ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    int i;
    char v1[MAX_NAME_LEN];
    char *condVal[10];
    int status;
    char *columnNames[] = {"restricted-to host"};


    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    printCount = 0;

    i = 0;
    i1a[i] = COL_TICKET_ALLOWED_HOST;
    i1b[i++] = 0;

    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    i2a[0] = COL_TICKET_ALLOWED_HOST_TICKET_ID;
    snprintf( v1, sizeof( v1 ), "='%s'", inColumn );
    condVal[0] = v1;
    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_USER_COMMENT;
        genQueryInp.selectInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "No host restrictions (1)\n" );
            return;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            printf( "No host restrictions\n" );
            return;
        }
    }

    printResultsAndSubQuery( Conn, status, genQueryOut, columnNames, -1, 0 );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        printResultsAndSubQuery( Conn, status, genQueryOut,
                                 columnNames, 0, 0 );
    }
    return;
}

void
showRestrictionsByUser( char *inColumn ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    int i;
    char v1[MAX_NAME_LEN];
    char *condVal[10];
    int status;
    char *columnNames[] = {"restricted-to user"};


    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    printCount = 0;

    i = 0;
    i1a[i] = COL_TICKET_ALLOWED_USER_NAME;
    i1b[i++] = 0;

    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    i2a[0] = COL_TICKET_ALLOWED_USER_TICKET_ID;
    snprintf( v1, sizeof( v1 ), "='%s'", inColumn );
    condVal[0] = v1;
    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_USER_COMMENT;
        genQueryInp.selectInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "No user restrictions (1)\n" );
            return;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            printf( "No user restrictions\n" );
            return;
        }
    }

    printResultsAndSubQuery( Conn, status, genQueryOut, columnNames, -1, 0 );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        printResultsAndSubQuery( Conn, status, genQueryOut,
                                 columnNames, 0, 0 );
    }
    return;
}

void
showRestrictionsByGroup( char *inColumn ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    int i;
    char v1[MAX_NAME_LEN];
    char *condVal[10];
    int status;
    char *columnNames[] = {"restricted-to group"};


    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    printCount = 0;

    i = 0;
    i1a[i] = COL_TICKET_ALLOWED_GROUP_NAME;
    i1b[i++] = 0;

    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    i2a[0] = COL_TICKET_ALLOWED_GROUP_TICKET_ID;
    snprintf( v1, sizeof( v1 ), "='%s'", inColumn );
    condVal[0] = v1;
    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_USER_COMMENT;
        genQueryInp.selectInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "No group restrictions (1)\n" );
            return;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            printf( "No group restrictions\n" );
            return;
        }
    }

    printResultsAndSubQuery( Conn, status, genQueryOut, columnNames, -1, 0 );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        printResultsAndSubQuery( Conn, status, genQueryOut,
                                 columnNames, 0, 0 );
    }
    return;
}

void
showRestrictions( char *inColumn ) {
    showRestrictionsByHost( inColumn );
    showRestrictionsByUser( inColumn );
    showRestrictionsByGroup( inColumn );
    return;
}


/*
Via a general query, show the Tickets for this user
*/
int
showTickets1( char *inOption, char *inName ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20];
    int i2a[20];
    int i;
    char v1[MAX_NAME_LEN];
    char *condVal[20];
    int status;
    char *columnNames[] = {"id", "string", "ticket type", "obj type", "owner name", "owner zone", "uses count", "uses limit", "write file count", "write file limit", "write byte count", "write byte limit", "expire time", "collection name", "data collection"};


    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    printCount = 0;

    i = 0;
    i1a[i] = COL_TICKET_ID;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_STRING;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_TYPE;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_OBJECT_TYPE;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_OWNER_NAME;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_OWNER_ZONE;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_USES_COUNT;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_USES_LIMIT;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_WRITE_FILE_COUNT;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_WRITE_FILE_LIMIT;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_WRITE_BYTE_COUNT;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_WRITE_BYTE_LIMIT;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_EXPIRY_TS;
    i1b[i++] = 0;
    i1a[i] = COL_TICKET_COLL_NAME;
    i1b[i++] = 0;
    if ( strstr( inOption, "data" ) != 0 ) {
        i--;
        i1a[i] = COL_TICKET_DATA_NAME;
        columnNames[i] = "data-object name";
        i1b[i++] = 0;
        i1a[i] = COL_TICKET_DATA_COLL_NAME;
        i1b[i++] = 0;
    }
    if ( strstr( inOption, "basic" ) != 0 ) {
        /* skip the COLL or DATA_NAME so it's just a query on the ticket tables */
        i--;
    }

    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    genQueryInp.condInput.len = 0;

    if ( inName != NULL && *inName != '\0' ) {
        if ( isInteger( inName ) == 1 ) {
            /* Could have an all-integer ticket but in most cases this is a good
            guess */
            i2a[0] = COL_TICKET_ID;
        }
        else {
            i2a[0] = COL_TICKET_STRING;
        }
        snprintf( v1, sizeof( v1 ), "='%s'", inName );
        condVal[0] = v1;
        genQueryInp.sqlCondInp.inx = i2a;
        genQueryInp.sqlCondInp.value = condVal;
        genQueryInp.sqlCondInp.len = 1;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_USER_COMMENT;
        genQueryInp.selectInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            return 0;
        }
    }

    printResultsAndSubQuery( Conn, status, genQueryOut, columnNames, 0, 1 );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printResultsAndSubQuery( Conn, status, genQueryOut,
                                 columnNames, 0, 1 );
    }

    return 0;
}

void
showTickets( char *inName ) {
    printedRows = 0;
    showTickets1( "data", inName );
    if ( printedRows > 0 ) {
        printf( "----\n" );
    }
    showTickets1( "collection", inName );
    if ( printedRows == 0 ) {
        /* try a more basic query in case the data obj or collection is gone */
        showTickets1( "basic", inName );
        if ( printedRows > 0 &&
                inName != NULL && *inName != '\0' ) {
            printf( "Warning: the data-object or collection for this ticket no longer exists\n" );
        }
    }
}


std::string
makeFullPath( const char *inName ) {
    std::stringstream fullPathStream;
    if ( strlen( inName ) == 0 ) {
        return std::string();
    }
    if ( *inName != '/' ) {
        fullPathStream << cwd << "/";
    }
    fullPathStream << inName;
    return fullPathStream.str();
}

/*
 Create, modify, or delete a ticket
 */
int
doTicketOp( const char *arg1, const char *arg2, const char *arg3,
            const char *arg4, const char *arg5 ) {
    ticketAdminInp_t ticketAdminInp;
    int status;
    char *mySubName;
    char *myName;

    ticketAdminInp.arg1 = strdup( arg1 );
    ticketAdminInp.arg2 = strdup( arg2 );
    ticketAdminInp.arg3 = strdup( arg3 );
    ticketAdminInp.arg4 = strdup( arg4 );
    ticketAdminInp.arg5 = strdup( arg5 );
    ticketAdminInp.arg6 = "";

    status = rcTicketAdmin( Conn, &ticketAdminInp );
    lastCommandStatus = status;

    free( ticketAdminInp.arg1 );
    free( ticketAdminInp.arg2 );
    free( ticketAdminInp.arg3 );
    free( ticketAdminInp.arg4 );
    free( ticketAdminInp.arg5 );

    if ( status < 0 ) {
        if ( Conn->rError ) {
            rError_t *Err;
            rErrMsg_t *ErrMsg;
            int i, len;
            Err = Conn->rError;
            len = Err->len;
            for ( i = 0; i < len; i++ ) {
                ErrMsg = Err->errMsg[i];
                rodsLog( LOG_ERROR, "Level %d: %s", i, ErrMsg->msg );
            }
        }
        myName = rodsErrorName( status, &mySubName );
        rodsLog( LOG_ERROR, "rcTicketAdmin failed with error %d %s %s",
                 status, myName, mySubName );
    }
    return status;
}

void
makeTicket( char *newTicket ) {
    int ticket_len = 15;
    char buf1[100], buf2[20];
    get64RandomBytes( buf1 );

    /*
     Set up an array of characters that are allowed in the result.
    */
    char characterSet_len = 26 + 26 + 10;
    char characterSet[26 + 26 + 10];
    int j = 0;
    for ( char i = 0; i < 26; i++ ) {
        characterSet[j++] = 'A' + i;
    }
    for ( char i = 0; i < 26; i++ ) {
        characterSet[j++] = 'a' + i;
    }
    for ( char i = 0; i < 10; i++ ) {
        characterSet[j++] = '0' + i;
    }

    for ( int i = 0; i < ticket_len; i++ ) {
        int ix = buf1[i] % characterSet_len;
        buf2[i] = characterSet[ix];
    }
    buf2[ticket_len] = '\0';
    strncpy( newTicket, buf2, 20 );
    printf( "ticket:%s\n", buf2 );
}

/*
 Prompt for input and parse into tokens
*/
void
getInput( char *cmdToken[], int maxTokens ) {
    int lenstr, i;
    static char ttybuf[BIG_STR];
    int nTokens;
    int tokenFlag; /* 1: start reg, 2: start ", 3: start ' */
    char *cpTokenStart;
    char *stat;

    memset( ttybuf, 0, BIG_STR );
    fputs( "iticket>", stdout );
    stat = fgets( ttybuf, BIG_STR, stdin );
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

    if ( strcmp( cmdToken[0], "create" ) == 0
            || strcmp( cmdToken[0], "make" ) == 0
            || strcmp( cmdToken[0], "mk" ) == 0
       ) {
        char myTicket[30];
        if ( strlen( cmdToken[3] ) > 0 ) {
            snprintf( myTicket, sizeof( myTicket ), "%s", cmdToken[3] );
        }
        else {
            makeTicket( myTicket );
        }
        std::string fullPath = makeFullPath( cmdToken[2] );
        doTicketOp( "create", myTicket, cmdToken[1], fullPath.c_str(),
                    cmdToken[3] );
        return 0;
    }


    if ( strcmp( cmdToken[0], "delete" ) == 0 ) {
        doTicketOp( "delete", cmdToken[1], cmdToken[2],
                    cmdToken[3], cmdToken[4] );
        return 0;
    }


    if ( strcmp( cmdToken[0], "mod" ) == 0 ) {
        doTicketOp( "mod", cmdToken[1], cmdToken[2],
                    cmdToken[3], cmdToken[4] );
        return 0;
    }

    if ( strcmp( cmdToken[0], "ls" ) == 0 ) {
        showTickets( cmdToken[1] );
        return 0;
    }

    if ( strcmp( cmdToken[0], "ls-all" ) == 0 ) {
        printf( "Listing all of your tickets, even those for which the target collection\nor data-object no longer exists:\n" );
        showTickets1( "basic", "" );
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

    rodsArguments_t myRodsArgs;

    char *mySubName;
    char *myName;

    int argOffset;

    int maxCmdTokens = 20;
    char *cmdToken[20];
    int keepGoing;
    int firstTime;

    rodsLogLevel( LOG_ERROR );

    status = parseCmdLineOpt( argc, argv, "vVhgrcGRCdulz:", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage( "" );
        exit( 0 );
    }

    if ( myRodsArgs.zone == True ) {
        strncpy( zoneArgument, myRodsArgs.zoneName, MAX_NAME_LEN );
    }

    if ( myRodsArgs.longOption ) {
        longMode = 1;
    }

    argOffset = myRodsArgs.optind;
    if ( argOffset > 1 ) {
        if ( argOffset > 2 ) {
            if ( *argv[1] == '-' && *( argv[1] + 1 ) == 'z' ) {
                if ( *( argv[1] + 2 ) == '\0' ) {
                    argOffset = 3; /* skip -z zone */
                }
                else {
                    argOffset = 2; /* skip -zzone */
                }
            }
            else {
                argOffset = 1; /* Ignore the parseCmdLineOpt parsing
			    as -d etc handled  below*/
            }
        }
        else {
            argOffset = 1; /* Ignore the parseCmdLineOpt parsing
			 as -d etc handled  below*/
        }
    }

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }
    strncpy( cwd, myEnv.rodsCwd, BIG_STR );
    if ( strlen( cwd ) == 0 ) {
        strcpy( cwd, "/" );
    }

    for ( i = 0; i < maxCmdTokens; i++ ) {
        cmdToken[i] = "";
    }
    j = 0;
    for ( i = argOffset; i < argc; i++ ) {
        cmdToken[j++] = argv[i];
    }

#if defined(linux_platform)
    /*
      imeta cp -d TestFile1 -d TestFile3
      comes in as:     -d -d cp TestFile1 TestFile3
      so switch it to: cp -d -d TestFile1 TestFile3
    */
    if ( cmdToken[0] != NULL && *cmdToken[0] == '-' ) {
        /* args were toggled, switch them back */
        if ( cmdToken[1] != NULL && *cmdToken[1] == '-' ) {
            cmdToken[0] = argv[argOffset + 2];
            cmdToken[1] = argv[argOffset];
            cmdToken[2] = argv[argOffset + 1];
        }
        else {
            cmdToken[0] = argv[argOffset + 1];
            cmdToken[1] = argv[argOffset];
        }
    }
#else
    /* tested on Solaris, not sure other than Linux/Solaris */
    /*
      imeta cp -d TestFile1 -d TestFile3
      comes in as:     cp -d TestFile1 -d TestFile3
      so switch it to: cp -d -d TestFile1 TestFile3
    */
    if ( cmdToken[0] != NULL && cmdToken[1] != NULL && *cmdToken[1] == '-' &&
            cmdToken[2] != NULL && cmdToken[3] != NULL && *cmdToken[3] == '-' ) {
        /* two args */
        cmdToken[2] = argv[argOffset + 3];
        cmdToken[3] = argv[argOffset + 2];
    }

#endif

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
        if ( !debug ) {
            exit( 3 );
        }
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

/*
Print the main usage/help information.
 */
void usageMain() {
    char *msgs[] = {
        "Usage: iticket [-h] [command]",
        " -h This help",
        "Commands are:",
        " create read/write Object-Name [string] (create a new ticket)",
        " mod Ticket_string-or-id uses/expire string-or-none  (modify restrictions)",
        " mod Ticket_string-or-id write-bytes-or-file number-or-0 (modify restrictions)",
        " mod Ticket_string-or-id add/remove host/user/group string (modify restrictions)",
        " ls [Ticket_string-or-id] (non-admins will see just your own)",
        " ls-all (list all your tickets, even with missing targets)",
        " delete ticket_string-or-id",
        " quit",
        " ",
        "Tickets are another way to provide access to iRODS data-objects (files) or",
        "collections (directories or folders).  The 'iticket' command allows you",
        "to create, modify, list, and delete tickets.  When you create a ticket",
        "it's 16 character string it given to you which you can share with others.",
        "This is less secure than normal iRODS login-based access control, but",
        "is useful in some situations.  See the 'ticket-based access' page on",
        "irods.org for more information.",
        " ",
        "A blank execute line invokes the interactive mode, where iticket",
        "prompts and executes commands until 'quit' or 'q' is entered.",
        "Like other unix utilities, a series of commands can be piped into it:",
        "'cat file1 | iticket' (maintaining one connection for all commands).",
        " ",
        "Use 'help command' for more help on a specific command.",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "iticket" );
}

/*
Print either main usage/help information, or some more specific
information on particular commands.
 */
int
usage( char *subOpt ) {
    int i;
    if ( *subOpt == '\0' ) {
        usageMain();
    }
    else {
        if ( strcmp( subOpt, "create" ) == 0 ) {
            char *msgs[] = {
                " create read/write Object-Name [string] (create a new ticket)",
                "Create a new ticket for Object-Name, which is either a data-object (file)",
                "or a collection (directory). ",
                "Example: create read myFile",
                "The ticket string, which can be used for access, will be displayed.",
                "If 'string' is provided on the command line, it is the ticket-string to use",
                "as the ticket instead of a randomly generated string of characters.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "mod" ) == 0 ) {
            char *msgs[] = {
                "   mod Ticket-id uses/expire string-or-none",
                "or mod Ticket-id add/remove host/user/group string (modify restrictions)",
                "Modify a ticket to use one of the specialized options.  By default a",
                "ticket can be used by anyone (and 'anonymous'), from any host, and any",
                "number of times, and for all time (until deleted).  You can modify it to",
                "add (or remove) these types of restrictions.",
                " ",
                " 'mod Ticket-id uses integer-or-0' will make the ticket only valid",
                "the specified number of times.  Use 0 to remove this restriction.",
                " ",
                " 'mod Ticket-id write-file integer-or-0' will make the write-ticket only",
                "valid for writing the specified number of times.  Use 0 to remove this",
                "restriction.",
                " ",
                " 'mod Ticket-id write-byte integer-or-0' will make the write-ticket only",
                "valid for writing the specified number of bytes.  Use 0 to remove this",
                "restriction.",
                " ",
                " 'mod Ticket-id add/remove user Username' will make the ticket only valid",
                "when used by that particular iRODS user.  You can use multiple mod commands",
                "to add more users to the allowed list.",
                " ",
                " 'mod Ticket-id add/remove group Groupname' will make the ticket only valid",
                "when used by iRODS users in that particular iRODS group.  You can use",
                "multiple mod commands to add more groups to the allowed list.",
                " ",
                " 'mod Ticket-id add/remove host Host/IP' will make the ticket only valid",
                "when used from that particular host computer.  Host (full DNS name) will be",
                "converted to the IP address for use in the internal checks or you can enter",
                "the IP address itself.  'iticket ls' will display the IP addresses.",
                "You can use multiple mod commands to add more hosts to the list.",
                " ",
                " 'mod Ticket-id expire date.time-or-0' will make the ticket only valid",
                "before the specified date-time.  You can cancel this expiration by using",
                "'0'.  The time is year-mo-da.hr:min:sec, for example: 2012-05-07.23:00:00",
                " ",
                " The Ticket-id is either the ticket object number or the ticket-string",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "delete" ) == 0 ) {
            char *msgs[] = {
                " delete Ticket-string",
                "Remove a ticket from the system.  Access will no longer be allowed",
                "via the ticket-string.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }

        if ( strcmp( subOpt, "ls" ) == 0 ) {
            char *msgs[] = {
                " ls [Ticket_string-or-id]",
                "List the tickets owned by you or, for admin users, all tickets.",
                "Include a ticket-string or the ticket-id (object number) to list only one",
                "(in this case, a numeric string is assumed to be an id).",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }

        if ( strcmp( subOpt, "ls-all" ) == 0 ) {
            char *msgs[] = {
                " ls-all",
                "Similar to 'ls' (with no ticket string-or-id) but will list all of your",
                "tickets even if the target collection or data-object no longer exists.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        printf( "Sorry, either %s is an invalid command or the help has not been written yet\n",
                subOpt );
    }
    return 0;
}
