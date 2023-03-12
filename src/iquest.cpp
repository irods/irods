#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/lsUtil.h>
#include <irods/parseCommandLine.h>
#include <irods/rcMisc.h>
#include <irods/rodsClient.h>
#include <irods/rodsPath.h>

#include <boost/format.hpp>

#include <iostream>
#include <string>

void usage();


void
usage() {
    char *msgs[] = {
        "Usage: iquest [-hz] [--no-page] [[hint] format_string] <query_string>",
        "  or:  iquest --sql <predefined_sql_string> [format_string] [arguments]",
        "  or:  iquest attrs",
        " ",
        "Options:",
        "  format_string     C-style format string restricted to character strings.",
        "  query_string      selection query string.",
        "                    See the section 'Selection query syntax'.",
        " ",
        "  -z zone_name      the zone to query (default or invalid uses the local zone)",
        "  --no-page         do not prompt to continue after printing a large number of",
        "                    results (256)",
        "  --sql             execute a pre-defined SQL query. The specified query must",
        "                    match one defined by the admin (see 'iadmin h asq' (add",
        "                    specific query)). A few of these may be defined at your",
        "                    site. A special alias 'ls' ('--sql ls') is normally defined",
        "                    to display these. You can specify these via the full SQL or",
        "                    the alias, if defined. Generally, it is better to use the",
        "                    general-query (non --sql forms herein) since that generates",
        "                    the proper SQL (knows how to link the ICAT tables) and",
        "                    handles access control and other aspects of security. If",
        "                    the SQL includes arguments, you enter them following the",
        "                    SQL. As without --sql, you can enter a printf format",
        "                    statement to use in formatting the results (except when",
        "                    using aliases).",
        "  -h                display this help and exit",
        " ",
        "If 'no-distinct' appears before the query_string, the normal distinct option on",
        "the SQL will be bypassed (this is useful in rare cases).",
        " ",
        "To do a case-insensitive search, pass the 'uppercase' (or 'upper') option and",
        "make ALL conditional input values uppercase.  For example:",
        " ",
        "  iquest uppercase \"SELECT DATA_ID WHERE COLL_NAME LIKE '%/TEST' AND DATA_NAME = 'FOO.BIN'\"",
        " ",
        "Case-insensitive search requires that all columns in the WHERE-clause have a",
        "string-based data type (e.g. varchar).  That means, columns such as DATA_SIZE",
        "will cause the query to fail with a CAT_SQL_ERR.",
        " ",
        "Selection query syntax:",
        " Used for 'query_string' argument",
        "> SELECT <attribute> [, <attribute>]* [WHERE <condition> [AND <condition>]*]",
        "  attribute         name of an attribute from 'iquest attrs'",
        "  condition         conditional statement.",
        "                    See the section 'Conditional statement syntax'.",
        " ",
        "One can also use a few aggregation operators such as SUM, COUNT, MIN, MAX, and AVG;",
        "or ORDER and ORDER_DESC (descending) to specify an order (if needed).",
        " ",
        "Conditional statement syntax:",
        " Used for 'WHERE' conditional statements in selection queries",
        "> <attribute> <rel-op> <value>",
        "  attribute         name of an attribute from 'iquest attrs'.",
        "  rel-op            relational operator (e.g. =, <>, >, <, LIKE, NOT LIKE,",
        "                    BETWEEN, etc.)",
        "  value             either a constant or a wild-carded expression.",
        "                    Use % and _ as wild-cards, and use \\ to escape them.",
        " ",
        "Examples:",
        "  iquest \"SELECT DATA_NAME, DATA_CHECKSUM WHERE DATA_RESC_NAME LIKE 'demo%'\"  recorded checksum for each object in a demo resource",
        "  iquest \"For %-12.12s size is %s\" \"SELECT DATA_NAME, DATA_SIZE WHERE COLL_NAME = '/tempZone/home/rods'\"  format object names as a left-justified 12-character column",
        "  iquest \"SELECT COLL_NAME WHERE COLL_NAME LIKE '/tempZone/home/%'\"  home- and sub-colections",
        "  iquest \"User %-6.6s has %-5.5s access to file %s\" \"SELECT USER_NAME, DATA_ACCESS_NAME, DATA_NAME WHERE COLL_NAME = '/tempZone/home/rods'\"  user access for each object in the rods home collection",
        "  iquest \" %-5.5s access has been given to user %-6.6s for the file %s\" \"SELECT DATA_ACCESS_NAME, USER_NAME, DATA_NAME WHERE COLL_NAME = '/tempZone/home/rods'\"  alternate formatting for user access",
        "  iquest no-distinct \"SELECT META_DATA_ATTR_NAME\"  allow multiple instances of the same result",
        "  iquest \"SELECT RESC_NAME, RESC_LOC, RESC_VAULT_PATH, DATA_PATH WHERE DATA_NAME = 't2' AND COLL_NAME = '/tempZone/home/rods'\"  vault storage for a single object",
        "  iquest \"User %-9.9s uses %14.14s bytes in %8.8s files in '%s'\" \"SELECT USER_NAME, SUM(DATA_SIZE), COUNT(DATA_NAME), RESC_NAME\"  storage used for each user on each resource",
        "  iquest \"SELECT SUM(DATA_SIZE) WHERE COLL_NAME = '/tempZone/home/rods'\"  aggregate size of objects in the immediate rods home collection",
        "  iquest \"SELECT SUM(DATA_SIZE) WHERE COLL_NAME LIKE '/tempZone/home/rods%'\"  aggregate size of rods home- and sub-collections",
        "  iquest \"SELECT SUM(DATA_SIZE), RESC_NAME WHERE COLL_NAME LIKE '/tempZone/home/rods%'\"  aggregate size of rods home- and sub-collections for each resource",
        "  iquest \"SELECT ORDER_DESC(DATA_ID) WHERE COLL_NAME LIKE '/tempZone/home/rods%'\"  ordered list of object IDs in rods home- and sub-collections",
        "  iquest \"SELECT COUNT(DATA_ID) WHERE COLL_NAME LIKE '/tempZone/home/rods%'\"  count objects in rods home- and sub-collections",
        "  iquest \"SELECT RESC_NAME WHERE RESC_CLASS_NAME IN ('bundle','archive')\"  bundle and archive resources",
        "  iquest \"SELECT DATA_NAME, DATA_SIZE WHERE DATA_SIZE BETWEEN '100000' '100200'\"  objects between 100kB and 100.2kB",
        "  iquest \"%s/%s %s\" \"SELECT COLL_NAME, DATA_NAME, DATA_CREATE_TIME WHERE COLL_NAME LIKE '/tempZone/home/rods%' AND DATA_CREATE_TIME LIKE '01508165%'\"  objects in rods home- and sub-collections created 2017-10-16 between UTC 14:43 and 15:00",
        "  iquest \"%s/%s\" \"SELECT COLL_NAME, DATA_NAME WHERE DATA_RESC_NAME = 'replResc' and DATA_REPL_STATUS = '0'\"  objects replicated successfully to replResc",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "iquest" );
}

void
printFormatted( char *format, char *args[], int nargs ) {
    try {
        boost::format formatter( format );
        for ( int i = 0; i < nargs; i++ ) {
            formatter % args[i];
        }
        std::cout << formatter;
    }
    catch ( const boost::io::format_error& _e ) {
        std::cout << _e.what() << std::endl;
    }
}

void
printBasicGenQueryOut( genQueryOut_t *genQueryOut, char *format ) {
    int i, j;
    if ( format == NULL || strlen( format ) == 0 ) {
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            if ( i > 0 ) {
                printf( "----\n" );
            }
            for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                char *tResult;
                tResult = genQueryOut->sqlResult[j].value;
                tResult += i * genQueryOut->sqlResult[j].len;
                printf( "%s\n", tResult );
            }
        }
    }
    else {
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            char *results[20];
            for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                char *tResult;
                tResult = genQueryOut->sqlResult[j].value;
                tResult += i * genQueryOut->sqlResult[j].len;
                results[j] = tResult;
            }

            printFormatted( format, results, j );
        }
    }
}

int
queryAndShowStrCond( rcComm_t *conn, char *hint, char *format,
                     char *selectConditionString, int noDistinctFlag,
                     int upperCaseFlag, char *zoneArgument, int noPageFlag ) {
    /*
      NoDistinctFlag is 1 if the user is requesting 'distinct' to be skipped.
     */

    genQueryInp_t genQueryInp;
    int i;
    genQueryOut_t *genQueryOut = NULL;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    i = fillGenQueryInpFromStrCond( selectConditionString, &genQueryInp );
    if ( i < 0 ) {
        return i;
    }

    if ( noDistinctFlag ) {
        genQueryInp.options = NO_DISTINCT;
    }
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    if ( zoneArgument != 0 && zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
        printf( "Zone is %s\n", zoneArgument );
    }

    genQueryInp.maxRows = MAX_SQL_ROWS;
    genQueryInp.continueInx = 0;
    i = rcGenQuery( conn, &genQueryInp, &genQueryOut );
    if ( i < 0 ) {
        return i;
    }

    i = printGenQueryOut( stdout, format, hint,  genQueryOut );
    if ( i < 0 ) {
        return i;
    }


    while ( i == 0 && genQueryOut->continueInx > 0 ) {
        if ( noPageFlag == 0 ) {
            char inbuf[100];
            printf( "Continue? [Y/n]" );
            std::string response = "";
            getline( std::cin, response );
            strncpy( inbuf, response.c_str(), 90 );
            if ( strncmp( inbuf, "n", 1 ) == 0 ) {
                break;
            }
        }
        genQueryInp.continueInx = genQueryOut->continueInx;
        i = rcGenQuery( conn, &genQueryInp, &genQueryOut );
        if ( i < 0 ) {
            if (i==CAT_NO_ROWS_FOUND) {
                return 0;
            } else {
                return i;
            }
        }
        i = printGenQueryOut( stdout, format, hint,  genQueryOut );
        if ( i < 0 ) {
            return i;
        }
    }

    return 0;

}

int
execAndShowSpecificQuery( rcComm_t *conn, char *sql,
                          char *args[], int argsOffset, int noPageFlag, char* zoneArgument ) {
    specificQueryInp_t specificQueryInp;
    int status, i;
    genQueryOut_t *genQueryOut = NULL;
    char *cp;
    int nQuestionMarks, nArgs;
    char *format = "";
    char myFormat[300] = "";

    memset( &specificQueryInp, 0, sizeof( specificQueryInp_t ) );
    specificQueryInp.maxRows = MAX_SQL_ROWS;
    specificQueryInp.continueInx = 0;
    specificQueryInp.sql = sql;

    if ( zoneArgument != 0 && zoneArgument[0] != '\0' ) {
        addKeyVal( &specificQueryInp.condInput, ZONE_KW, zoneArgument );
        printf( "Zone is %s\n", zoneArgument );
    }



    /* To differentiate format from args, count the ? in the SQL and the
       arguments */
    cp = specificQueryInp.sql;
    nQuestionMarks = 0;
    while ( *cp != '\0' ) {
        if ( *cp++ == '?' ) {
            nQuestionMarks++;
        }
    }
    i = argsOffset;
    nArgs = 0;
    while ( args[i] != NULL && strlen( args[i] ) > 0 ) {
        nArgs++;
        i++;
    }
    /* If the SQL is an alias, counting the ?'s won't be accurate so now
       the following is only done if nQuestionMarks is > 0.  But this means
       iquest won't be able to notice a Format statement when using aliases,
       but will instead assume all are parameters to the SQL. */
    if ( nQuestionMarks > 0 && nArgs > nQuestionMarks ) {
        format = args[argsOffset];  /* this must be the format */
        argsOffset++;
        strncpy( myFormat, format, 300 - 10 );
        strcat( myFormat, "\n" ); /* since \n is difficult to pass in
				on the command line, add one by default */
    }

    i = 0;
    while ( args[argsOffset] != NULL && strlen( args[argsOffset] ) > 0 ) {
        specificQueryInp.args[i++] = args[argsOffset];
        argsOffset++;
    }
    status = rcSpecificQuery( conn, &specificQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        printf( "CAT_NO_ROWS_FOUND: Nothing was found matching your query\n" );
        return CAT_NO_ROWS_FOUND;
    }
    if ( status < 0 ) {
        printError( conn, status, "rcSpecificQuery" );
        return status;
    }

    printBasicGenQueryOut( genQueryOut, myFormat );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        if ( noPageFlag == 0 ) {
            char inbuf[100];
            printf( "Continue? [Y/n]" );
            std::string response = "";
            getline( std::cin, response );
            strncpy( inbuf, response.c_str(), 90 );
            if ( strncmp( inbuf, "n", 1 ) == 0 ) {
                break;
            }
        }
        specificQueryInp.continueInx = genQueryOut->continueInx;
        status = rcSpecificQuery( conn, &specificQueryInp, &genQueryOut );
        if ( status < 0 ) {
            printError( conn, status, "rcSpecificQuery" );
            return status;
        }
        printBasicGenQueryOut( genQueryOut, myFormat );
    }

    return 0;

}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    char *optStr;
    int noDistinctFlag = 0;
    int upperCaseFlag = 0;

    optStr = "hz:Z";

    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs );

    if ( myRodsArgs.optind < argc ) {
        if ( !strcmp( argv[myRodsArgs.optind], "no-distinct" ) ) {
            noDistinctFlag = 1;
            myRodsArgs.optind++;
        }
    }
    if ( myRodsArgs.optind < argc ) {
        if ( strncmp( argv[myRodsArgs.optind], "upper", 5 ) == 0 ) {
            upperCaseFlag = 1;
            myRodsArgs.optind++;
        }
    }


    if ( status < 0 ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    if ( myRodsArgs.optind == argc ) {
        printf( "StringCondition needed\n" );
        usage();
        exit( 0 );
    }


    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    if ( myRodsArgs.optind == 1 ) {
        if ( !strncmp( argv[argc - 1], "attrs", 5 ) ) {
            showAttrNames();
            exit( 0 );
        }
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }

    const auto disconnect = irods::at_scope_exit{[conn] { rcDisconnect(conn); }};

    status = clientLogin( conn );
    if ( status != 0 ) {
        return 3;
    }

    if ( myRodsArgs.sql ) {
        status = execAndShowSpecificQuery( conn, argv[myRodsArgs.optind],
                                           argv,
                                           myRodsArgs.optind + 1,
                                           myRodsArgs.noPage,
                                           myRodsArgs.zoneName );
        if ( status == CAT_NO_ROWS_FOUND ) {
            return 1;
        }
        else if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status, "iquest Error: specificQuery (sql-query) failed" );
            return 4;
        }
        else {
            return 0;
        }
    }

    if ( myRodsArgs.optind == ( argc - 3 ) ) {
        status = queryAndShowStrCond( conn, argv[argc - 3],
                                      argv[argc - 2], argv[argc - 1],
                                      noDistinctFlag, upperCaseFlag,
                                      myRodsArgs.zoneName,
                                      myRodsArgs.noPage );
    }
    else if ( myRodsArgs.optind == ( argc - 2 ) ) {
        status = queryAndShowStrCond( conn, NULL, argv[argc - 2], argv[argc - 1],
                                      noDistinctFlag, upperCaseFlag,
                                      myRodsArgs.zoneName,
                                      myRodsArgs.noPage );
    }
    else {
        status = queryAndShowStrCond( conn, NULL, NULL, argv[argc - 1],
                                      noDistinctFlag, upperCaseFlag,
                                      myRodsArgs.zoneName,
                                      myRodsArgs.noPage );
    }

    if ( status < 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            printf( "CAT_NO_ROWS_FOUND: Nothing was found matching your query\n" );
            return 1;
        }
        else {
            rodsLogError( LOG_ERROR, status, "iquest Error: queryAndShowStrCond failed" );
            return 4;
        }
    }
    else {
        return 0;
    }
}
