/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is a regular-user level utility to list the resources.
*/

#include "rods.hpp"
#include "rodsClient.hpp"
#include "irods_children_parser.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_resource_constants.hpp"

#include <iostream>
#include <vector>

#define MAX_SQL 300
#define BIG_STR 200

rcComm_t *Conn;

char zoneArgument[MAX_NAME_LEN + 2] = "";


// =-=-=-=-=-=-=-
// tree stuff

// containers for storing query results
// one vector per attribute
std::vector<std::string> resc_names;
std::vector<std::string> resc_types;
std::vector<std::string> resc_children;

// mapping of resource names and row indexes for looking up children
std::map<std::string, int> resc_map;

// list of root resources
std::vector<std::string> roots;

// tree drawing gfx
std::string middle_child_connector[2] = {"|-- ", "\u251C\u2500\u2500 "};
std::string last_child_connector[2] = {"`-- ", "\u2514\u2500\u2500 "};
std::string vertical_pipe[2] = {"|   ", "\u2502   "};
std::string indent = "    ";
int gfx_mode = 1; // 0 for ascii, 1 for unicode

// =-=-=-=-=-=-=-


void usage();

/*
 print the results of a general query.
 */
int
printGenQueryResults( rcComm_t *Conn, int status, genQueryOut_t *genQueryOut,
                      char *descriptions[], int doDashes ) {
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
                if ( i > 0 && doDashes ) {
                    printf( "----\n" );
                }
                for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[j].value;
                    tResult += i * genQueryOut->sqlResult[j].len;
                    if ( *descriptions[j] != '\0' ) {
                        if ( strstr( descriptions[j], "time" ) != 0 ) {
                            getLocalTimeFromRodsTime( tResult, localTime );
                            printf( "%s: %s: %s\n", descriptions[j], tResult,
                                    localTime );
                        }
                        else {
                            printf( "%s: %s\n", descriptions[j], tResult );
                        }
                    }
                    else {
                        printf( "%s\n", tResult );
                    }
                    printCount++;
                }
            }
        }
    }
    return printCount;
}


/*
Via a general query, show a resource
*/
int
showResc( char *name, int longOption ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int i2a[20];
    char *condVal[10];
    char v1[BIG_STR];
    int i, status;
    int printCount;
    char *columnNames[] = {"resource name", "id", "zone", "type", "class",
                           "location",  "vault", "free space", "free space time", "status",
                           "info", "comment", "create time", "modify time", "children",
                           "context", "parent", "object count"
                          };

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    printCount = 0;

    i = 0;
    i1a[i++] = COL_R_RESC_NAME;
    if ( longOption ) {
        i1a[i++] = COL_R_RESC_ID;
        i1a[i++] = COL_R_ZONE_NAME;
        i1a[i++] = COL_R_TYPE_NAME;
        i1a[i++] = COL_R_CLASS_NAME;
        i1a[i++] = COL_R_LOC;
        i1a[i++] = COL_R_VAULT_PATH;
        i1a[i++] = COL_R_FREE_SPACE;
        i1a[i++] = COL_R_FREE_SPACE_TIME;
        i1a[i++] = COL_R_RESC_STATUS;
        i1a[i++] = COL_R_RESC_INFO;
        i1a[i++] = COL_R_RESC_COMMENT;
        i1a[i++] = COL_R_CREATE_TIME;
        i1a[i++] = COL_R_MODIFY_TIME;
        i1a[i++] = COL_R_RESC_CHILDREN;
        i1a[i++] = COL_R_RESC_CONTEXT;
        i1a[i++] = COL_R_RESC_PARENT;
        i1a[i++] = COL_R_RESC_OBJCOUNT;

    }
    else {
        columnNames[0] = "";
    }

    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    if ( name != NULL && *name != '\0' ) {
        // =-=-=-=-=-=-=-
        // JMC - backport 4629
        if ( strncmp( name, BUNDLE_RESC, sizeof( BUNDLE_RESC ) ) == 0 ) {
            printf( "%s is a pseudo resource for system use only.\n",
                    BUNDLE_RESC );
            return 0;
        }
        // =-=-=-=-=-=-=-
        i2a[0] = COL_R_RESC_NAME;
        snprintf( v1, BIG_STR, "='%s'", name );
        condVal[0] = v1;
        genQueryInp.sqlCondInp.len = 1;
    }
    else {
        // =-=-=-=-=-=-=-
        // JMC - backport 4629
        i2a[0] = COL_R_RESC_NAME;
        sprintf( v1, "!='%s'", BUNDLE_RESC ); /* all but bundleResc */
        condVal[0] = v1;
        genQueryInp.sqlCondInp.len = 1;
        // =-=-=-=-=-=-=-
    }

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_R_RESC_INFO;
        genQueryInp.selectInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "None\n" );
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            if ( name != NULL && name[0] != '\0' ) {
                printf( "Resource %s does not exist.\n", name );
            }
            else {
                printf( "Resource does not exist.\n" );
            }
            return 0;
        }
    }

    printCount += printGenQueryResults( Conn, status, genQueryOut, columnNames,
                                        longOption );
    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 && longOption ) {
            printf( "----\n" );
        }
        printCount += printGenQueryResults( Conn, status, genQueryOut,
                                            columnNames, longOption );
    }

    return 1;
}


// depth is a binary string of open nodes
void printDepth( const std::string& depth ) {
    for ( std::string::const_iterator it = depth.begin(); it != depth.end(); ++it ) {
        if ( *it == '1' ) {
            std::cout << vertical_pipe[gfx_mode];
        }
        else {
            std::cout << indent;
        }
    }
}


// recursive function to print resource tree
void printRescTree( const std::string& node_name, std::string depth ) {
    int resc_index;

    // get children string
    resc_index = resc_map[node_name];
    std::string& children_str = resc_children[resc_index];

    // print node name, and type if not UFS
    if ( resc_types[resc_index] != irods::RESOURCE_TYPE_NATIVE ) {
        std::cout << node_name << ":" << resc_types[resc_index] << std::endl;
    }
    else {
        std::cout << node_name << std::endl;

    }

    // if leaf we're done
    if ( children_str.empty() ) {
        return;
    }


    // print children
    irods::children_parser parser;
    parser.set_string( children_str );
    irods::children_parser::const_iterator it, final_it = parser.end();
    final_it--;
    for ( it = parser.begin(); it != parser.end(); ++it ) {
        if ( it != final_it ) {
            // print depth string
            printDepth( depth );

            // print middle child connector
            std::cout << middle_child_connector[gfx_mode];

            // append '1' to depth string and print child
            printRescTree( it->first, depth + "1" );
        }
        else {
            // print depth string
            printDepth( depth );

            // print last child connector
            std::cout << last_child_connector[gfx_mode];

            // append '0' to depth string and print child
            printRescTree( it->first, depth + "0" );
        }
    }
    return;
}


int parseGenQueryOut( int offset, genQueryOut_t *genQueryOut ) {
    char* t_res;	// target result

    // loop over rows (i.e. for each resource)
    for ( int i = 0; i < genQueryOut->rowCnt; i++ ) {

        // get resource name
        t_res = genQueryOut->sqlResult[0].value + i * genQueryOut->sqlResult[0].len;
        if ( !t_res || !strlen( t_res ) ) {
            // parsing error
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }
        resc_names.push_back( std::string( t_res ) );

        // map resource name to row index
        resc_map[resc_names.back()] = i + offset;

        // get resource type
        t_res = genQueryOut->sqlResult[1].value + i * genQueryOut->sqlResult[1].len;
        resc_types.push_back( std::string( t_res ) );

        // get resource children
        t_res = genQueryOut->sqlResult[2].value + i * genQueryOut->sqlResult[2].len;
        resc_children.push_back( std::string( t_res ) );

        // check if has parent
        t_res = genQueryOut->sqlResult[3].value + i * genQueryOut->sqlResult[3].len;
        if ( !t_res || !strlen( t_res ) ) {
            // another root node
            roots.push_back( resc_names.back() );
        }

    }

    return 0;
}



int showRescTree( char *name ) {
    int status;
    int offset = 0; // when getting more rows

    // genQuery input and output (camelCase for tradition)
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    char collQCond[MAX_NAME_LEN];


    // start clean
    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    // set up query columns
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_NAME, ORDER_BY );
    addInxIval( &genQueryInp.selectInp, COL_R_TYPE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_CHILDREN, 1 );
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_PARENT, 1 );


    // set up query condition (resc_name != 'bundleResc')
    snprintf( collQCond, MAX_NAME_LEN, "!='%s'", BUNDLE_RESC );
    addInxVal( &genQueryInp.sqlCondInp, COL_R_RESC_NAME, collQCond );

    // query for resources
    genQueryInp.maxRows = MAX_SQL_ROWS;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    // query fail?
    if ( status < 0 ) {
        printError( Conn, status, "rcGenQuery" );
        return status;
    }


    // parse results
    status = parseGenQueryOut( offset, genQueryOut );



    // More rows in the pipeline?
    while ( !status && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;

        // update offset
        offset += genQueryOut->rowCnt;

        // get next batch of rows
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

        // parse results
        status = parseGenQueryOut( offset, genQueryOut );
    }



    // If target resource was specified print tree for that resource
    if ( name && strlen( name ) ) {
        std::string target_resc_name( name );

        // check for invalid resource name
        if ( std::find( resc_names.begin(), resc_names.end(), target_resc_name ) == resc_names.end() ) {
            std::cout << "Resource " << target_resc_name << " does not exist." << std::endl;
            return USER_INVALID_RESC_INPUT;
        }

        // print tree
        printRescTree( target_resc_name, "" );
    }
    else {
        // Otherwise print tree for each root node
        for ( std::vector<std::string>::const_iterator it = roots.begin(); it != roots.end(); ++it ) {
            printRescTree( *it, "" );
        }
    }

    return status;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rErrMsg_t errMsg;

    rodsArguments_t myRodsArgs;

    char *mySubName;
    char *myName;

    rodsEnv myEnv;


    rodsLogLevel( LOG_ERROR );

    status = parseCmdLineOpt( argc, argv, "hvVlz:Z", 1, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    if ( myRodsArgs.zone == True ) {
        strncpy( zoneArgument, myRodsArgs.zoneName, MAX_NAME_LEN );
    }

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
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
        exit( 3 );
    }

    // tree view
    if ( myRodsArgs.tree == True ) {
        if ( myRodsArgs.ascii == True ) { // character set for printing tree
            gfx_mode = 0;
        }

        status = showRescTree( argv[myRodsArgs.optind] );
    }
    else { // regular view
        status = showResc( argv[myRodsArgs.optind], myRodsArgs.longOption );
    }

    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    /* Exit 0 if one or more items were displayed */
    if ( status > 0 ) {
        exit( 0 );
    }
    exit( 4 );
}

/*
Print the main usage/help information.
 */
void usage() {
    char *msgs[] = {
        "ilsresc lists iRODS resources",
        "Usage: ilsresc [-lvVh] [--tree] [--ascii] [Name]",
        "If Name is present, list only that resource, ",
        "otherwise list them all ",
        "Options are:",
        " -l Long format - list details",
        " -v verbose",
        " -V Very verbose",
        " -z Zonename  list resources of specified Zone",
        " --tree - tree view",
        " --ascii - use ascii character set to build tree view (ignored without --tree)",
        " -h This help",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ilsresc" );
}

