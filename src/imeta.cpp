/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is an interface to the Attribute-Value-Units type of metadata.
*/

#include "rods.h"
#include "rodsClient.h"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "boost/program_options.hpp"

#define MAX_SQL 300
#define BIG_STR 3000
#define MAX_CMD_TOKENS 40

char cwd[BIG_STR];

int longMode = 0; /* more detailed listing */
int upperCaseFlag = 0;

char zoneArgument[MAX_NAME_LEN + 2] = "";

rcComm_t *Conn;
rodsEnv myEnv;

int lastCommandStatus = 0;
int printCount = 0;

int usage( char *subOpt );

int do_command( 
    std::vector< std::string >              _input,
    boost::program_options::variables_map&  _vm );

int do_interactive( 
    boost::program_options::variables_map&  _vm );
/*
 print the results of a general query.
 */
void
printGenQueryResults( rcComm_t *Conn, int status, genQueryOut_t *genQueryOut,
                      char *descriptions[] ) {
    int i, j;
    char localTime[TIME_LEN];
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
                if ( i > 0 ) {
                    printf( "----\n" );
                }
                for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[j].value;
                    tResult += i * genQueryOut->sqlResult[j].len;
                    if ( *descriptions[j] != '\0' ) {
                        if ( strstr( descriptions[j], "time" ) != 0 ) {
                            getLocalTimeFromRodsTime( tResult, localTime );
                            printf( "%s: %s\n", descriptions[j],
                                    localTime );
                        }
                        else {
                            printf( "%s: %s\n", descriptions[j], tResult );
                            printCount++;
                        }
                    }
                }
            }
        }
    }
}

/*
 Via a general query and show the AVUs for a dataobject.
 */
int
showDataObj( char *name, char *attrName, int wild ) {
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    char fullName[MAX_NAME_LEN];
    char myDirName[MAX_NAME_LEN];
    char myFileName[MAX_NAME_LEN];
    int status;
    /* Fourth 'time set' column is only used in longMode */
    char *columnNames[] = { "attribute", "value", "units", "time set" };    

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    printf( "AVUs defined for dataObj %s:\n", name );
    printCount = 0;
    i1a[0] = COL_META_DATA_ATTR_NAME;
    i1b[0] = 0;
    i1a[1] = COL_META_DATA_ATTR_VALUE;
    i1b[1] = 0;
    i1a[2] = COL_META_DATA_ATTR_UNITS;
    i1b[2] = 0;
    if ( longMode ) {
        i1a[3] = COL_META_DATA_MODIFY_TIME;
        i1b[3] = 0;
    }
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 3;
    if ( longMode ) {
        genQueryInp.selectInp.len = 4;
    }

    i2a[0] = COL_COLL_NAME;
    std::string v1;
    v1 = "='";
    v1 += cwd;
    v1 += "'";

    i2a[1] = COL_DATA_NAME;
    std::string v2;
    v2 = "='";
    v2 += name;
    v2 += "'";

    if ( *name == '/' ) {
        snprintf( fullName, sizeof( fullName ), "%s", name );
    }
    else {
        snprintf( fullName, sizeof( fullName ), "%s/%s", cwd, name );
    }

    if ( strstr( name, "/" ) != NULL ) {
        /* reset v1 and v2 for when full path or relative path entered */
        if ( int status = splitPathByKey( fullName, myDirName,
                                          MAX_NAME_LEN, myFileName, MAX_NAME_LEN, '/' ) ) {
            rodsLog( LOG_ERROR, "splitPathByKey failed in showDataObj with status %d", status );
        }

        v1 = "='";
        v1 += myDirName;
        v1 += "'";

        v2 = "='";
        v2 += myFileName;
        v2 += "'";
    }

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    std::string v3;
    if ( attrName != NULL && *attrName != '\0' ) {
        i2a[2] = COL_META_DATA_ATTR_NAME;
        if ( wild ) {
            v3  = "like '";
            v3 += attrName;
            v3 += "'";
        }
        else {
            v3  = "='";
            v3 += attrName;
            v3 += "'";
        }
        condVal[2] = const_cast<char*>( v3.c_str() );
        genQueryInp.sqlCondInp.len++;
    }

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_D_DATA_PATH;
        genQueryInp.selectInp.len = 1;
        genQueryInp.sqlCondInp.len = 2;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "None\n" );
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            lastCommandStatus = status;
            printf( "Dataobject %s does not exist\n", fullName );
            printf( "or, if 'strict' access control is enabled, you may not have access.\n" );
            return 0;
        }
        printGenQueryResults( Conn, status, genQueryOut, columnNames );
    }
    else {
        printGenQueryResults( Conn, status, genQueryOut, columnNames );
    }

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printGenQueryResults( Conn, status, genQueryOut,
                              columnNames );
    }

    return 0;
}

/*
Via a general query, show the AVUs for a collection
*/
int
showColl( char *name, char *attrName, int wild ) {
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    char fullName[MAX_NAME_LEN];
    int  status;
    char *columnNames[] = {"attribute", "value", "units"};

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    printf( "AVUs defined for collection %s:\n", name );
    printCount = 0;
    i1a[0] = COL_META_COLL_ATTR_NAME;
    i1b[0] = 0; /* currently unused */
    i1a[1] = COL_META_COLL_ATTR_VALUE;
    i1b[1] = 0;
    i1a[2] = COL_META_COLL_ATTR_UNITS;
    i1b[2] = 0;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 3;

    strncpy( fullName, cwd, MAX_NAME_LEN );
    if ( strlen( name ) > 0 ) {
        if ( *name == '/' ) {
            strncpy( fullName, name, MAX_NAME_LEN );
        }
        else {
            rstrcat( fullName, "/", MAX_NAME_LEN );
            rstrcat( fullName, name, MAX_NAME_LEN );
        }
    }

    // JMC cppcheck - dangerous use of strcpy : need a explicit null term
    // NOTE :: adding len of name + 1 for added / + len of cwd + 1 for null term
    if ( ( strlen( name ) + 1 + strlen( cwd ) + 1 ) < LONG_NAME_LEN ) {
        fullName[ strlen( name ) + 1 + strlen( cwd ) + 1 ] = '\0';
    }
    else {
        rodsLog( LOG_ERROR, "showColl :: error - fullName could not be explicitly null terminated" );
    }

    i2a[0] = COL_COLL_NAME;
    std::string v1;
    v1 =  "='";
    v1 += fullName;
    v1 += "'";

    condVal[0] = const_cast<char*>( v1.c_str() );

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    std::string v2;
    if ( attrName != NULL && *attrName != '\0' ) {
        i2a[1] = COL_META_COLL_ATTR_NAME;
        if ( wild ) {
            v2 =  "like '";
            v2 += attrName;
            v2 += "'";
        }
        else {
            v2 =  "= '";
            v2 += attrName;
            v2 += "'";
        }
        condVal[1] = const_cast<char*>( v2.c_str() );
        genQueryInp.sqlCondInp.len++;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_COLL_COMMENTS;
        genQueryInp.selectInp.len = 1;
        genQueryInp.sqlCondInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "None\n" );
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            lastCommandStatus = status;
            printf( "Collection %s does not exist.\n", fullName );
            return 0;
        }
    }

    printGenQueryResults( Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printGenQueryResults( Conn, status, genQueryOut,
                              columnNames );
    }

    return 0;
}

/*
Via a general query, show the AVUs for a resource
*/
int
showResc( char *name, char *attrName, int wild ) {
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    int  status;
    char *columnNames[] = {"attribute", "value", "units"};

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    printf( "AVUs defined for resource %s:\n", name );
    printCount = 0;
    i1a[0] = COL_META_RESC_ATTR_NAME;
    i1b[0] = 0; /* currently unused */
    i1a[1] = COL_META_RESC_ATTR_VALUE;
    i1b[1] = 0;
    i1a[2] = COL_META_RESC_ATTR_UNITS;
    i1b[2] = 0;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 3;

    i2a[0] = COL_R_RESC_NAME;
    std::string v1;
    v1 =  "='";
    v1 += name;
    v1 += "'";
    condVal[0] = const_cast<char*>( v1.c_str() );

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    std::string v2;
    if ( attrName != NULL && *attrName != '\0' ) {
        i2a[1] = COL_META_RESC_ATTR_NAME;
        if ( wild ) {
            v2 =  "like '";
            v2 += attrName;
            v2 += "'";
        }
        else {
            v2 =  "= '";
            v2 += attrName;
            v2 += "'";
        }
        condVal[1] = const_cast<char*>( v2.c_str() );
        genQueryInp.sqlCondInp.len++;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_R_RESC_INFO;
        genQueryInp.selectInp.len = 1;
        genQueryInp.sqlCondInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "None\n" );
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            lastCommandStatus = status;
            printf( "Resource %s does not exist.\n", name );
            return 0;
        }
    }

    printGenQueryResults( Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printGenQueryResults( Conn, status, genQueryOut,
                              columnNames );
    }

    return 0;
}

/*
Via a general query, show the AVUs for a user
*/
int
showUser( char *name, char *attrName, int wild ) {
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];
    int status;
    char *columnNames[] = {"attribute", "value", "units"};

    char userName[NAME_LEN];
    char userZone[NAME_LEN];

    status = parseUserName( name, userName, userZone );
    if ( status ) {
        printf( "Invalid username format\n" );
        return 0;
    }
    if ( userZone[0] == '\0' ) {
        snprintf( userZone, sizeof( userZone ), "%s", myEnv.rodsZone );
    }

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    printf( "AVUs defined for user %s#%s:\n", userName, userZone );
    printCount = 0;
    i1a[0] = COL_META_USER_ATTR_NAME;
    i1b[0] = 0; /* currently unused */
    i1a[1] = COL_META_USER_ATTR_VALUE;
    i1b[1] = 0;
    i1a[2] = COL_META_USER_ATTR_UNITS;
    i1b[2] = 0;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 3;

    i2a[0] = COL_USER_NAME;
    std::string v1;
    v1 =  "='";
    v1 += userName;
    v1 += "'";

    i2a[1] = COL_USER_ZONE;
    std::string v2;
    v2 =  "='";
    v2 += userZone;
    v2 += "'";

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    std::string v3;
    if ( attrName != NULL && *attrName != '\0' ) {
        i2a[2] = COL_META_USER_ATTR_NAME;
        if ( wild ) {
            v3 =  "like '";
            v3 += attrName;
            v3 += "'";
        }
        else {
            v3 =  "= '";
            v3 += attrName;
            v3 += "'";
        }
        condVal[2] = const_cast<char*>( v3.c_str() );
        genQueryInp.sqlCondInp.len++;
    }

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_USER_COMMENT;
        genQueryInp.selectInp.len = 1;
        genQueryInp.sqlCondInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "None\n" );
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            lastCommandStatus = status;
            printf( "User %s does not exist.\n", name );
            return 0;
        }
    }

    printGenQueryResults( Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printGenQueryResults( Conn, status, genQueryOut,
                              columnNames );
    }

    return 0;
}

/*
Do a query on AVUs for dataobjs and show the results
attribute op value [AND attribute op value] [REPEAT]
 */
int queryDataObj( char *cmdToken[] ) {
    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20];
    int i2a[20];
    char *condVal[20];

    int status;
    char *columnNames[] = {"collection", "dataObj"};
    int cmdIx;
    int condIx;

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    printCount = 0;
    i1a[0] = COL_COLL_NAME;
    i1b[0] = 0; /* (unused) */
    i1a[1] = COL_DATA_NAME;
    i1b[1] = 0;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 2;

    i2a[0] = COL_META_DATA_ATTR_NAME;
    std::string v1;
    v1 =  "='";
    v1 += cmdToken[2];
    v1 += "'";

    i2a[1] = COL_META_DATA_ATTR_VALUE;
    std::string v2;
    v2 =  cmdToken[3];
    v2 += " '";
    v2 += cmdToken[4];
    v2 += "'";

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    if ( strcmp( cmdToken[5], "or" ) == 0 ) {
        std::stringstream s;
        s << "|| " << cmdToken[6] << " '" << cmdToken[7] << "'";
        v2 += s.str();
    }

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    cmdIx = 5;
    condIx = 2;
    std::vector<std::string> vstr( condIx );
    while ( strcmp( cmdToken[cmdIx], "and" ) == 0 ) {
        i2a[condIx] = COL_META_DATA_ATTR_NAME;
        cmdIx++;
        std::stringstream s1;
        s1 << "='" << cmdToken[cmdIx] << "'";
        vstr.push_back( s1.str() );
        condVal[condIx] = const_cast<char*>( vstr.back().c_str() );
        condIx++;

        i2a[condIx] = COL_META_DATA_ATTR_VALUE;
        std::stringstream s2;
        s2 << cmdToken[cmdIx + 1] << " '" << cmdToken[cmdIx + 2] << "'";
        vstr.push_back( s2.str() );
        cmdIx += 3;
        condVal[condIx] = const_cast<char*>( vstr.back().c_str() );
        condIx++;
        genQueryInp.sqlCondInp.len += 2;
    }

    std::cout << "XXXXX cmdToken[cmdIx] = " << *cmdToken[cmdIx] << std::endl;

    if ( *cmdToken[cmdIx] != '\0' ) {
        printf( "Unrecognized input\n" );
        return -2;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    printGenQueryResults( Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printGenQueryResults( Conn, status, genQueryOut,
                              columnNames );
    }

    return 0;
}

/*
Do a query on AVUs for collections and show the results
 */
int queryCollection( char *cmdToken[] ) {
    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20];
    int i2a[20];
    char *condVal[20];

    int status;
    char *columnNames[] = {"collection"};
    int cmdIx;
    int condIx;

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    printCount = 0;
    i1a[0] = COL_COLL_NAME;
    i1b[0] = 0; /* (unused) */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_META_COLL_ATTR_NAME;
    std::string v1;
    v1 =  "='";
    v1 += cmdToken[2];
    v1 += "'";

    i2a[1] = COL_META_COLL_ATTR_VALUE;
    std::string v2;
    v2 =  cmdToken[3];
    v2 += " '";
    v2 += cmdToken[4];
    v2 += "'";

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    if ( strcmp( cmdToken[5], "or" ) == 0 ) {
        std::stringstream s;
        s << "|| " << cmdToken[6] << " '" << cmdToken[7] << "'";
        v2 += s.str();
    }

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    cmdIx = 5;
    condIx = 2;
    std::vector<std::string> vstr( condIx );
    while ( strcmp( cmdToken[cmdIx], "and" ) == 0 ) {
        i2a[condIx] = COL_META_COLL_ATTR_NAME;
        cmdIx++;
        std::stringstream s1;
        s1 << "='" << cmdToken[cmdIx] << "'";
        vstr.push_back( s1.str() );
        condVal[condIx] = const_cast<char*>( vstr.back().c_str() );
        condIx++;

        i2a[condIx] = COL_META_COLL_ATTR_VALUE;
        std::stringstream s2;
        s2 << cmdToken[cmdIx + 1] << " '" << cmdToken[cmdIx + 2] << "'";
        vstr.push_back( s2.str() );
        cmdIx += 3;
        condVal[condIx] = const_cast<char*>( vstr.back().c_str() );
        condIx++;
        genQueryInp.sqlCondInp.len += 2;
    }

    if ( *cmdToken[cmdIx] != '\0' ) {
        printf( "Unrecognized input\n" );
        return -2;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    printGenQueryResults( Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printGenQueryResults( Conn, status, genQueryOut,
                              columnNames );
    }

    return 0;
}


/*
Do a query on AVUs for resources and show the results
 */
int queryResc( char *attribute, char *op, char *value ) {
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    int status;
    char *columnNames[] = {"resource"};

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    printCount = 0;
    i1a[0] = COL_R_RESC_NAME;
    i1b[0] = 0; /* (unused) */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_META_RESC_ATTR_NAME;
    std::string v1;
    v1 =  "='";
    v1 += attribute;
    v1 += "'";

    i2a[1] = COL_META_RESC_ATTR_VALUE;
    std::string v2;
    v2 =  op;
    v2 += " '";
    v2 += value;
    v2 += "'";

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    printGenQueryResults( Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printGenQueryResults( Conn, status, genQueryOut,
                              columnNames );
    }

    return 0;
}

/*
Do a query on AVUs for users and show the results
 */
int queryUser( char *attribute, char *op, char *value ) {
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];
    int status;
    char *columnNames[] = {"user", "zone"};

    printCount = 0;
    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    i1a[0] = COL_USER_NAME;
    i1b[0] = 0; /* (unused) */
    i1a[1] = COL_USER_ZONE;
    i1b[1] = 0; /* (unused) */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 2;

    i2a[0] = COL_META_USER_ATTR_NAME;
    std::string v1;
    v1 =  "='";
    v1 += attribute;
    v1 += "'";

    i2a[1] = COL_META_USER_ATTR_VALUE;
    std::string v2;
    v2 =  op;
    v2 += " '";
    v2 += value;
    v2 += "'";

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    printGenQueryResults( Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        printGenQueryResults( Conn, status, genQueryOut,
                              columnNames );
    }

    return 0;
}


/*
 Modify (copy) AVUs
 */
int
modCopyAVUMetadata( char *arg0, char *arg1, char *arg2, char *arg3,
                    char *arg4, char *arg5, char *arg6, char *arg7 ) {
    modAVUMetadataInp_t modAVUMetadataInp;
    int status;
    char fullName1[MAX_NAME_LEN];
    char fullName2[MAX_NAME_LEN];

    if ( strcmp( arg1, "-R" ) == 0 || strcmp( arg1, "-r" ) == 0 ||
            strcmp( arg1, "-u" ) == 0 ) {
        snprintf( fullName1, sizeof( fullName1 ), "%s", arg3 );
    }
    else if ( strlen( arg3 ) > 0 ) {
        if ( *arg3 == '/' ) {
            snprintf( fullName1, sizeof( fullName1 ), "%s", arg3 );
        }
        else {
            snprintf( fullName1, sizeof( fullName1 ), "%s/%s", cwd, arg3 );
        }
    }
    else {
        snprintf( fullName1, sizeof( fullName1 ), "%s", cwd );
    }

    if ( strcmp( arg2, "-R" ) == 0 || strcmp( arg2, "-r" ) == 0 ||
            strcmp( arg2, "-u" ) == 0 ) {
        snprintf( fullName2, sizeof( fullName2 ), "%s", arg4 );
    }
    else if ( strlen( arg4 ) > 0 ) {
        if ( *arg4 == '/' ) {
            snprintf( fullName2, sizeof( fullName2 ), "%s", arg4 );
        }
        else {
            snprintf( fullName2, sizeof( fullName2 ), "%s/%s", cwd, arg4 );
        }
    }
    else {
        snprintf( fullName2, sizeof( fullName2 ), "%s", cwd );
    }

    modAVUMetadataInp.arg0 = arg0;
    modAVUMetadataInp.arg1 = arg1;
    modAVUMetadataInp.arg2 = arg2;
    modAVUMetadataInp.arg3 = fullName1;
    modAVUMetadataInp.arg4 = fullName2;
    modAVUMetadataInp.arg5 = arg5;
    modAVUMetadataInp.arg6 = arg6;
    modAVUMetadataInp.arg7 = arg7;
    modAVUMetadataInp.arg8 = "";
    modAVUMetadataInp.arg9 = "";

    status = rcModAVUMetadata( Conn, &modAVUMetadataInp );
    lastCommandStatus = status;

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
        char *mySubName = NULL;
        const char *myName = rodsErrorName( status, &mySubName );
        rodsLog( LOG_ERROR, "rcModAVUMetadata failed with error %d %s %s",
                 status, myName, mySubName );
        free( mySubName );
    }

    if ( status == CAT_UNKNOWN_FILE ) {
        char tempName[MAX_NAME_LEN] = "/";
        int len;
        int isRemote = 0;
        strncat( tempName, myEnv.rodsZone, MAX_NAME_LEN - strlen( tempName ) );
        len = strlen( tempName );
        if ( strncmp( tempName, fullName1, len ) != 0 ) {
            printf( "Cannot copy metadata from a remote zone.\n" );
            isRemote = 1;
        }
        if ( strncmp( tempName, fullName2, len ) != 0 ) {
            printf( "Cannot copy metadata to a remote zone.\n" );
            isRemote = 1;
        }
        if ( isRemote ) {
            printf( "Copying of metadata is done via SQL within each ICAT\n" );
            printf( "for efficiency.  Copying metadata between zones is\n" );
            printf( "not implemented.\n" );
        }
    }
    return status;
}

/*
 Modify (add or remove) AVUs
 */
int
modAVUMetadata( char *arg0, char *arg1, char *arg2, char *arg3,
                char *arg4, char *arg5, char *arg6, char *arg7, char *arg8 ) {
    modAVUMetadataInp_t modAVUMetadataInp;
    int status;
    char fullName[MAX_NAME_LEN];

    if ( strcmp( arg1, "-R" ) == 0 || strcmp( arg1, "-r" ) == 0 ||
            strcmp( arg1, "-u" ) == 0 ) {
        snprintf( fullName, sizeof( fullName ), "%s", arg2 );
    }
    else if ( strlen( arg2 ) > 0 ) {
        if ( *arg2 == '/' ) {
            snprintf( fullName, sizeof( fullName ), "%s", arg2 );
        }
        else {
            snprintf( fullName, sizeof( fullName ), "%s/%s", cwd, arg2 );
        }
    }
    else {
        snprintf( fullName, sizeof( fullName ), "%s", cwd );
    }

    modAVUMetadataInp.arg0 = arg0;
    modAVUMetadataInp.arg1 = arg1;
    modAVUMetadataInp.arg2 = fullName;
    modAVUMetadataInp.arg3 = arg3;
    modAVUMetadataInp.arg4 = arg4;
    modAVUMetadataInp.arg5 = arg5;
    modAVUMetadataInp.arg6 = arg6;
    modAVUMetadataInp.arg7 = arg7;
    modAVUMetadataInp.arg8 = arg8;
    modAVUMetadataInp.arg9 = "";

    status = rcModAVUMetadata( Conn, &modAVUMetadataInp );
    lastCommandStatus = status;

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
        char *mySubName = NULL;
        const char *myName = rodsErrorName( status, &mySubName );
        rodsLog( LOG_ERROR, "rcModAVUMetadata failed with error %d %s %s",
                 status, myName, mySubName );
        free( mySubName );
    }
    
    return status;
}

/*
 Prompt for input and parse into tokens
*/
int
getInput( char *cmdToken[], int maxTokens ) {
    int lenstr, i;
    static char ttybuf[BIG_STR];
    int nTokens;
    int tokenFlag; /* 1: start reg, 2: start ", 3: start ' */
    char *cpTokenStart;
    char *stat;

    memset( ttybuf, 0, BIG_STR );
    fputs( "imeta>", stdout );
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
            return nTokens;
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
        if ( nTokens >= maxTokens ) {
            printf( "Limit reached (too many tokens, unrecognized input\n" );
            return -1;
        }
    }
    return nTokens;
}

int
parse_program_options (
    int                                     _argc,
    char**                                  _argv,
    rodsArguments_t&                        _rods_args,
    boost::program_options::parsed_options& _parsed,
    boost::program_options::variables_map&  _vm) {
    namespace po = boost::program_options;

    po::options_description global("Global options");
    global.add_options()
        ("help,h",                                             "Show imeta help")
        ("verbose,v",                                          "Verbose output")
        ("very_verbose,V",                                     "Very verbose output")
        ("zone_name,z", po::value< std::string >(),            "Work with the specified zone")
        ("command", po::value< std::string >(),                "Command to execute")
        ("subargs", po::value< std::vector< std::string > >(), "Arguments for command");

    po::positional_options_description pos;
    pos.add("command", 1).
        add("subargs", -1);

    try {
        _parsed = 
            po::command_line_parser( _argc, _argv ).
            options( global ).
            positional( pos ).
            allow_unregistered().
            run();

        po::store( _parsed, _vm );
        po::notify( _vm );

        if ( _vm.count( "verbose" ) ) {
            _rods_args.verbose = 1;
        }

        if ( _vm.count( "very_verbose" ) ) {
            _rods_args.verbose = 1;
            _rods_args.veryVerbose = 1;
            rodsLogLevel( LOG_NOTICE );
        }

        if ( _vm.count( "zone_name" ) ) {
            _rods_args.zone = 1;
            _rods_args.zoneName = (char*) _vm["zone_name"].as<std::string>().c_str();
        }

        if ( _vm.count( "help" ) ) {
            usage( "" );
            exit( 0 );
        }

        return 0;
    } catch ( po::error& _e ) {
        std::cout << std::endl
                  << "Error: "
                  << _e.what()
                  << std::endl
                  << std::endl;
        return SYS_INVALID_INPUT_PARAM;
    }
}

int do_interactive( boost::program_options::variables_map& _vm ) {
    char *cmdToken[40];
    int status;
    std::vector< std::string > command;

    while ( true ) {
        status = getInput( cmdToken, MAX_CMD_TOKENS );
        if ( status < 0 ) {
            return status;
        }

        command.clear();
        // getInput now returns the number of tokens on success
        for ( int i = 0; i < status; ++i ) {
            command.push_back(cmdToken[i]);
        }

        status = do_command( command, _vm );
        if ( status < 0 ) {
            break;
        }
    }

    return 0;
}

int do_command( 
    std::vector< std::string >              _input,
    boost::program_options::variables_map&  _vm ) {
    namespace po = boost::program_options;

    if ( _input.size() >= MAX_CMD_TOKENS ) {
        std::cout << "Unrecognized input, too many input tokens\n";
        exit( 4 );
    }

    int status;

    std::string cmd = _input.at(0);

    if ( cmd == "add" ||
         cmd == "adda" ||
         cmd == "addw" ) {
        // Add new AVU triple
        std::string units = "";

        po::options_description add_desc( "add options" );
        add_desc.add_options()
            ("object_type", po::value<std::string>(), "Object type (-d/-C/-R/-u)")
            ("name", po::value<std::string>(),        "Object name")
            ("attribute", po::value<std::string>(),   "AVU attribute")
            ("value", po::value<std::string>(),       "AVU value")
            ("units", po::value<std::string>(),       "AVU units [optional]");

        po::positional_options_description add_pos;
        add_pos.add("object_type", 1).
            add("name", 1).
            add("attribute", 1).
            add("value", 1).
            add("units", 1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( add_desc ).
                    positional( add_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type" ) ||
            ( _vm["object_type"].as<std::string>() != "-d" &&
             _vm["object_type"].as<std::string>() != "-C" &&
             _vm["object_type"].as<std::string>() != "-R" &&
             _vm["object_type"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }
        
        if ( !_vm.count( "value" ) ) {
            std::cout << std::endl
                      << "Error: Not enough arguments provided to "
                      << cmd
                      << std::endl
                      << std::endl;
        }

        if ( _vm.count( "units" ) ) {
            units = _vm["units"].as<std::string>();
        }

        status = modAVUMetadata( (char*) cmd.c_str(),
                        (char*) _vm["object_type"].as<std::string>().c_str(),
                        (char*) _vm["name"].as<std::string>().c_str(),
                        (char*) _vm["attribute"].as<std::string>().c_str(),
                        (char*) _vm["value"].as<std::string>().c_str(),
                        (char*) units.c_str(),
                        "",
                        "",
                        "" );

        if ( cmd == "addw" ) {
            std::cout << "AVU added to " << status << " data-objects\n";
        }

        return 0;
    } else if ( cmd == "rm" ||
                cmd == "rmw" ) {
        // Remove AVU triple
        std::string units = "";

        po::options_description rm_desc( "rm options" );
        rm_desc.add_options()
            ("object_type", po::value<std::string>(), "Object type (-d/-C/-R/-u)")
            ("name", po::value<std::string>(),      "Object name")
            ("attribute", po::value<std::string>(), "AVU attribute")
            ("value", po::value<std::string>(),     "AVU value")
            ("units", po::value<std::string>(),     "AVU units [optional]");

        po::positional_options_description rm_pos;
        rm_pos.add("object_type", 1).
            add("name", 1).
            add("attribute", 1).
            add("value", 1).
            add("units", 1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( rm_desc ).
                    positional( rm_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type" ) ||
            ( _vm["object_type"].as<std::string>() != "-d" &&
             _vm["object_type"].as<std::string>() != "-C" &&
             _vm["object_type"].as<std::string>() != "-R" &&
             _vm["object_type"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "value" ) ) {
            std::cout << std::endl
                      << "Error: Not enough arguments provided to "
                      << cmd
                      << std::endl
                      << std::endl;
        }

        if ( _vm.count( "units" ) ) {
            units = _vm["units"].as<std::string>();
        }

        status = modAVUMetadata( (char*) cmd.c_str(),
                        (char*) _vm["object_type"].as<std::string>().c_str(),
                        (char*) _vm["name"].as<std::string>().c_str(),
                        (char*) _vm["attribute"].as<std::string>().c_str(),
                        (char*) _vm["value"].as<std::string>().c_str(),
                        (char*) units.c_str(),
                        "",
                        "",
                        "" );

        return 0;
    } else if ( cmd == "rmi" ) {
        // Remove AVU triple by metadata ID
        po::options_description rmi_desc( "rm options" );
        rmi_desc.add_options()
            ("object_type", po::value<std::string>(), "Object type (-d/-C/-R/-u)")
            ("name", po::value<std::string>(),        "Object name")
            ("metadata_id", po::value<std::string>(), "Metadata ID");

        po::positional_options_description rmi_pos;
        rmi_pos.add("object_type", 1).
            add("name", 1).
            add("metadata_id", 1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( rmi_desc ).
                    positional( rmi_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type" ) ||
            ( _vm["object_type"].as<std::string>() != "-d" &&
             _vm["object_type"].as<std::string>() != "-C" &&
             _vm["object_type"].as<std::string>() != "-R" &&
             _vm["object_type"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "metadata_id" ) ) {
            std::cout << std::endl
                      << "Error: Not enough arguments provided to "
                      << cmd
                      << std::endl
                      << std::endl;
        }

        status = modAVUMetadata( (char*) cmd.c_str(),
                        (char*) _vm["object_type"].as<std::string>().c_str(),
                        (char*) _vm["name"].as<std::string>().c_str(),
                        (char*) _vm["metadata_id"].as<std::string>().c_str(),
                        "",
                        "",
                        "",
                        "",
                        "" );

        return 0;
    } else if ( cmd == "mod" ) {
        // Modify existing AVU triple
        std::string units = "";
        std::string new_attribute = "";
        std::string new_value = "";
        std::string new_units = "";

        po::options_description mod_desc( "mod options" );
        mod_desc.add_options()
            ("object_type", po::value<std::string>(), "Object type (-d/-C/-R/-u)")
            ("name", po::value<std::string>(),      "Object name")
            ("attribute", po::value<std::string>(), "AVU attribute")
            ("value", po::value<std::string>(),     "AVU value")
            ("opt_1", po::value<std::string>(),     "AVU units [optional]")
            ("opt_2", po::value<std::string>(),     "New AVU attribute [optional]")
            ("opt_3", po::value<std::string>(),     "New AVU value [optional]")
            ("opt_4", po::value<std::string>(),     "New AVU units [optional]");

        po::positional_options_description mod_pos;
        mod_pos.add("object_type", 1).
            add("name", 1).
            add("attribute", 1).
            add("value", 1).
            add("opt_1", 1).
            add("opt_2", 1).
            add("opt_3", 1).
            add("opt_4", 1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( mod_desc ).
                    positional( mod_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type" ) ||
            ( _vm["object_type"].as<std::string>() != "-d" &&
             _vm["object_type"].as<std::string>() != "-C" &&
             _vm["object_type"].as<std::string>() != "-R" &&
             _vm["object_type"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "opt_1" ) ) {
            std::cout << std::endl
                      << "Error: Not enough arguments provided to "
                      << cmd
                      << std::endl
                      << std::endl;
        } else {
            // Need to check optional arguments to see if the string represents
            //  units, new_attribute, new_value, or new_units
            std::string temp = _vm["opt_1"].as<std::string>();

            if ( temp.substr(2) == "n:" ) {
                new_attribute = temp;
            } else if (temp.substr(2) == "v:" ) {
                new_value = temp;
            } else if (temp.substr(2) == "u:" ) {
                new_value = temp;
            } else {
                units = temp;
            }
        }

        if ( _vm.count( "opt_2" ) ) {
            std::string temp = _vm["opt_2"].as<std::string>();

            if ( temp.substr(2) == "n:" ) {
                new_attribute = temp;
            } else if (temp.substr(2) == "v:" ) {
                new_value = temp;
            } else if (temp.substr(2) == "u:" ) {
                new_value = temp;
            } else {
                units = temp;
            }
        }
        
        if ( _vm.count( "opt_3" ) ) {
            std::string temp = _vm["opt_3"].as<std::string>();

            if ( temp.substr(2) == "n:" ) {
                new_attribute = temp;
            } else if (temp.substr(2) == "v:" ) {
                new_value = temp;
            } else if (temp.substr(2) == "u:" ) {
                new_value = temp;
            } else {
                units = temp;
            }
        }

        if ( _vm.count( "opt_4" ) ) {
            std::string temp = _vm["opt_4"].as<std::string>();

            if ( temp.substr(2) == "n:" ) {
                new_attribute = temp;
            } else if (temp.substr(2) == "v:" ) {
                new_value = temp;
            } else if (temp.substr(2) == "u:" ) {
                new_value = temp;
            } else {
                units = temp;
            }
        }

        status = modAVUMetadata( (char*) cmd.c_str(),
                        (char*) _vm["object_type"].as<std::string>().c_str(),
                        (char*) _vm["name"].as<std::string>().c_str(),
                        (char*) _vm["attribute"].as<std::string>().c_str(),
                        (char*) _vm["value"].as<std::string>().c_str(),
                        (char*) units.c_str(),
                        (char*) new_attribute.c_str(),
                        (char*) new_value.c_str(),
                        (char*) new_units.c_str() );

        return 0;
    } else if ( cmd == "set" ) {
        // Assign a new value to an existing AVU triple
        std::string new_units = "";

        po::options_description set_desc( "set options" );
        set_desc.add_options()
            ("object_type", po::value<std::string>(), "Object type (-d/-C/-R/-u)")
            ("name", po::value<std::string>(),      "Object name")
            ("attribute", po::value<std::string>(), "Attribute name")
            ("new_value", po::value<std::string>(), "New attribute value")
            ("new_units", po::value<std::string>(), "New attribute units [optional]");

        po::positional_options_description set_pos;
        set_pos.add("object_type", 1).
            add("name", 1).
            add("attribute", 1).
            add("new_value", 1).
            add("new_units", 1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( set_desc ).
                    positional( set_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type" ) ||
            ( _vm["object_type"].as<std::string>() != "-d" &&
             _vm["object_type"].as<std::string>() != "-C" &&
             _vm["object_type"].as<std::string>() != "-R" &&
             _vm["object_type"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "new_value" ) ) {
            std::cout << std::endl
                      << "Error: Not enough arguments provided to "
                      << cmd
                      << std::endl
                      << std::endl;
        }

        if ( _vm.count( "new_units" ) ) {
            new_units = _vm["new_units"].as<std::string>();
        }

        status = modAVUMetadata( (char*) cmd.c_str(),
                        (char*) _vm["object_type"].as<std::string>().c_str(),
                        (char*) _vm["name"].as<std::string>().c_str(),
                        (char*) _vm["attribute"].as<std::string>().c_str(),
                        (char*) _vm["new_value"].as<std::string>().c_str(),
                        (char*) new_units.c_str(),
                        "",
                        "",
                        "" );

        return 0;
    } else if ( cmd == "ls" ||
                cmd == "lsw" ) {
        // List existing AVUs on an iRODS object
        std::string attribute = "";
        int wild = ( cmd == "lsw" ) ? 1 : 0;

        po::options_description ls_desc( "ls options" );
        ls_desc.add_options()
            ("object_type", po::value<std::string>(), "Object type (-d/-C/-R/-u)")
            ("name", po::value<std::string>(),      "Object name")
            ("attribute", po::value<std::string>(), "Attribute name");

        po::positional_options_description ls_pos;
        ls_pos.add("object_type", 1).
            add("name", 1).
            add("attribute", 1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( ls_desc ).
                    positional( ls_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type" ) ||
            ( _vm["object_type"].as<std::string>() != "-ld" &&
             _vm["object_type"].as<std::string>() != "-d" &&
             _vm["object_type"].as<std::string>() != "-C" &&
             _vm["object_type"].as<std::string>() != "-R" &&
             _vm["object_type"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "name" ) ) {
            std::cout << std::endl
                      << "Error: Not enough arguments provided to "
                      << cmd
                      << std::endl
                      << std::endl;
        }

        if ( _vm.count( "attribute" ) ) {
            attribute = _vm["attribute"].as<std::string>();
        }

        std::string obj_type = _vm["object_type"].as<std::string>();

        if ( obj_type == "-ld") {
            longMode = 1;

            showDataObj( (char*) _vm["name"].as<std::string>().c_str(),
                         (char*) attribute.c_str(),
                         wild );
        } else if ( obj_type == "-d" ) {
            showDataObj( (char*) _vm["name"].as<std::string>().c_str(),
                         (char*) attribute.c_str(),
                         wild );
        } else if ( obj_type == "-C" ) {
            showColl( (char*) _vm["name"].as<std::string>().c_str(),
                      (char*) attribute.c_str(),
                      wild );
        } else if ( obj_type == "-R" ) {
            showResc( (char*) _vm["name"].as<std::string>().c_str(),
                      (char*) attribute.c_str(),
                      wild );
        } else {
            showUser( (char*) _vm["name"].as<std::string>().c_str(),
                      (char*) attribute.c_str(),
                      wild );
        }

        return 0;
    } else if ( cmd == "qu" ) {
        // Query objects with matching AVUs
        po::options_description qu_desc( "qu options" );
        qu_desc.add_options()
            ("object_type", po::value<std::string>(), "Object type (-d/-C/-R/-u)")
            ("attribute", po::value<std::string>(),              "Attribute name")
            ("operator", po::value<std::string>(),               "Operator")
            ("value", po::value<std::string>(),                  "Value to compare against")
            ("more_args", po::value<std::vector<std::string>>(), "Additional arguments");

        po::positional_options_description qu_pos;
        qu_pos.add("object_type", 1).
            add("attribute", 1).
            add("operator", 1).
            add("value", 1).
            add("more_args", -1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( qu_desc ).
                    positional( qu_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type" ) ||
            ( _vm["object_type"].as<std::string>() != "-d" &&
             _vm["object_type"].as<std::string>() != "-C" &&
             _vm["object_type"].as<std::string>() != "-R" &&
             _vm["object_type"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "value" ) ) {
            std::cout << std::endl
                      << "Error: Not enough arguments provided to "
                      << cmd
                      << std::endl
                      << std::endl;
        }

        std::string obj_type = _vm["object_type"].as<std::string>();

        if ( obj_type == "-d" ) {
            char *tempCmdToken[40];
            int i = 0;
            for ( int j = 0; j < MAX_CMD_TOKENS; ++j ) {
                tempCmdToken[j] = "";
            }

            /* Need to put first element (command) back on to satisfy queryDataObj */
            _input.insert( _input.begin(), "qu" );

            for ( const auto& s : _input ) {
                tempCmdToken[i++] = (char*) s.c_str();
            }

            status = queryDataObj( tempCmdToken );
        } else if ( obj_type == "-C" ) {
            char *tempCmdToken[40];
            int i = 0;
            for ( int j = 0; j < MAX_CMD_TOKENS; ++j ) {
                tempCmdToken[j] = "";
            }

            /* Need to put first element (command) back on to satisfy queryCollection */
            _input.insert( _input.begin(), "qu" );

            for ( const auto& s : _input ) {
                tempCmdToken[i++] = (char*) s.c_str();
            }

            status = queryCollection( tempCmdToken );
        } else if ( obj_type == "-R" ) {
            status = queryResc( (char*) _vm["attribute"].as<std::string>().c_str(),
                                (char*) _vm["operator"].as<std::string>().c_str(),
                                (char*) _vm["value"].as<std::string>().c_str() );
        } else {
            status = queryUser( (char*) _vm["attribute"].as<std::string>().c_str(),
                                (char*) _vm["operator"].as<std::string>().c_str(),
                                (char*) _vm["value"].as<std::string>().c_str() );
        }

        return 0;
    } else if ( cmd == "cp" ) {
        // Copy AVUs from one iRODS object to another
        po::options_description cp_desc( "cp options" );
        cp_desc.add_options()
            ("object_type_1", po::value<std::string>(), "First object type (-d/-C/-R/-u)")
            ("object_type_2", po::value<std::string>(), "Second object type (-d/-C/-R/-u")
            ("name_1", po::value<std::string>(),        "First object name")
            ("name_2", po::value<std::string>(),        "Second object name");

        po::positional_options_description cp_pos;
        cp_pos.add("object_type_1", 1).
            add("object_type_2", 1).
            add("name_1", 1).
            add("name_2", 1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( cp_desc ).
                    positional( cp_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type_1" ) ||
            ( _vm["object_type_1"].as<std::string>() != "-d" &&
             _vm["object_type_1"].as<std::string>() != "-C" &&
             _vm["object_type_1"].as<std::string>() != "-R" &&
             _vm["object_type_1"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No first object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "object_type_2" ) ||
            ( _vm["object_type_2"].as<std::string>() != "-d" &&
             _vm["object_type_2"].as<std::string>() != "-C" &&
             _vm["object_type_2"].as<std::string>() != "-R" &&
             _vm["object_type_2"].as<std::string>() != "-u" ) ) {
            std::cout << std::endl
                      << "Error: "
                      << "No second object type descriptor (-d/C/R/u) specified"
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( !_vm.count( "name_2" ) ) {
            std::cout << std::endl
                      << "Error: Not enough arguments provided to "
                      << cmd
                      << std::endl
                      << std::endl;
        }

        status = modCopyAVUMetadata( (char*) cmd.c_str(),
                        (char*) _vm["object_type_1"].as<std::string>().c_str(),
                        (char*) _vm["object_type_2"].as<std::string>().c_str(),
                        (char*) _vm["name_1"].as<std::string>().c_str(),
                        (char*) _vm["name_2"].as<std::string>().c_str(),
                        "",
                        "",
                        "" );

        return 0;
    } else if ( cmd == "upper" ) {
        // Toggle between upper case mode for queries
        if ( upperCaseFlag ) {
            upperCaseFlag = 0;
            std::cout << "upper case mode disabled\n";
        } else {
            upperCaseFlag = 1;
            std::cout << "upper case mode for 'qu' command enabled\n";
        }

        return 0;
    } else if ( cmd == "help" ||
                cmd == "h" ) {
        // Display help for a specified command
        std::string help_command = "";
        po::options_description help_desc( "help options" );
        help_desc.add_options()
            ("help_command", po::value<std::string>(), "Which command is help for");

        po::positional_options_description help_pos;
        help_pos.add("help_command", 1);

        try {
            /*
            The unrecognized options from parse_program_options contain the
            first positional argument, the command name. Need to delete it
            before we continue parsing
            */
            _input.erase(_input.begin());

            po::store(
                po::command_line_parser(_input).
                    options( help_desc ).
                    positional( help_pos ).
                    style(po::command_line_style::unix_style ^ 
                        po::command_line_style::allow_short).
                    allow_unregistered().
                    run(), _vm );
            po::notify( _vm );
        } catch ( po::error& _e ) {
            std::cout << std::endl
                      << "Error: "
                      << _e.what()
                      << std::endl
                      << std::endl;
            return SYS_INVALID_INPUT_PARAM;
        }

        if ( _vm.count( "help_command" ) ) {
            help_command = _vm["help_command"].as<std::string>();
        }

        usage( (char*) help_command.c_str() );
        exit( 0 );
    } else if ( cmd == "quit" ||
                cmd == "q" ) {
        return -1;
    } else if ( cmd.empty() ) {
        return -3;
    } else {
        std::cout << "unrecognized subcommand '"
                  << cmd
                  << "', try 'imeta help'"
                  << std::endl;
        return -2;
    }

    return 0;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rErrMsg_t errMsg;

    rodsArguments_t myRodsArgs;

    rodsLogLevel( LOG_ERROR );


    boost::program_options::variables_map _vm;
    boost::program_options::parsed_options _parsed(NULL, 0);
    status = parse_program_options( argc, argv, myRodsArgs, _parsed, _vm );

    if ( status ) {
        std::cout << "Use -h for help\n";
        exit( 1 );
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

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        char *mySubName = NULL;
        const char *myName = rodsErrorName( errMsg.status, &mySubName );
        rodsLog( LOG_ERROR, "rcConnect failure %s (%s) (%d) %s",
                 myName,
                 mySubName,
                 errMsg.status,
                 errMsg.msg );
        free( mySubName );

        exit( 2 );
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        exit( 3 );
    }

    if ( !_vm.count( "command" ) ) {
        // No command entered; run in interactive mode
        do_interactive( _vm );
    } else {
        std::vector< std::string > command_to_be_parsed = 
            boost::program_options::collect_unrecognized( _parsed.options, 
                                                          boost::program_options::include_positional );

        do_command( command_to_be_parsed, _vm );
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
        "Usage: imeta [-vVhz] [command]",
        " -v verbose",
        " -V Very verbose",
        " -z Zonename  work with the specified Zone",
        " -h This help",
        "Commands are:",
        " add -d|C|R|u Name AttName AttValue [AttUnits] (Add new AVU triple)",
        " adda -d|C|R|u Name AttName AttValue [AttUnits] (Add as administrator)",
        "                                     (same as 'add' but bypasses ACLs)",
        " addw -d Name AttName AttValue [AttUnits] (Add new AVU triple",
        "                                           using Wildcards in Name)",
        " rm  -d|C|R|u Name AttName AttValue [AttUnits] (Remove AVU)",
        " rmw -d|C|R|u Name AttName AttValue [AttUnits] (Remove AVU, use Wildcards)",
        " rmi -d|C|R|u Name MetadataID (Remove AVU by MetadataID)",
        " mod -d|C|R|u Name AttName AttValue [AttUnits] [n:Name] [v:Value] [u:Units]",
        "      (modify AVU; new name (n:), value(v:), and/or units(u:)",
        " set -d|C|R|u Name AttName newValue [newUnits] (Assign a single value)",
        " ls  -[l]d|C|R|u Name [AttName] (List existing AVUs for item Name)",
        " lsw -[l]d|C|R|u Name [AttName] (List existing AVUs, use Wildcards)",
        " qu -d|C|R|u AttName Op AttVal [...] (Query objects with matching AVUs)",
        " cp -d|C|R|u -d|C|R|u Name1 Name2 (Copy AVUs from item Name1 to Name2)",
        " upper (Toggle between upper case mode for queries (qu))",
        " ",
        "Metadata attribute-value-units triples (AVUs) consist of an Attribute-Name,",
        "Attribute-Value, and an optional Attribute-Units.  They can be added",
        "via the 'add' command (and in other ways), and",
        "then queried to find matching objects.",
        " ",
        "For each command, -d, -C, -R, or -u is used to specify which type of",
        "object to work with: dataobjs (iRODS files), collections, resources,",
        "or users.",
        " ",
        "Fields represented with upper case, such as Name, are entered values.  For",
        "example, 'Name' is the name of a dataobject, collection, resource,",
        "or user.",
        " ",
        "For rmw and lsw, the % and _ wildcard characters (as defined for SQL) can",
        "be used for matching attribute values.",
        " ",
        "For addw, the % and _ wildcard characters (as defined for SQL) can",
        "be used for matching object names.  This is currently implemented only",
        "for data-objects (-d).",
        " ",
        "A blank execute line invokes the interactive mode, where imeta",
        "prompts and executes commands until 'quit' or 'q' is entered.",
        "Like other unix utilities, a series of commands can be piped into it:",
        "'cat file1 | imeta' (maintaining one connection for all commands).",
        " ",
        "Single or double quotes can be used to enter items with blanks.",
        " ",
        "Entered usernames are of the form username[#zone].  If #zone is not",
        "provided, the zone from your irods_environment.json is assumed.",
        " ",
        "The appropriate zone (local or remote) is determined from the path names",
        "or via -z Zonename (for 'qu' and when working with resources).",
        " ",
        "Try 'help command' for more help on a specific command.",
        "'help qu' will explain additional options on the query.",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "imeta" );
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
        if ( strcmp( subOpt, "add" ) == 0 ) {
            char *msgs[] = {
                " add -d|C|R|u Name AttName AttValue [AttUnits]  (Add new AVU triple)",
                "Add an AVU to a dataobj (-d), collection(-C), resource(-R), ",
                "or user(-u)",
                "Example: add -d file1 distance 12 miles",
                " ",
                "Admins can also use the command 'adda' (add as admin) to add metadata",
                "to any collection or dataobj; syntax is the same as 'add'.  Admins are",
                "also allowed to add user and resource metadata.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "adda" ) == 0 ) {
            char *msgs[] = {
                " adda -d|C|R|u Name AttName AttValue [AttUnits]  (Add as administrator)",
                "                                     (same as 'add' but bypasses ACLs)",
                " ",
                "Administrators (rodsadmin users) may use this command to add AVUs",
                "to any dataobj, collection, resource, or user.  The syntax is the same",
                "as 'imeta add'.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "addw" ) == 0 ) {
            char *msgs[] = {
                " addw -d Name AttName AttValue [AttUnits]  (Add new AVU triple)",
                "Add an AVU to a set of data-objects using wildcards to match",
                "the data-object names.",
                " ",
                "The character _ matches any single character and % matches any",
                "number of any characters.",
                " ",
                "Example: addw -d file% distance 12 miles",
                "would add the AVU to dataobjects in the current directory with names",
                "that start with 'file'.",
                " ",
                "Example2: addw -d /tempZone/home/rods/test/%/% distance 12 miles",
                "would add the AVU to all dataobjects in the 'test' collection or any",
                "subcollections under 'test'.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "rm" ) == 0 ) {
            char *msgs[] = {
                " rm  -d|C|R|u Name AttName AttValue [AttUnits] (Remove AVU)",
                "Remove an AVU from a dataobj (-d), collection(-C), resource(-R),",
                "or user(-u)",
                "Example: rm -d file1 distance 12 miles",
                "An AttUnits value must be included if it was when the AVU was added.",
                "Also see rmw for use of wildcard characters.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "rmw" ) == 0 ) {
            char *msgs[] = {
                " rmw  -d|C|R|u Name AttName AttValue [AttUnits] (Remove AVU, use Wildcard)",
                "Remove an AVU from a dataobj (-d), collection(-C), resource(-R), ",
                "or user(-u)",
                "An AttUnits value must be included if it was when the AVU was added.",
                "rmw is very similar to rm but using SQL wildcard characters, _ and %.",
                "The _ matches any single character and % matches any number of any",
                "characters.  Examples:",
                "  rmw -d file1 distance % %",
                " or ",
                "  rmw -d file1 distance % m% ",
                " ",
                "Note that if the attributes contain the characters '%' or '_', ",
                "the rmw command still do matching using them as wildcards, so you may",
                "need to use rm instead.",
                "Also see lsw.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "rmi" ) == 0 ) {
            char *msgs[] = {
                " rmi -d|C|R|u Name MetadataID (Remove AVU by MetadataID)",
                "Remove an AVU from a dataobj (-d), collection(-C), resource(-R),",
                "or user(-u) by name and metadataID from the catalog.  Intended for",
                "expert use.  There is no matching done to confirm correctness of",
                "a request (but permissions are still checked).  Use with caution.",
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
                " mod -d|C|R|u Name AttName AttValue [AttUnits] [n:Name] [v:Value] [u:Units]",
                "      (modify AVU; new name (n:), value(v:), and/or units(u:)",
                "Modify a defined AVU for the specified item (object)",
                "Example: mod -d file1 distance 14 miles v:27",
                " ",
                "Only the AVU associated with this object is modified.  Internally, the system",
                "removes the old AVU from the object and creates a new one (ensuring",
                "consistency), and performs a single 'commit' if all is valid.",
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
                " ls -d|C|R|u Name [AttName] (List existing AVUs for item Name)",
                " ls -ld Name [AttName]        (List in long format)",
                "List defined AVUs for the specified item",
                "Example: ls -d file1",
                "If the optional AttName is included, it is the attribute name",
                "you wish to list and only those will be listed.",
                "If the optional -l is used on dataObjects (-ld), the long format will",
                "be displayed which includes the time the AVU was set.",
                "Also see lsw.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }

        if ( strcmp( subOpt, "lsw" ) == 0 ) {
            char *msgs[] = {
                " lsw -d|C|R|u Name [AttName] (List existing AVUs, use Wildcards)",
                "List defined AVUs for the specified item",
                "Example: lsw -d file1",
                "If the optional AttName is included, it is the attribute name",
                "you wish to list, doing so using wildcard matching.",
                "For example: ls -d file1 attr%",
                "If the optional -l is used on dataObjects (-ld), the long format will",
                "be displayed which includes the time the AVU was set.",
                "Also see rmw and ls.",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "set" ) == 0 ) {
            char *msgs[] = {
                " set -d|C|R|u Name AttName newValue [newUnits]  (assign a single value)",
                "Set the newValue (and newUnit) of an AVU of a dataobj (-d), collection(-C),",
                "     resource(-R) or user(-u).",
                " ",
                "'set' modifies an AVU if it exists, or creates one if it does not.",
                "If the AttName does not exist, or is used by multiple objects, the AVU for",
                "this object is added.",
                "If the AttName is used only by this one object, the AVU (row) is modified",
                "with the new values, reducing the database overhead (unused rows).",
                "Example: set -d file1 distance 12",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "qu" ) == 0 ) {
            char *msgs[] = {
                " qu -d|C|R|u AttName Op AttVal [...] (Query objects with matching AVUs)",
                "Query across AVUs for the specified type of item",
                "Example: qu -d distance '<=' 12",
                " ",
                "When querying dataObjects (-d) or collections (-C) additional conditions",
                "(AttName Op AttVal) may be given separated by 'and', for example:",
                " qu -d a = b and c '<' 10",
                "Or a single 'or' can be given for the same AttName, for example",
                " qu -d r '<' 5 or '>' 7",
                " ",
                "You can also query in numeric mode (instead of as strings) by adding 'n'",
                "in front of the test condition, for example:",
                " qu -d r 'n<' 123",
                "which causes it to cast the AVU column to numeric (decimal) in the SQL.",
                "In numeric mode, if any of the named AVU values are non-numeric, a SQL",
                "error will occur but this avoids problems when comparing numeric strings",
                "of different lengths.",
                " ",
                "Other examples:",
                " qu -d a like b%",
                "returns data-objects with attribute 'a' with a value that starts with 'b'.",
                " qu -d a like %",
                "returns data-objects with attribute 'a' defined (with any value).",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "cp" ) == 0 ) {
            char *msgs[] = {
                " cp -d|C|R|u -d|C|R|u Name1 Name2 (Copy AVUs from item Name1 to Name2)",
                "Example: cp -d -C file1 dir1",
                ""
            };
            for ( i = 0;; i++ ) {
                if ( strlen( msgs[i] ) == 0 ) {
                    return 0;
                }
                printf( "%s\n", msgs[i] );
            }
        }
        if ( strcmp( subOpt, "upper" ) == 0 ) {
            char *msgs[] = {
                " upper (Toggle between upper case mode for queries (qu)",
                "When enabled, the 'qu' queries will use the upper-case mode for the 'where'",
                "clause, so you can do a case-insensitive query (using an upper case literal)",
                "to compare against.  For example:",
                "  upper",
                "  qu -d A like B%",
                "will return all dataobjects with an AVU named 'A' or 'a' with a value that",
                "begins with 'b' or 'B'.",
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
