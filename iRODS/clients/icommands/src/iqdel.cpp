/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  A user interface for deleting delayed execution rules
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

#define MAX_SQL 300
#define BIG_STR 200

void usage();
int
qdelUtil( rcComm_t *conn, char *userName, int allFlag,
          rodsArguments_t *myRodsArgs );

int debug = 0;

rcComm_t *Conn;
rodsEnv myEnv;

int
rmDelayedRule( char *ruleId ) {
    int status;

    ruleExecDelInp_t ruleExecDelInp;

    snprintf( ruleExecDelInp.ruleExecId, sizeof( ruleExecDelInp.ruleExecId ),
              "%s", ruleId );
    status = rcRuleExecDel( Conn, &ruleExecDelInp );

    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        printf( "No rule found with id %s\n", ruleId );
    }
    if ( status < 0 ) {
        printError( Conn, status, "rcRuleExecDel" );
    }
    return status;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rErrMsg_t errMsg;
    rodsArguments_t myRodsArgs;
    int argOffset;
    int i;

    rodsLogLevel( LOG_ERROR ); /* This should be the default someday */

    status = parseCmdLineOpt( argc, argv, "au:vVh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }
    argOffset = myRodsArgs.optind;

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    if ( myRodsArgs.all || myRodsArgs.user ) {
        if ( argc > argOffset ) {
            /* should not have any input */
            usage();
            exit( -1 );
        }
    }
    else if ( argc <= argOffset ) {
        usage();
        exit( -1 );
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
        printError( Conn, status, "clientLogin" );
        if ( !debug ) {
            exit( 3 );
        }
    }

    if ( myRodsArgs.all ) {
        status = qdelUtil( Conn, NULL, myRodsArgs.all, &myRodsArgs );
    }
    else if ( myRodsArgs.user ) {
        status = qdelUtil( Conn, myRodsArgs.userString, myRodsArgs.all, &myRodsArgs );
    }
    else {
        for ( i = argOffset; i < argc; i++ ) {
            status = rmDelayedRule( argv[i] );
        }
    }

    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    if ( status ) {
        exit( 4 );
    }
    exit( 0 );
}

int
qdelUtil( rcComm_t *conn, char *userName, int allFlag,
          rodsArguments_t *myRodsArgs ) {
    genQueryInp_t genQueryInp;
    int status, i, continueInx;
    char tmpStr[MAX_NAME_LEN];
    sqlResult_t *execId;
    char *execIdStr;
    genQueryOut_t *genQueryOut = NULL;
    int savedStatus = 0;

    if ( allFlag == 1 && userName != NULL ) {
        rodsLog( LOG_ERROR,
                 "qdelUtil: all(-a) and  user(-u) cannot be used together" );
        return USER_INPUT_OPTION_ERR;
    }
    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    if ( userName != NULL ) {
        snprintf( tmpStr, MAX_NAME_LEN, " = '%s'", userName );
        addInxVal( &genQueryInp.sqlCondInp, COL_RULE_EXEC_USER_NAME, tmpStr );
    }
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ID, 1 );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    continueInx = 1;    /* a fake one so it will do the first query */
    while ( continueInx > 0 ) {
        status =  rcGenQuery( conn, &genQueryInp, &genQueryOut );
        if ( status < 0 ) {
            if ( status != CAT_NO_ROWS_FOUND ) {
                savedStatus = status;
            }
            break;
        }

        if ( ( execId = getSqlResultByInx( genQueryOut, COL_RULE_EXEC_ID ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "qdelUtil: getSqlResultByInx for COL_RULE_EXEC_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            execIdStr = &execId->value[execId->len * i];
            if ( myRodsArgs->verbose ) {
                printf( "Deleting %s\n", execIdStr );
            }
            status = rmDelayedRule( execIdStr );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "qdelUtil: rmDelayedRule %s error. status = %d",
                         execIdStr, status );
                savedStatus = status;
            }
        }
        continueInx = genQueryInp.continueInx = genQueryOut->continueInx;
        freeGenQueryOut( &genQueryOut );
    }
    clearGenQueryInp( &genQueryInp );
    return savedStatus;
}

void usage() {
    printf( "Usage: iqdel [-vVh] ruleId [...]\n" );
    printf( "Usage: iqdel [-a] [-u user]\n" );
    printf( "\n" );
    printf( " iqdel removes delayed rules from the queue.\n" );
    printf( " multiple ruleIds may be included on the command line.\n" );
    printf( " The -a option specifies the removal of all delayed rules\n" );
    printf( " The -u option specifies the removal of all delayed rules of the given user\n" );
    printf( " The -a and -u options cannot be used together\n" );
    printf( "\n" );
    printf( "Also see iqstat and iqmod.\n" );
    printReleaseInfo( "iqdel" );
}
