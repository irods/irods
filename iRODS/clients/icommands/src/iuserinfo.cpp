/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is an interface to the user information (metadata).
*/

#include "rods.hpp"
#include "rodsClient.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

#define MAX_SQL 300
#define BIG_STR 200

char cwd[BIG_STR];

int debug = 0;

rcComm_t *Conn;
rodsEnv myEnv;

void usage();

/*
 print the results of a general query.
 */
int
printGenQueryResults( rcComm_t *Conn, int status, genQueryOut_t *genQueryOut,
                      char *descriptions[] ) {
    int printCount;
    int i, j;
    char localTime[TIME_LEN];
    printCount = 0;
    if ( status != 0 ) {
        printError( Conn, status, "rcGenQuery" );
    }
    else {
        if ( status != CAT_NO_ROWS_FOUND ) {
            for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
                for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[j].value;
                    tResult += i * genQueryOut->sqlResult[j].len;
                    if ( strstr( descriptions[j], "time" ) != 0 ) {
                        getLocalTimeFromRodsTime( tResult, localTime );
                        printf( "%s: %s: %s\n", descriptions[j], tResult,
                                localTime );
                    }
                    else {
                        printf( "%s: %s\n", descriptions[j], tResult );
                    }
                    printCount++;
                }
            }
        }
    }
    return printCount;
}

/*
Via a general query, show user information
*/
int
showUser( char *name ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int i2a[20];
    char *condVal[10];
    char v1[BIG_STR];
    int i, status;
    int printCount;
    char *columnNames[] = {"name", "id", "type", "zone", "info", "comment",
                           "create time", "modify time"
                          };

    char *columnNames2[] = {"member of group"};
    char *columnNames3[] = {"GSI DN or Kerberos Principal Name"};

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    printCount = 0;
    i = 0;
    i1a[i++] = COL_USER_NAME;
    i1a[i++] = COL_USER_ID;
    i1a[i++] = COL_USER_TYPE;
    i1a[i++] = COL_USER_ZONE;
    i1a[i++] = COL_USER_INFO;
    i1a[i++] = COL_USER_COMMENT;
    i1a[i++] = COL_USER_CREATE_TIME;
    i1a[i++] = COL_USER_MODIFY_TIME;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    i2a[0] = COL_USER_NAME;
    snprintf( v1, BIG_STR, "='%s'", name );
    condVal[0] = v1;

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.condInput.len = 0;

    genQueryInp.maxRows = 2;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_USER_COMMENT;
        genQueryInp.selectInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "None\n" );
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            printf( "User %s does not exist.\n", name );
            return 0;
        }
    }

    printCount += printGenQueryResults( Conn, status, genQueryOut, columnNames );


    printCount = 0;
    i1a[0] = COL_USER_DN;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_USER_NAME;
    snprintf( v1, BIG_STR, "='%s'", name );
    condVal[0] = v1;

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.condInput.len = 0;

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
    }
    else {
        printCount += printGenQueryResults( Conn, status, genQueryOut,
                                            columnNames3 );

        while ( status == 0 && genQueryOut->continueInx > 0 ) {
            genQueryInp.continueInx = genQueryOut->continueInx;
            status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
            printCount += printGenQueryResults( Conn, status, genQueryOut,
                                                columnNames3 );
        }
    }

    printCount = 0;
    i1a[0] = COL_USER_GROUP_NAME;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_USER_NAME;
    snprintf( v1, BIG_STR, "='%s'", name );
    condVal[0] = v1;

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.condInput.len = 0;

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        printf( "Not a member of any group\n" );
        return 0;
    }
    printCount += printGenQueryResults( Conn, status, genQueryOut, columnNames2 );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        printCount += printGenQueryResults( Conn, status, genQueryOut,
                                            columnNames2 );
    }

    return 0;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status, nArgs;
    rErrMsg_t errMsg;

    rodsArguments_t myRodsArgs;

    rodsLogLevel( LOG_ERROR );

    status = parseCmdLineOpt( argc, argv, "vVh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        exit( 2 );
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        if ( !debug ) {
            exit( 3 );
        }
    }

    nArgs = argc - myRodsArgs.optind;

    if ( nArgs == 1 ) {
        status = showUser( argv[myRodsArgs.optind] );
    }
    else {
        status = showUser( myEnv.rodsUserName );
    }

    /* status = parseUserName(cmdToken[1], userName, zoneName); needed? */
    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    exit( 0 );
}

/*
Print the main usage/help information.
 */
void usage() {
    char *msgs[] = {
        "Usage: iuserinfo [-vVh] [user]",
        " ",
        "Show information about your iRODS user account or the entered user",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "iuserinfo" );
}
