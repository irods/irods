/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is a regular-user level utility to list the resources.
*/

#include "rods.h"
#include "rodsClient.h"
#include "irods_children_parser.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_resource_constants.hpp"
#include "irods_exception.hpp"

#include <iostream>
#include <vector>

#define BIG_STR 200


// tree drawing gfx
enum DrawingStyle {
    DrawingStyle_ascii = 0,
    DrawingStyle_unicode
};


const std::string middle_child_connector[2] = {"|-- ", "\u251C\u2500\u2500 "};
const std::string last_child_connector[2] = {"`-- ", "\u2514\u2500\u2500 "};
const std::string vertical_pipe[2] = {"|   ", "\u2502   "};
const std::string indent = "    ";


void usage();

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
showResc( char *name, int longOption, const char* zoneArgument, rcComm_t *Conn ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int i2a[20];
    char *condVal[10];
    char v1[BIG_STR];
    int i, status;
    int printCount;
    char *columnNames[] = {
        "resource name", "id", "zone", "type", "class",
        "location",  "vault", "free space", "free space time", "status",
        "info", "comment", "create time", "modify time",
        "context", "parent", "parent context"
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
        i1a[i++] = COL_R_RESC_CONTEXT;
        i1a[i++] = COL_R_RESC_PARENT;
        i1a[i++] = COL_R_RESC_PARENT_CONTEXT;
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

    if ( zoneArgument && *zoneArgument ) {
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
void printDepth( const std::string& depth, DrawingStyle drawing_style ) {
    for ( std::string::const_iterator it=depth.begin(); it!=depth.end(); ++it ) {
        if ( *it == '1' ) {
            std::cout << vertical_pipe[drawing_style];
        } else {
            std::cout << indent;
        }
    }
}

// recursive function to print resource tree
void printRescTree(
     const std::string&                node_name,
     const std::string&                depth,
     const std::vector<std::string>&   resc_types,
     const std::vector<std::string>&   resc_children,
     const std::map<std::string, int>& resc_map,
     DrawingStyle                      drawing_style ) {

    // get children string
    const std::map<std::string, int>::const_iterator it_resc_map = resc_map.find(node_name);
    if (it_resc_map == resc_map.end()) {
        THROW( SYS_INTERNAL_NULL_INPUT_ERR, std::string("Missing node in resc_map: [") + node_name + "]" );
    }

    const int resc_index = it_resc_map->second;
    const std::string& children_str = resc_children[resc_index];

    // print node name, and type if not UFS
    std::cout << node_name << ":" << resc_types[resc_index] << std::endl;

    if ( children_str.empty() ) {
        return;
    }

    // print children
    irods::children_parser parser;
    parser.set_string( children_str );
    irods::children_parser::const_iterator it;
    const irods::children_parser::const_iterator final_it = --parser.end();
    for ( it=parser.begin(); it!=parser.end(); ++it ) {
        if ( it != final_it ) {
            printDepth( depth, drawing_style);
            std::cout << middle_child_connector[drawing_style];
            printRescTree( it->first, depth + "1", resc_types, resc_children, resc_map, drawing_style );
        } else {
            printDepth( depth, drawing_style );
            std::cout << last_child_connector[drawing_style];
            printRescTree( it->first, depth + "0", resc_types, resc_children, resc_map, drawing_style );
        }
    }
    return;
}

int
parseGenQueryOut(
    int                         offset,
    genQueryOut_t*              genQueryOut,
    std::vector<std::string>&   resc_names,
    std::vector<std::string>&   resc_indices,
    std::vector<std::string>&   resc_types,
    std::vector<std::string>&   resc_parents,
    std::map<std::string, int>& resc_map,
    std::vector<std::string>&   roots ) {

    // loop over rows (i.e. for each resource)
    for ( int i=0; i<genQueryOut->rowCnt; ++i ) {
        // get resource name
        char *t_res = genQueryOut->sqlResult[0].value + i * genQueryOut->sqlResult[0].len;
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

        // get resource indicies
        t_res = genQueryOut->sqlResult[2].value + i * genQueryOut->sqlResult[2].len;
        if (t_res) {
            resc_indices.push_back( std::string( t_res ) );
        } else {
            resc_indices.push_back( "" );
        }

        // check if has parent
        t_res = genQueryOut->sqlResult[3].value + i * genQueryOut->sqlResult[3].len;
        if ( !t_res || !strlen( t_res ) ) {
            // another root node
            roots.push_back( resc_names.back() );
            resc_parents.push_back( "" ); // parents must line up with resources
        } else {
            resc_parents.push_back( std::string( t_res ) );
        }
    }

    bool all_parent_strings_are_names_not_ids_aka_queried_a_pre_42_zone = true;
    for (auto& parent : resc_parents) {
        if (!parent.empty()) {
            auto itr = resc_map.find(parent);
            if (itr == resc_map.end()) {
                all_parent_strings_are_names_not_ids_aka_queried_a_pre_42_zone = false;
                break;
            }
        }
    }

    if (all_parent_strings_are_names_not_ids_aka_queried_a_pre_42_zone) {
        for (auto& parent : resc_parents) {
            auto itr = resc_map.find(parent);
            if (itr != resc_map.end()) {
                parent = resc_indices[itr->second];
            }
        }
    }

    return 0;
}

int build_child_list(
    const std::vector<std::string>&   _resc_names,
    const std::vector<std::string>&   _resc_parents,
    const std::vector<std::string>&   _resc_ids,
    std::vector<std::string>&         _resc_children ) {

    for (size_t idx=0; idx < _resc_names.size(); ++idx ) {
        if (idx >= _resc_parents.size() || _resc_parents[idx].empty()) {
            continue;
        }

        size_t parent_pos = std::find(_resc_ids.begin(), _resc_ids.end(), _resc_parents[idx] ) - _resc_ids.begin();
        if (parent_pos >= _resc_ids.size()) {
            continue;
        }

        if (!_resc_children[parent_pos].empty()) {
            _resc_children[parent_pos] += ";";
        }
        _resc_children[parent_pos] += _resc_names[idx];
        _resc_children[parent_pos] += "{}";

    } // for idx

    return 0;

} // build_child_list


int showRescTree( const char *name, const char *zoneArgument, rcComm_t *Conn , DrawingStyle drawing_style) {
    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    genQueryOut_t *genQueryOut = NULL;

    // set up query columns
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_NAME, ORDER_BY );
    addInxIval( &genQueryInp.selectInp, COL_R_TYPE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_PARENT, 1 );

    if ( zoneArgument && *zoneArgument ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    // set up query condition (resc_name != 'bundleResc')
    char collQCond[MAX_NAME_LEN];
    snprintf( collQCond, MAX_NAME_LEN, "!='%s'", BUNDLE_RESC );
    addInxVal( &genQueryInp.sqlCondInp, COL_R_RESC_NAME, collQCond );

    // query for resources
    genQueryInp.maxRows = MAX_SQL_ROWS;
    int status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    // query fail?
    if ( status < 0 ) {
        printError( Conn, status, "rcGenQuery" );
        return status;
    }

    // containers for storing query results
    // one vector per attribute
    std::vector<std::string> resc_names;
    std::vector<std::string> resc_indices;
    std::vector<std::string> resc_types;
    std::vector<std::string> resc_children;
    std::vector<std::string> resc_parents;

    // mapping of resource names and row indexes for looking up children
    std::map<std::string, int> resc_map;

    // list of root resources
    std::vector<std::string> roots;

    // parse results
    int offset = 0;
    status = parseGenQueryOut( offset, genQueryOut, resc_names, resc_indices, resc_types, resc_parents, resc_map, roots );
    while ( !status && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        offset += genQueryOut->rowCnt;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        status = parseGenQueryOut( offset, genQueryOut, resc_names, resc_indices, resc_types, resc_parents, resc_map, roots );
    }

    // child strings assumed to be initialized by printRescTree
    resc_children.resize( resc_names.size() );

    status =  build_child_list( resc_names, resc_parents, resc_indices, resc_children );
    if( status < 0 ) {
        printError( Conn, status, "build_child_list failed" );
        return status;
    }

    // If target resource was specified print tree for that resource
    if ( name && *name ) {
        // check for invalid resource name
        if ( std::find( resc_names.begin(), resc_names.end(), name ) == resc_names.end() ) {
            std::cout << "Resource " << name << " does not exist." << std::endl;
            return USER_INVALID_RESC_INPUT;
        }

        // print tree
        printRescTree( name, "", resc_types, resc_children, resc_map, drawing_style );
    } else {
        // Otherwise print tree for each root node
        for ( std::vector<std::string>::const_iterator it = roots.begin(); it != roots.end(); ++it ) {
            printRescTree( *it, "", resc_types, resc_children, resc_map, drawing_style );
        }
    }

    return status;
}

int
main( int argc, char **argv ) {
    signal( SIGPIPE, SIG_IGN );
    rodsLogLevel( LOG_ERROR );

    rodsArguments_t myRodsArgs;
    int status = parseCmdLineOpt( argc, argv, "hvVlz:Z", 1, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        return 1;
    }

    if ( myRodsArgs.help == True ) {
        usage();
        return 0;
    }

    char zoneArgument[MAX_NAME_LEN + 2] = "";
    if ( myRodsArgs.zone == True ) {
        strncpy( zoneArgument, myRodsArgs.zoneName, MAX_NAME_LEN );
    }

    rodsEnv myEnv;
    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d", status );
        return 1;
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    rErrMsg_t errMsg;
    rcComm_t *Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    try {
        if ( Conn == NULL ) {
            char *mySubName;
            const char *myName = rodsErrorName( errMsg.status, &mySubName );
            rodsLog( LOG_ERROR, "rcConnect failure %s (%s) (%d) %s",
                     myName,
                     mySubName,
                     errMsg.status,
                     errMsg.msg );

            return 2;
        }

        status = clientLogin( Conn );
        if ( status != 0 ) {
            return 3;
        }

        // tree view
        DrawingStyle drawing_style = DrawingStyle_unicode;
        if ( myRodsArgs.longOption != True ) {
            if ( myRodsArgs.ascii == True ) { // character set for printing tree
                drawing_style = DrawingStyle_ascii;
            }
            status = showRescTree( argv[myRodsArgs.optind], zoneArgument, Conn, drawing_style);
        } else { // regular view
            status = showResc( argv[myRodsArgs.optind], myRodsArgs.longOption, zoneArgument, Conn );
        }
    } catch ( const irods::exception& e_ ) {
        rodsLog( LOG_ERROR, "Caught irods::exception\n%s", e_.what() );
        status = e_.code();
    }

    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    /* Exit 0 if one or more items were displayed */
    if ( status >= 0 ) {
        return 0;
    } else {
        return status;
    }
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
    for ( int i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ilsresc" );
}
