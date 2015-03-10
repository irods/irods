/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is an interface to the Attribute-Value-Units type of metadata.
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
                      char *descriptions[], int formatFlag ) {
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
                if ( i > 0 && descriptions ) {
                    printf( "----\n" );
                }
                for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[j].value;
                    tResult += i * genQueryOut->sqlResult[j].len;
                    if ( descriptions ) {
                        if ( strcmp( descriptions[j], "time" ) == 0 ) {
                            getLocalTimeFromRodsTime( tResult, localTime );
                            printf( "%s: %s : %s\n", descriptions[j], tResult,
                                    localTime );
                        }
                        else {
                            printf( "%s: %s\n", descriptions[j], tResult );
                        }
                    }
                    else {
                        if ( formatFlag == 0 ) {
                            printf( "%s\n", tResult );
                        }
                        else {
                            printf( "%s ", tResult );
                        }
                    }
                    printCount++;
                }
                if ( formatFlag == 1 ) {
                    printf( "\n" );
                }
            }
        }
    }
    return 0;
}

/*
Via a general query, show rule information
*/
int
showRuleExec( char *name, char *ruleName, int allFlag ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int i2a[20];
    char *condVal[10];
    char v1[BIG_STR];
    char v2[BIG_STR];
    int i, status;
    int printCount;
    char *columnNames[] = {"id", "name", "rei_file_path", "user_name",
                           "address", "time", "frequency", "priority",
                           "estimated_exe_time", "notification_addr",
                           "last_exe_time", "exec_status"
                          };

    printCount = 0;
    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    i = 0;
    i1a[i++] = COL_RULE_EXEC_ID;
    i1a[i++] = COL_RULE_EXEC_NAME;
    i1a[i++] = COL_RULE_EXEC_REI_FILE_PATH;
    i1a[i++] = COL_RULE_EXEC_USER_NAME;
    i1a[i++] = COL_RULE_EXEC_ADDRESS;
    i1a[i++] = COL_RULE_EXEC_TIME;
    i1a[i++] = COL_RULE_EXEC_FREQUENCY;
    i1a[i++] = COL_RULE_EXEC_PRIORITY;
    i1a[i++] = COL_RULE_EXEC_ESTIMATED_EXE_TIME;
    i1a[i++] = COL_RULE_EXEC_NOTIFICATION_ADDR;
    i1a[i++] = COL_RULE_EXEC_LAST_EXE_TIME;
    i1a[i++] = COL_RULE_EXEC_STATUS;

    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    if ( allFlag ) {
        genQueryInp.sqlCondInp.len = 0;
    }
    else {
        i2a[0] = COL_RULE_EXEC_USER_NAME;
        snprintf( v1, BIG_STR, "='%s'", name );
        condVal[0] = v1;
        genQueryInp.sqlCondInp.inx = i2a;
        genQueryInp.sqlCondInp.value = condVal;
        genQueryInp.sqlCondInp.len = 1;
    }

    if ( ruleName != NULL && *ruleName != '\0' ) {
        int i;
        i =  genQueryInp.sqlCondInp.len;
        /*  i2a[i]=COL_RULE_EXEC_NAME;
        sprintf(v2,"='%s'",ruleName);  */
        i2a[i] = COL_RULE_EXEC_ID;
        snprintf( v2, BIG_STR, "='%s'", ruleName );
        condVal[i] = v2;
        genQueryInp.sqlCondInp.len++;
    }

    genQueryInp.condInput.len = 0;

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        if ( ruleName != NULL && *ruleName != '\0' ) {
            printf( "User %s or rule '%s' does not exist.\n", name, ruleName );
        }
        else {
            i1a[0] = COL_USER_COMMENT;
            i2a[0] = COL_USER_NAME;
            genQueryInp.selectInp.len = 1;
            status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
            if ( status == 0 ) {
                if ( allFlag ) {
                    printf( "No delayed rules pending\n" );
                }
                else {
                    printf( "No delayed rules pending for user %s\n", name );
                }
                return 0;
            }
            printf( "User %s does not exist.\n", name );
            return 0;
        }
    }

    if ( allFlag ) {
        printf( "Pending rule-executions\n" );
    }
    else {
        printf( "Pending rule-executions for user %s\n", name );
    }
    printCount += printGenQueryResults( Conn, status, genQueryOut, columnNames, 0 );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printCount += printGenQueryResults( Conn, status, genQueryOut,
                                            columnNames, 0 );
    }

    return 0;
}

/*
Via a general query, show rule information, brief form
*/
int
showRuleExecBrief( char *name, int allFlag ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int i2a[20];
    char *condVal[10];
    char v1[BIG_STR];
    int i, status;
    int printCount;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    printCount = 0;
    i = 0;
    i1a[i++] = COL_RULE_EXEC_ID;
    i1a[i++] = COL_RULE_EXEC_NAME;

    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    i2a[0] = COL_RULE_EXEC_USER_NAME;
    sprintf( v1, "='%s'", name );
    condVal[0] = v1;

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;
    if ( allFlag ) {
        genQueryInp.sqlCondInp.len = 0;
    }

    genQueryInp.condInput.len = 0;

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_USER_COMMENT;
        i2a[0] = COL_USER_NAME;
        genQueryInp.selectInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            if ( allFlag ) {
                printf( "No delayed rules pending\n" );
            }
            else {
                printf( "No delayed rules pending for user %s\n", name );
            }
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            printf( "User %s does not exist.\n", name );
            return 0;
        }
    }
    printf( "id     name\n" );
    printCount += printGenQueryResults( Conn, status, genQueryOut, NULL, 1 );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        printCount += printGenQueryResults( Conn, status, genQueryOut,
                                            NULL, 1 );
    }
    return 0;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status, nArgs;
    rErrMsg_t errMsg;

    rodsArguments_t myRodsArgs;

    char userName[NAME_LEN];

    rodsLogLevel( LOG_ERROR );

    status = parseCmdLineOpt( argc, argv, "alu:vVh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help\n" );
        return 1;
    }
    if ( myRodsArgs.help == True ) {
        usage();
        return 0;
    }

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        return 1;
    }

    if ( myRodsArgs.user ) {
        snprintf( userName, sizeof( userName ), "%s", myRodsArgs.userString );
    }
    else {
        snprintf( userName, sizeof( userName ), "%s", myEnv.rodsUserName );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        return 2;
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        if ( !debug ) {
            return 3;
        }
    }

    nArgs = argc - myRodsArgs.optind;
    if ( nArgs > 0 ) {
        status = showRuleExec( userName, argv[myRodsArgs.optind],
                               myRodsArgs.all );
    }
    else {
        if ( myRodsArgs.longOption == 0 ) {
            status = showRuleExecBrief( userName, myRodsArgs.all );
        }
        else {
            status = showRuleExec( userName, "", myRodsArgs.all );
        }
    }

    /* status = parseUserName(cmdToken[1], userName, zoneName); */
    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    return status;
}

/*
Print the main usage/help information.
 */
void usage() {
    char *msgs[] = {
        "Usage: iqstat [-luvVh] [-u user] [ruleId]",
        "Show information about your pending iRODS rule executions",
        "or for the entered user.",
        " -a        display requests of all users",
        " -l        for long format",
        " -u user   for the specified user",
        " ruleId for the specified rule",
        " ",
        "See also iqdel and iqmod",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "iqstat" );
}
