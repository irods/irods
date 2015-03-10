/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
   Initial version of an administrator interface
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

#include <iostream>
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#include <string>

#define MAX_SQL 300
#define BIG_STR 3000

/* The simpleQuery input sql is passed as an argument (along with up
   to 4 bind variables) so that is is clear what is going on.  But the
   server-side code checks the input sql against some pre-defined
   forms (to improve security a bit).
*/

int debug = 0;
int veryVerbose = 0;

char localZone[BIG_STR] = "";

rcComm_t *Conn;
rodsEnv myEnv;

int lastCommandStatus = 0;

void usage( char *subOpt );

/* print the results of a simple query, converting time values if
   necessary.  Called recursively.
*/
int
printSimpleQuery( char *buf ) {

    std::vector< std::string > tokens;
    irods::string_tokenize( buf, "\n", tokens );
    std::vector< std::string >::iterator itr = tokens.begin();
    for ( ; itr != tokens.end(); ++itr ) {
        // =-=-=-=-=-=-=-
        // explicitly filter out the resource class
        if ( std::string::npos == itr->find( "resc_class" ) ) {
            // =-=-=-=-=-=-=-
            // determine if the token is of a time that needs
            // converted from unix time to a human readable form
            if ( std::string::npos != itr->find( "_ts" ) ) {
                // =-=-=-=-=-=-=-
                // tokenize based on the ':' delimiter this time
                // to convert the time
                std::vector< std::string > time_tokens;
                irods::string_tokenize( *itr, ":", time_tokens );
                if ( time_tokens.size() != 2 ) {
                    std::cout << "printSimpleQuery - incorrect number of tokens "
                              << "for case of time conversion" << std::endl;
                    return -1;
                }
                else {
                    char local_time[TIME_LEN];
                    getLocalTimeFromRodsTime( time_tokens[1].c_str(), local_time );
                    std::cout << time_tokens[0] << " " << local_time << std::endl;
                }

            }
            else {
                // =-=-=-=-=-=-=-
                // simply print out the token
                std::cout << *itr << std::endl;
            }

        } // if not resc_class

    } // for itr

    return 0;
}

int
doSimpleQuery( simpleQueryInp_t simpleQueryInp ) {
    int status;
    simpleQueryOut_t *simpleQueryOut;
    char *mySubName;
    char *myName;
    status = rcSimpleQuery( Conn, &simpleQueryInp, &simpleQueryOut );
    lastCommandStatus = status;

    if ( status ==  CAT_NO_ROWS_FOUND ) {
        lastCommandStatus = 0; /* success */
        printf( "No rows found\n" );
        return status;
    }
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
        rodsLog( LOG_ERROR, "rcSimpleQuery failed with error %d %s %s",
                 status, myName, mySubName );
        return status;
    }

    printSimpleQuery( simpleQueryOut->outBuf );
    if ( debug ) {
        printf( "control=%d\n", simpleQueryOut->control );
    }
    if ( simpleQueryOut->control > 0 ) {
        simpleQueryInp.control = simpleQueryOut->control;
        for ( ; simpleQueryOut->control > 0 && status == 0; ) {
            status = rcSimpleQuery( Conn, &simpleQueryInp, &simpleQueryOut );
            if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                myName = rodsErrorName( status, &mySubName );
                rodsLog( LOG_ERROR,
                         "rcSimpleQuery failed with error %d %s %s",
                         status, myName, mySubName );
                return status;
            }
            if ( status == 0 ) {
                printSimpleQuery( simpleQueryOut->outBuf );
                if ( debug ) {
                    printf( "control=%d\n", simpleQueryOut->control );
                }
            }
        }
    }
    return status;
}

int
showToken( char *token, char *tokenName2 ) {
    simpleQueryInp_t simpleQueryInp;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    if ( token == 0 || *token == '\0' ) {
        simpleQueryInp.form = 1;
        simpleQueryInp.sql =
            "select token_name from R_TOKN_MAIN where token_namespace = 'token_namespace'";
        simpleQueryInp.maxBufSize = 1024;
    }
    else {
        if ( tokenName2 == 0 || *tokenName2 == '\0' ) {
            simpleQueryInp.form = 1;
            simpleQueryInp.sql = "select token_name from R_TOKN_MAIN where token_namespace = ?";
            simpleQueryInp.arg1 = token;
            simpleQueryInp.maxBufSize = 1024;
        }
        else {
            simpleQueryInp.form = 2;
            simpleQueryInp.sql = "select * from R_TOKN_MAIN where token_namespace = ? and token_name like ?";
            simpleQueryInp.arg1 = token;
            simpleQueryInp.arg2 = tokenName2;
            simpleQueryInp.maxBufSize = 1024;
        }
    }
    return doSimpleQuery( simpleQueryInp );
}

int
showResc( char *resc ) {
    simpleQueryInp_t simpleQueryInp;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    if ( resc == 0 || *resc == '\0' ) {
        simpleQueryInp.form = 1;
        simpleQueryInp.sql =
            "select resc_name from R_RESC_MAIN";
        simpleQueryInp.maxBufSize = 1024;
    }
    else {
        simpleQueryInp.form = 2;
        simpleQueryInp.sql = "select * from R_RESC_MAIN where resc_name=?";
        simpleQueryInp.arg1 = resc;
        simpleQueryInp.maxBufSize = 1024;
    }
    return doSimpleQuery( simpleQueryInp );
}

int
showZone( char *zone ) {
    simpleQueryInp_t simpleQueryInp;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    if ( zone == 0 || *zone == '\0' ) {
        simpleQueryInp.form = 1;
        simpleQueryInp.sql =
            "select zone_name from R_ZONE_MAIN";
        simpleQueryInp.maxBufSize = 1024;
    }
    else {
        simpleQueryInp.form = 2;
        simpleQueryInp.sql = "select * from R_ZONE_MAIN where zone_name=?";
        simpleQueryInp.arg1 = zone;
        simpleQueryInp.maxBufSize = 1024;
    }
    return doSimpleQuery( simpleQueryInp );
}

int
getLocalZone() {
    int status, i;
    simpleQueryInp_t simpleQueryInp;
    simpleQueryOut_t *simpleQueryOut;
    if ( localZone[0] == '\0' ) {
        memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
        simpleQueryInp.form = 1;
        simpleQueryInp.sql =
            "select zone_name from R_ZONE_MAIN where zone_type_name=?";
        simpleQueryInp.arg1 = "local";
        simpleQueryInp.maxBufSize = 1024;
        status = rcSimpleQuery( Conn, &simpleQueryInp, &simpleQueryOut );
        lastCommandStatus = status;
        if ( status < 0 ) {
            char *myName;
            char *mySubName;
            myName = rodsErrorName( status, &mySubName );
            rodsLog( LOG_ERROR, "rcSimpleQuery failed with error %d %s %s",
                     status, myName, mySubName );
            printf( "Error getting local zone\n" );
            return status;
        }
        strncpy( localZone, simpleQueryOut->outBuf, BIG_STR );
        i = strlen( localZone );
        for ( ; i > 1; i-- ) {
            if ( localZone[i] == '\n' ) {
                localZone[i] = '\0';
                if ( localZone[i - 1] == ' ' ) {
                    localZone[i - 1] = '\0';
                }
                break;
            }
        }
    }
    return 0;
}

/*
   print the results of a general query for the showGroup function below
*/
void // JMC - backport 4742
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

int
showGroup( char *groupName ) { // JMC - backport 4742
    // =-=-=-=-=-=-=-
    // JMC - backport 4742
    genQueryInp_t  genQueryInp;
    genQueryOut_t *genQueryOut = 0;
    int selectIndexes[10];
    int selectValues[10];
    int conditionIndexes[10];
    char *conditionValues[10];
    char conditionString1[BIG_STR];
    char conditionString2[BIG_STR];
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
    sprintf( conditionString1, "='rodsgroup'" );
    conditionValues[0] = conditionString1;

    genQueryInp.sqlCondInp.inx = conditionIndexes;
    genQueryInp.sqlCondInp.value = conditionValues;
    genQueryInp.sqlCondInp.len = 1;

    if ( groupName != NULL && *groupName != '\0' ) {

        sprintf( conditionString1, "!='rodsgroup'" );

        conditionIndexes[1] = COL_USER_GROUP_NAME;
        sprintf( conditionString2, "='%s'", groupName );
        conditionValues[1] = conditionString2;
        genQueryInp.sqlCondInp.len = 2;
    }

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        printf( "No rows found\n" );
        return -1;
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
    return 0;
    // =-=-=-=-=-=-=-
}

int
showFile( char *file ) {
    simpleQueryInp_t simpleQueryInp;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    if ( file == 0 || *file == '\0' ) {
        printf( "Need to specify a data_id number\n" );
        return USER__NULL_INPUT_ERR;
    }
    simpleQueryInp.form = 2;
    simpleQueryInp.sql = "select * from R_DATA_MAIN where data_id=?";
    simpleQueryInp.arg1 = file;
    simpleQueryInp.maxBufSize = 1024;
    return doSimpleQuery( simpleQueryInp );
}

int
showDir( char *dir ) {
    simpleQueryInp_t simpleQueryInp;
    int status;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;

    if ( dir == 0 || *dir == '\0' ) {
        dir = "/";
    }
    printf( "Contents of collection %s\n", dir );

    simpleQueryInp.form = 1;
    simpleQueryInp.sql = "select data_name, data_id, data_repl_num from R_DATA_MAIN where coll_id =(select coll_id from R_COLL_MAIN where coll_name=?)";
    simpleQueryInp.arg1 = dir;
    simpleQueryInp.maxBufSize = 1024;
    if ( debug ) {
        simpleQueryInp.maxBufSize = 20;
    }

    printf( "Files (data objects) (name, data_id, repl_num):\n" );
    status = doSimpleQuery( simpleQueryInp );
    if ( status < 0 ) {
        // error case
    }

    simpleQueryInp.form = 1;
    simpleQueryInp.sql =
        "select coll_name from R_COLL_MAIN where parent_coll_name=?";
    simpleQueryInp.arg1 = dir;
    simpleQueryInp.maxBufSize = 1024;
    printf( "Subcollections:\n" );
    return doSimpleQuery( simpleQueryInp );
}

int
showUser( char *user ) {
    simpleQueryInp_t simpleQueryInp;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    if ( *user != '\0' ) {
        simpleQueryInp.form = 2;
        simpleQueryInp.sql = "select * from R_USER_MAIN where user_name=?";
        simpleQueryInp.arg1 = user;
        simpleQueryInp.maxBufSize = 1024;
    }
    else {
        simpleQueryInp.form = 1;
        simpleQueryInp.sql = "select user_name||'#'||zone_name from R_USER_MAIN where user_type_name != 'rodsgroup'";
        simpleQueryInp.maxBufSize = 1024;
    }
    return doSimpleQuery( simpleQueryInp );
}

int
showUserAuth( char *user, char *zone ) {
    simpleQueryInp_t simpleQueryInp;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    simpleQueryInp.form = 1;
    if ( *user != '\0' ) {
        if ( *zone == '\0' ) {
            simpleQueryInp.sql = "select user_name, user_auth_name from R_USER_AUTH, R_USER_MAIN where R_USER_AUTH.user_id = R_USER_MAIN.user_id and R_USER_MAIN.user_name=?";
            simpleQueryInp.arg1 = user;
        }
        else {
            simpleQueryInp.sql = "select user_name, user_auth_name from R_USER_AUTH, R_USER_MAIN where R_USER_AUTH.user_id = R_USER_MAIN.user_id and R_USER_MAIN.user_name=? and R_USER_MAIN.zone_name=?";
            simpleQueryInp.arg1 = user;
            simpleQueryInp.arg2 = zone;
        }
        simpleQueryInp.maxBufSize = 1024;
    }
    else {
        simpleQueryInp.sql = "select user_name, user_auth_name from R_USER_AUTH, R_USER_MAIN where R_USER_AUTH.user_id = R_USER_MAIN.user_id";
        simpleQueryInp.maxBufSize = 1024;
    }
    return doSimpleQuery( simpleQueryInp );
}

int
showUserAuthName( char *authName )

{
    simpleQueryInp_t simpleQueryInp;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    simpleQueryInp.form = 1;
    simpleQueryInp.sql = "select user_name, user_auth_name from R_USER_AUTH, R_USER_MAIN where R_USER_AUTH.user_id = R_USER_MAIN.user_id and R_USER_AUTH.user_auth_name=?";
    simpleQueryInp.arg1 = authName;
    simpleQueryInp.maxBufSize = 1024;
    return doSimpleQuery( simpleQueryInp );
}

int
showUserOfZone( char *zone, char *user ) {
    simpleQueryInp_t simpleQueryInp;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    if ( *user != '\0' ) {
        simpleQueryInp.form = 2;
        simpleQueryInp.sql = "select * from R_USER_MAIN where user_name=? and zone_name=?";
        simpleQueryInp.arg1 = user;
        simpleQueryInp.arg2 = zone;
        simpleQueryInp.maxBufSize = 1024;
    }
    else {
        simpleQueryInp.form = 1;
        simpleQueryInp.sql = "select user_name from R_USER_MAIN where zone_name=? and user_type_name != 'rodsgroup'";
        simpleQueryInp.arg1 = zone;
        simpleQueryInp.maxBufSize = 1024;
    }
    return doSimpleQuery( simpleQueryInp );
}

int
simpleQueryCheck() {
    int status;
    simpleQueryInp_t simpleQueryInp;
    simpleQueryOut_t *simpleQueryOut;

    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );

    simpleQueryInp.control = 0;
    simpleQueryInp.form = 2;
    simpleQueryInp.sql = "select * from R_RESC_MAIN where resc_name=?";
    simpleQueryInp.arg1 = "foo";
    simpleQueryInp.maxBufSize = 1024;

    status = rcSimpleQuery( Conn, &simpleQueryInp, &simpleQueryOut );

    if ( status == CAT_NO_ROWS_FOUND ) {
        status = 0;    /* success */
    }

    return status;
}

int
showGlobalQuotas( char *inputUserOrGroup ) {
    simpleQueryInp_t simpleQueryInp;
    char userName[NAME_LEN];
    char zoneName[NAME_LEN];
    int status;

    if ( inputUserOrGroup == 0 || *inputUserOrGroup == '\0' ) {
        printf( "\nGlobal (total usage) quotas (if any) for users/groups:\n" );
    }
    else {
        printf( "\nGlobal (total usage) quotas (if any) for user/group %s:\n",
                inputUserOrGroup );
    }
    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    if ( inputUserOrGroup == 0 || *inputUserOrGroup == '\0' ) {
        simpleQueryInp.form = 2;
        simpleQueryInp.sql =
            "select user_name, R_USER_MAIN.zone_name, quota_limit, quota_over, R_QUOTA_MAIN.modify_ts from R_QUOTA_MAIN, R_USER_MAIN where R_USER_MAIN.user_id = R_QUOTA_MAIN.user_id and R_QUOTA_MAIN.resc_id = 0";
        simpleQueryInp.maxBufSize = 1024;
    }
    else {
        status = getLocalZone();
        if ( status ) {
            return status;
        }
        status = parseUserName( inputUserOrGroup, userName, zoneName );
        if ( zoneName[0] == '\0' ) {
            snprintf( zoneName, sizeof( zoneName ), "%s", localZone );
        }
        simpleQueryInp.form = 2;
        simpleQueryInp.sql =
            "select user_name, R_USER_MAIN.zone_name, quota_limit, quota_over, R_QUOTA_MAIN.modify_ts from R_QUOTA_MAIN, R_USER_MAIN where R_USER_MAIN.user_id = R_QUOTA_MAIN.user_id and R_QUOTA_MAIN.resc_id = 0 and user_name=? and R_USER_MAIN.zone_name=?";
        simpleQueryInp.arg1 = userName;
        simpleQueryInp.arg2 = zoneName;
        simpleQueryInp.maxBufSize = 1024;
    }
    return doSimpleQuery( simpleQueryInp );
}

int
showResourceQuotas( char *inputUserOrGroup ) {
    simpleQueryInp_t simpleQueryInp;
    char userName[NAME_LEN];
    char zoneName[NAME_LEN];
    int status;

    if ( inputUserOrGroup == 0 || *inputUserOrGroup == '\0' ) {
        printf( "Per resource quotas (if any) for users/groups:\n" );
    }
    else {
        printf( "Per resource quotas (if any) for user/group %s:\n",
                inputUserOrGroup );
    }
    memset( &simpleQueryInp, 0, sizeof( simpleQueryInp_t ) );
    simpleQueryInp.control = 0;
    if ( inputUserOrGroup == 0 || *inputUserOrGroup == '\0' ) {
        simpleQueryInp.form = 2;
        simpleQueryInp.sql =
            "select user_name, R_USER_MAIN.zone_name, resc_name, quota_limit, quota_over, R_QUOTA_MAIN.modify_ts from R_QUOTA_MAIN, R_USER_MAIN, R_RESC_MAIN where R_USER_MAIN.user_id = R_QUOTA_MAIN.user_id and R_RESC_MAIN.resc_id = R_QUOTA_MAIN.resc_id";
        simpleQueryInp.maxBufSize = 1024;
    }
    else {
        status = getLocalZone();
        if ( status ) {
            return status;
        }
        status = parseUserName( inputUserOrGroup, userName, zoneName );
        if ( zoneName[0] == '\0' ) {
            snprintf( zoneName, sizeof( zoneName ), "%s", localZone );
        }
        simpleQueryInp.form = 2;
        simpleQueryInp.sql = "select user_name, R_USER_MAIN.zone_name, resc_name, quota_limit, quota_over, R_QUOTA_MAIN.modify_ts from R_QUOTA_MAIN, R_USER_MAIN, R_RESC_MAIN where R_USER_MAIN.user_id = R_QUOTA_MAIN.user_id and R_RESC_MAIN.resc_id = R_QUOTA_MAIN.resc_id and user_name=? and R_USER_MAIN.zone_name=?";
        simpleQueryInp.arg1 = userName;
        simpleQueryInp.arg2 = zoneName;
        simpleQueryInp.maxBufSize = 1024;
    }
    return doSimpleQuery( simpleQueryInp );
}

int
generalAdmin( int userOption, char *arg0, char *arg1, char *arg2, char *arg3,
              char *arg4, char *arg5, char *arg6, char *arg7, char* arg8, char* arg9,
              rodsArguments_t* _rodsArgs = 0 ) {
    /* If userOption is 1, try userAdmin if generalAdmin gets a permission
     * failure */

    generalAdminInp_t generalAdminInp;
    userAdminInp_t userAdminInp;
    int status;
    char *mySubName;
    char *myName;
    char *funcName;

    if ( _rodsArgs && _rodsArgs->dryrun ) {
        arg3 = "--dryrun";
    }

    generalAdminInp.arg0 = arg0;
    generalAdminInp.arg1 = arg1;
    generalAdminInp.arg2 = arg2;
    generalAdminInp.arg3 = arg3;
    generalAdminInp.arg4 = arg4;
    generalAdminInp.arg5 = arg5;
    generalAdminInp.arg6 = arg6;
    generalAdminInp.arg7 = arg7;
    generalAdminInp.arg8 = arg8;
    generalAdminInp.arg9 = arg9;

    status = rcGeneralAdmin( Conn, &generalAdminInp );
    lastCommandStatus = status;
    funcName = "rcGeneralAdmin";

    if ( userOption == 1 && status == SYS_NO_API_PRIV ) {
        userAdminInp.arg0 = arg0;
        userAdminInp.arg1 = arg1;
        userAdminInp.arg2 = arg2;
        userAdminInp.arg3 = arg3;
        userAdminInp.arg4 = arg4;
        userAdminInp.arg5 = arg5;
        userAdminInp.arg6 = arg6;
        userAdminInp.arg7 = arg7;
        userAdminInp.arg8 = arg8;
        userAdminInp.arg9 = arg9;
        status = rcUserAdmin( Conn, &userAdminInp );
        funcName = "rcGeneralAdmin and rcUserAdmin";
    }

    // =-=-=-=-=-=-=-
    // JMC :: for 'dryrun' option on rmresc we need to capture the
    //     :: return value and simply output either SUCCESS or FAILURE
    // rm resource dryrun BOOYA
    if ( _rodsArgs &&
            _rodsArgs->dryrun == true &&
            0 == strcmp( arg0, "rm" ) &&
            0 == strcmp( arg1, "resource" ) ) {
        if ( 0 == status ) {
            printf( "DRYRUN REMOVING RESOURCE [%s - %d] :: SUCCESS\n", arg2, status );
        }
        else {
            printf( "DRYRUN REMOVING RESOURCE [%s - %d] :: FAILURE\n", arg2, status );
        } // else
    }
    else if ( status < 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        myName = rodsErrorName( status, &mySubName );
        rodsLog( LOG_ERROR, "%s failed with error %d %s %s", funcName, status, myName, mySubName );
        if ( status == CAT_INVALID_USER_TYPE ) {
            printf( "See 'lt user_type' for a list of valid user types.\n" );
        }
    } // else if status < 0

    printErrorStack( Conn->rError );
    freeRErrorContent( Conn->rError );

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
    char *cpStat;

    memset( ttybuf, 0, BIG_STR );
    fputs( "iadmin>", stdout );
    cpStat = fgets( ttybuf, BIG_STR, stdin );
    if ( cpStat == NULL ) {
        ttybuf[0] = 'q';
        ttybuf[1] = '\n';
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
            return 0;
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
    return 0;
}

/* handle a command,
   return code is 0 if the command was (at least partially) valid,
   -1 for quitting,
   -2 for if invalid
   -3 if empty.
*/
int
doCommand( char *cmdToken[], rodsArguments_t* _rodsArgs = 0 ) {
    char buf0[MAX_PASSWORD_LEN + 10];
    char buf1[MAX_PASSWORD_LEN + 10];
    char buf2[MAX_PASSWORD_LEN + 100];
    if ( veryVerbose ) {
        int i;
        printf( "executing command:" );
        for ( i = 0; i < 20 && strlen( cmdToken[i] ) > 0; i++ ) {
            printf( " %s", cmdToken[i] );
        }
        printf( "\n" );
        fflush( stderr );
        fflush( stdout );
    }

    if ( strcmp( cmdToken[0], "help" ) == 0 ||
            strcmp( cmdToken[0], "h" ) == 0 ) {
        usage( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "quit" ) == 0 ||
            strcmp( cmdToken[0], "q" ) == 0 ) {
        return -1;
    }
    if ( strcmp( cmdToken[0], "lu" ) == 0 ) {
        char userName[NAME_LEN] = "";
        char zoneName[NAME_LEN] = "";
        int status = parseUserName( cmdToken[1], userName, zoneName );
        if ( status < 0 ) {
            // error case
        }

        if ( zoneName[0] != '\0' ) {
            showUserOfZone( zoneName, userName );
        }
        else {
            showUser( cmdToken[1] );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "luz" ) == 0 ) {
        showUserOfZone( cmdToken[1], cmdToken[2] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lt" ) == 0 ) {
        if ( strcmp( cmdToken[1], "resc_type" ) == 0 ) {
            generalAdmin( 0, "lt", "resc_type", "", "", "", "", "", "", "", "" );
        }
        else {
            showToken( cmdToken[1], cmdToken[2] );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "lr" ) == 0 ) {
        showResc( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "ls" ) == 0 ) {
        showDir( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lf" ) == 0 ) {
        showFile( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lz" ) == 0 ) {
        showZone( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lg" ) == 0 ) {
        showGroup( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lgd" ) == 0 ) {
        if ( *cmdToken[1] == '\0' ) {
            printf( "You must specify a group with the lgd command\n" );
        }
        else {
            showUser( cmdToken[1] );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "lrg" ) == 0 ) {
        printf( "Resource groups are deprecated.\n" );
        printf( "Please investigate the available coordinating resource plugins.\n" );
        printf( "(e.g. random, replication, etc.)\n" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "mkuser" ) == 0 ) {
        int status;
        char userName[NAME_LEN];
        char zoneName[NAME_LEN];
        status = parseUserName( cmdToken[1], userName, zoneName );
        if ( status ) {
            printf( "Invalid user name format\n" );
            return USER_INVALID_USERNAME_FORMAT;
        }
        status = getLocalZone();
        if ( status ) {
            return status;
        }
        if ( strcmp( zoneName, localZone ) == 0 ) {
            /* User entered user#localZone but call generalAdmin
               without #localZone as is needed to differentiate
               local and remote user creation */
            generalAdmin( 0, "add", "user", userName, cmdToken[2], "",
                          cmdToken[3], cmdToken[4], cmdToken[5], "", "" );
        }
        else {
            generalAdmin( 0, "add", "user", cmdToken[1], cmdToken[2], "",
                          cmdToken[3], cmdToken[4], cmdToken[5], "", "" );  /* "" is unused
                                                                              zoneName as that's part of the username now */
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "moduser" ) == 0 ) {
        if ( strcmp( cmdToken[2], "password" ) == 0 ) {
            int i, len, lcopy;
            char *key2;
            /* this is a random string used to pad, arbitrary, but must match
               the server side: */
            char rand[] = "1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs";

            strncpy( buf0, cmdToken[3], MAX_PASSWORD_LEN );
            len = strlen( cmdToken[3] );
            lcopy = MAX_PASSWORD_LEN - 10 - len;
            if ( lcopy > 15 ) { /* server will look for 15 characters of random string */
                strncat( buf0, rand, lcopy );
            }
            i = obfGetPw( buf1 );
            if ( i != 0 ) {
#ifdef WIN32
                HANDLE hStdin = GetStdHandle( STD_INPUT_HANDLE );
                DWORD mode;
                GetConsoleMode( hStdin, &mode );
                DWORD lastMode = mode;
                mode &= ~ENABLE_ECHO_INPUT;
                BOOL error = !SetConsoleMode( hStdin, mode );
                int errsv = -1;
#else
                struct termios tty;
                tcgetattr( STDIN_FILENO, &tty );
                tcflag_t oldflag = tty.c_lflag;
                tty.c_lflag &= ~ECHO;
                int error = tcsetattr( STDIN_FILENO, TCSANOW, &tty );
                int errsv = errno;
#endif
                if ( error ) {
                    printf( "WARNING: Error %d disabling echo mode. Password will be displayed in plain text.", errsv );
                }
                printf( "Enter your current iRODS password:" );
                std::string password = "";
                getline( std::cin, password );
                strncpy( buf1, password.c_str(), MAX_PASSWORD_LEN );
#ifdef WIN32
                if ( !SetConsoleMode( hStdin, lastMode ) ) {
                    printf( "Error reinstating echo mode." );
                }
#else
                tty.c_lflag = oldflag;
                if ( tcsetattr( STDIN_FILENO, TCSANOW, &tty ) ) {
                    printf( "Error reinstating echo mode." );
                }
#endif

            }
            key2 = getSessionSignatureClientside();
            obfEncodeByKeyV2( buf0, buf1, key2, buf2 );
            cmdToken[3] = buf2;
        }
        generalAdmin( 0, "modify", "user", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "aua" ) == 0 ) {
        generalAdmin( 0, "modify", "user", cmdToken[1], "addAuth",
                      cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rua" ) == 0 ) {
        generalAdmin( 0, "modify", "user", cmdToken[1], "rmAuth",
                      cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rpp" ) == 0 ) {
        generalAdmin( 0, "modify", "user", cmdToken[1], "rmPamPw",
                      cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lua" ) == 0 ) {
        char userName[NAME_LEN];
        char zoneName[NAME_LEN];
        int status;
        status = parseUserName( cmdToken[1], userName, zoneName );
        if ( status < 0 ) {
            // error case
        }

        if ( zoneName[0] != '\0' ) {
            showUserAuth( userName, zoneName );
        }
        else {
            showUserAuth( cmdToken[1], "" );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "luan" ) == 0 ) {
        showUserAuthName( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "cu" ) == 0 ) {
        generalAdmin( 0, "calculate-usage", "", "", "",
                      "", "", "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "suq" ) == 0 ) {
        generalAdmin( 0, "set-quota", "user",
                      cmdToken[1], cmdToken[2], cmdToken[3],
                      "", "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "sgq" ) == 0 ) {
        generalAdmin( 0, "set-quota", "group",
                      cmdToken[1], cmdToken[2], cmdToken[3],
                      "", "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lq" ) == 0 ) {
        showResourceQuotas( cmdToken[1] );
        showGlobalQuotas( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "mkdir" ) == 0 ) {
        if ( _rodsArgs->force == True ) {
            int path_index = 1;
#ifdef osx_platform
            path_index = 2;
#endif
            generalAdmin( 0, "add", "dir",
                          cmdToken[path_index], "",
                          "", "",
                          "", "",
                          "", "" );
        }
        else {
            usage( "mkdir" );
        }

        return 0;
    }

    if ( strcmp( cmdToken[0], "mkresc" ) == 0 ) {
        // trim spaces in resource type string
        std::string resc_type( cmdToken[2] );
        resc_type.erase( std::remove_if( resc_type.begin(), resc_type.end(), ::isspace ), resc_type.end() );

        // tell the user what they are doing
        std::cout << "Creating resource:" << std::endl;
        std::cout << "Name:\t\t\"" << cmdToken[1] << "\"" << std::endl;
        std::cout << "Type:\t\t\"" << cmdToken[2] << "\"" << std::endl;
        if ( cmdToken[3] != NULL && strlen( cmdToken[3] ) > 0 ) {
            std::string host_path( cmdToken[3] );
            std::size_t colon_pos = host_path.find( ":" );
            std::string host = host_path.substr( 0, colon_pos );
            std::cout << "Host:\t\t\"" << host << "\"" << std::endl;
            if ( colon_pos != std::string::npos ) {
                std::string path = host_path.substr( colon_pos + 1, host_path.length() - colon_pos );
                std::cout << "Path:\t\t\"" << path << "\"" << std::endl;
            }
            else {
                std::cout << "Path:\t\t\"\"" << std::endl;
            }
        }
        else {
            std::cout << "Host:\t\t\"\"" << std::endl;
            std::cout << "Path:\t\t\"\"" << std::endl;
        }
        if ( cmdToken[4] != NULL && strlen( cmdToken[4] ) > 0 ) {
            std::cout << "Context:\t\"" << cmdToken[4] << "\"" << std::endl;;
        }
        else {
            std::cout << "Context:\t\"\"" << std::endl;
        }

        generalAdmin( 0, "add", "resource", cmdToken[1], ( char * )resc_type.c_str(),
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], cmdToken[7], cmdToken[8] );
        /* (add resource name type host:path contextstring) */
        return 0;
    }
    if ( strcmp( cmdToken[0], "addchildtoresc" ) == 0 ) {

        generalAdmin( 0, "add", "childtoresc", cmdToken[1], cmdToken[2],
                      cmdToken[3], "", "", "", "", "" );
        /* (add childtoresc parent child context) */
        return 0;
    }
    if ( strcmp( cmdToken[0], "rmchildfromresc" ) == 0 ) {

        generalAdmin( 0, "rm", "childfromresc", cmdToken[1], cmdToken[2],
                      "", "", "", "", "", "" );
        /* (rm childfromresc parent child) */
        return 0;
    }

    if ( strcmp( cmdToken[0], "modrescdatapaths" ) == 0 ) {
        printf(
            "Warning, this command, more than others, is relying on your direct\n"
            "input to modify iCAT tables (potentially, many rows).  It will do a\n"
            "string pattern find and replace operation on the main data-object\n"
            "table for the paths at which the physical files are stored. If you\n"
            "are not sure what you are doing, do not run this command.  You may\n"
            "want to backup the iCAT database before running this.  See the help\n"
            "text for more information.\n"
            "\n"
            "Are you sure you want to run this command? [y/N]:" );
        std::string response = "";
        getline( std::cin, response );
        if ( response == "y" || response == "yes" ) {
            printf( "OK, performing the resource data paths update\n" );

            generalAdmin( 0, "modify", "resourcedatapaths", cmdToken[1],
                          cmdToken[2], cmdToken[3], cmdToken[4], "", "", "", "" );
        }
        return 0;
    }

    if ( strcmp( cmdToken[0], "modresc" ) == 0 ) {
        if ( strcmp( cmdToken[2], "name" ) == 0 )       {
            printf(
                "If you modify a resource name, you and other users will need to\n" );
            printf( "change your irods_environment.json files to use it, you may need to update\n" );
            printf( "server_config.json and, if rules use the resource name, you'll need to\n" );
            printf( "update the core rules (core.re).  This command will update various\n" );
            printf( "tables with the new name.\n" );
            printf( "Do you really want to modify the resource name? (enter y or yes to do so):" );
            std::string response = "";
            getline( std::cin, response );
            if ( response == "y" || response == "yes" ) {
                printf( "OK, performing the resource rename\n" );
                int stat;
                stat = generalAdmin( 0, "modify", "resource", cmdToken[1], cmdToken[2],
                                     cmdToken[3], "", "", "", "", "" );
                if ( strcmp( cmdToken[2], "path" ) == 0 ) {
                    if ( stat == 0 ) {
                        printf( "Modify resource path was successful.\n" );
                        printf( "If the existing iRODS files have been physically moved,\n" );
                        printf( "you may want to run 'iadmin modrescdatapaths' with the old\n" );
                        printf( "and new path.  See 'iadmin h modrescdatapaths' for more information.\n" );
                    }
                }

            }
            else {
                printf( "Resource rename aborted\n" );
            }
        }
        else {
            generalAdmin( 0, "modify", "resource", cmdToken[1], cmdToken[2],
                          cmdToken[3], "", "", "", "", "" );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "mkzone" ) == 0 ) {
        generalAdmin( 0, "add", "zone", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "modzone" ) == 0 ) {
        if ( strcmp( myEnv.rodsZone, cmdToken[1] ) == 0 &&
                strcmp( cmdToken[2], "name" ) == 0 )       {
            printf(
                "If you modify the local zone name, you and other users will need to\n" );
            printf( "change your irods_environment.json files to use it, you may need to update\n" );
            printf( "server_config.json and, if rules use the zone name, you'll need to update\n" );
            printf( "core.re.  This command will update various tables with the new name\n" );
            printf( "and rename the top-level collection.\n" );
            printf( "Do you really want to modify the local zone name? (enter y or yes to do so):" );
            std::string response = "";
            getline( std::cin, response );
            if ( response == "y" || response == "yes" ) {
                printf( "OK, performing the local zone rename\n" );
                generalAdmin( 0, "modify", "localzonename", cmdToken[1], cmdToken[3],
                              "", "", "", "", "", "" );
            }
            else {
                printf( "Local zone rename aborted\n" );
            }
        }
        else {
            generalAdmin( 0, "modify", "zone", cmdToken[1], cmdToken[2],
                          cmdToken[3], "", "", "", "", "" );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "modzonecollacl" ) == 0 ) {
        generalAdmin( 0, "modify", "zonecollacl", cmdToken[1], cmdToken[2],
                      cmdToken[3], "", "", "", "", "" );
        return 0;
    }


    if ( strcmp( cmdToken[0], "rmzone" ) == 0 ) {
        generalAdmin( 0, "rm", "zone", cmdToken[1], "",
                      "", "", "", "", "", "" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "mkgroup" ) == 0 ) {
        generalAdmin( 0, "add", "user", cmdToken[1], "rodsgroup",
                      myEnv.rodsZone, "", "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rmgroup" ) == 0 ) {
        generalAdmin( 0, "rm", "user", cmdToken[1],
                      myEnv.rodsZone, "", "", "", "", "", "" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "atg" ) == 0 ) {
        generalAdmin( 1, "modify", "group", cmdToken[1], "add", cmdToken[2],
                      cmdToken[3], "", "", "", "" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "rfg" ) == 0 ) {
        generalAdmin( 1, "modify", "group", cmdToken[1], "remove", cmdToken[2],
                      cmdToken[3], "", "", "", "" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "atrg" ) == 0 ) {
        printf( "Resource groups are deprecated.\n" );
        printf( "Please investigate the available coordinating resource plugins.\n" );
        printf( "(e.g. random, replication, etc.)\n" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "rfrg" ) == 0 ) {
        printf( "Resource groups are deprecated.\n" );
        printf( "Please investigate the available coordinating resource plugins.\n" );
        printf( "(e.g. random, replication, etc.)\n" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "rmresc" ) == 0 ) {
        generalAdmin( 0, "rm", "resource", cmdToken[1], cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "", _rodsArgs );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rmdir" ) == 0 ) {
        generalAdmin( 0, "rm", "dir", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rmuser" ) == 0 ) {
        int status;
        char userName[NAME_LEN];
        char zoneName[NAME_LEN];
        status = parseUserName( cmdToken[1], userName, zoneName );
        /* just check for format */
        if ( status ) {
            printf( "Invalid user name format\n" );
            return 0;
        }
        /* also check for current user, should not be able to rm own account */
        if ( strcmp( userName, myEnv.rodsUserName ) == 0 ) {
            if ( ( strcmp( zoneName, "" ) == 0 ) || ( strcmp( zoneName, myEnv.rodsZone ) == 0 ) ) {
                printf( "Cannot remove currently authenticated user (yourself)\n" );
                return 0;
            }
        }
        generalAdmin( 0, "rm", "user", cmdToken[1],
                      cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "at" ) == 0 ) {
        generalAdmin( 0, "add", "token", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rt" ) == 0 ) {
        generalAdmin( 0, "rm", "token", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "spass" ) == 0 ) {
        char scrambled[MAX_PASSWORD_LEN + 100];
        if ( strlen( cmdToken[1] ) > MAX_PASSWORD_LEN - 2 ) {
            printf( "Password exceeds maximum length\n" );
        }
        else {
            if ( strlen( cmdToken[2] ) == 0 ) {
                printf( "Warning, scramble key is null\n" );
            }
            obfEncodeByKey( cmdToken[1], cmdToken[2], scrambled );
            printf( "Scrambled form is:%s\n", scrambled );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "dspass" ) == 0 ) {
        char unscrambled[MAX_PASSWORD_LEN + 100];
        if ( simpleQueryCheck() != 0 ) {
            exit( -1 ); /* not authorized */
        }
        if ( strlen( cmdToken[1] ) > MAX_PASSWORD_LEN - 2 ) {
            printf( "Scrambled password exceeds maximum length\n" );
        }
        else {
            if ( strlen( cmdToken[2] ) == 0 ) {
                printf( "Warning, scramble key is null\n" );
            }
            obfDecodeByKey( cmdToken[1], cmdToken[2], unscrambled );
            printf( "Unscrambled form is:%s\n", unscrambled );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "ctime" ) == 0 ) {
        char myString[TIME_LEN];
        if ( strcmp( cmdToken[1], "str" ) == 0 ) {
            int status;
            status = checkDateFormat( cmdToken[2] );
            if ( status ) {
                rodsLogError( LOG_ERROR, status, "ctime str:checkDateFormat error" );
            }
            printf( "Converted to local iRODS integer time: %s\n", cmdToken[2] );
            return 0;
        }
        if ( strcmp( cmdToken[1], "now" ) == 0 ) {
            char nowString[100];
            getNowStr( nowString );
            printf( "Current time as iRODS integer time: %s\n", nowString );
            return 0;
        }
        getLocalTimeFromRodsTime( cmdToken[1], myString );
        printf( "Converted to local time: %s\n", myString );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rum" ) == 0 ) {
        int status;
        status = generalAdmin( 0, "rm", "unusedAVUs", "", "",
                               "", "", "", "", "", "" );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            printf(
                "The return of CAT_SUCCESS_BUT_WITH_NO_INFO in this case means that the\n" );
            printf(
                "SQL succeeded but there were no rows removed; there were no unused\n" );
            printf(
                "AVUs to remove.\n" );
            lastCommandStatus = 0;

        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "asq" ) == 0 ) {
        int status;
        status = generalAdmin( 0, "add", "specificQuery", cmdToken[1],
                               cmdToken[2], "", "", "", "", "", "" );
        return status;
    }
    if ( strcmp( cmdToken[0], "rsq" ) == 0 ) {
        int status;
        status = generalAdmin( 0, "rm", "specificQuery", cmdToken[1],
                               "", "", "", "", "", "", "" );
        return status;
    }

    /* test is only used for testing so is not included in the help */
    if ( strcmp( cmdToken[0], "test" ) == 0 ) {
        char* result;
        result = obfGetMD5Hash( cmdToken[1] );
        printf( "md5:%s\n", result );
        return 0;
    }
    /* 2spass is only used for testing so is not included in the help */
    if ( strcmp( cmdToken[0], "2spass" ) == 0 ) {
        char scrambled[MAX_PASSWORD_LEN + 10];
        obfEncodeByKeyV2( cmdToken[1], cmdToken[2], cmdToken[3], scrambled );
        printf( "Version 2 scrambled form is:%s\n", scrambled );
        return 0;
    }
    /* 2dspass is only used for testing so is not included in the help */
    if ( strcmp( cmdToken[0], "2dspass" ) == 0 ) {
        char unscrambled[MAX_PASSWORD_LEN + 10];
        obfDecodeByKeyV2( cmdToken[1], cmdToken[2], cmdToken[3], unscrambled );
        printf( "Version 2 unscrambled form is:%s\n", unscrambled );
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

    status = parseCmdLineOpt( argc, argv, "fvVhZ", 1, &myRodsArgs );

#ifdef osx_platform
    // getopt has different behavior on OSX, we work around this for
    // the one specific instance where mkdir is use with a force flag
    // this will be refactored in the future to use the boost::program_options
    // to remove the need for this gratuitous hack
    if ( argc > 2 ) {
        std::string sub_cmd   = argv[1];
        std::string force_flg = argv[2];
        if ( "mkdir" == sub_cmd && "-f" == force_flg ) {
            myRodsArgs.force = True;
        }
    } // if argc
#endif


    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 2 );
    }
    if ( myRodsArgs.help == True ) {
        usage( "" );
        exit( 0 );
    }
    argOffset = myRodsArgs.optind;

    memset( &myEnv, 0, sizeof( myEnv ) );
    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    if ( myRodsArgs.veryVerbose == True ) {
        veryVerbose = 1;
    }

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
            if ( strlen( cmdToken[2] ) == 0 ) {
                printf( "Warning, scramble key is null\n" );
            }
            obfEncodeByKey( cmdToken[1], cmdToken[2], scrambled );
            printf( "Scrambled form is:%s\n", scrambled );
        }
        exit( 0 );
    }

    if ( strcmp( cmdToken[0], "ctime" ) == 0 ) {
        char myString[TIME_LEN];
        if ( strcmp( cmdToken[1], "str" ) == 0 ) {
            int status;
            status = checkDateFormat( cmdToken[2] );
            if ( status ) {
                rodsLogError( LOG_ERROR, status, "ctime str:checkDateFormat error" );
            }
            printf( "Converted to local iRODS integer time: %s\n", cmdToken[2] );
            exit( 0 );
        }
        if ( strcmp( cmdToken[1], "now" ) == 0 ) {
            char nowString[100];
            getNowStr( nowString );
            printf( "Current time as iRODS integer time: %s\n", nowString );
            exit( 0 );
        }
        getLocalTimeFromRodsTime( cmdToken[1], myString );
        printf( "Converted to local time: %s\n", myString );
        exit( 0 );
    }

    /* need to copy time convert commands up here too */

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
        if ( !debug ) {
            exit( 3 );
        }
    }

    keepGoing = 1;
    firstTime = 1;
    while ( keepGoing ) {
        int status;
        status = doCommand( cmdToken, &myRodsArgs );
        if ( status == -1 ) {
            keepGoing = 0;
        }
        if ( firstTime ) {
            if ( status == 0 ) {
                keepGoing = 0;
            }
            if ( status == -2 ) {
                keepGoing = 0;
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
        "Usage: iadmin [-hvV] [command]",
        "A blank execute line invokes the interactive mode, where it",
        "prompts and executes commands until 'quit' or 'q' is entered.",
        "Single or double quotes can be used to enter items with blanks.",
        "Commands are:",
        " lu [name[#Zone]] (list user info; details if name entered)",
        " lua [name[#Zone]] (list user authentication (GSI/Kerberos Names, if any))",
        " luan Name (list users associated with auth name (GSI/Kerberos)",
        " lt [name] [subname] (list token info)",
        " lr [name] (list resource info)",
        " ls [name] (list directory: subdirs and files)",
        " lz [name] (list zone info)",
        " lg [name] (list group info (user member list))",
        " lgd name  (list group details)",
        " lf DataId (list file details; DataId is the number (from ls))",
        " mkuser Name[#Zone] Type (make user)",
        " moduser Name[#Zone] [ type | zone | comment | info | password ] newValue",
        " aua Name[#Zone] Auth-Name (add user authentication-name (GSI/Kerberos)",
        " rua Name[#Zone] Auth-Name (remove user authentication name (GSI/Kerberos)",
        " rpp Name  (remove PAM-derived Password for user Name)",
        " rmuser Name[#Zone] (remove user, where userName: name[@department][#zone])",
        " rmdir Name (remove directory) ",
        " mkresc Name Type [Host:Path] [ContextString] (make Resource)",
        " modresc Name [name, type, host, path, status, comment, info, freespace, rebalance] Value (mod Resc)",
        " modrescdatapaths Name oldpath newpath [user] (update data-object paths,",
        "      sometimes needed after modresc path)",
        " rmresc Name (remove resource)",
        " addchildtoresc Parent Child [ContextString]",
        " rmchildfromresc Parent Child",
        " mkzone Name Type(remote) [Connection-info] [Comment] (make zone)",
        " modzone Name [ name | conn | comment ] newValue  (modify zone)",
        " modzonecollacl null|read userOrGroup /remotezone (set strict-mode root ACLs)",
        " rmzone Name (remove zone)",
        " mkgroup Name (make group)",
        " rmgroup Name (remove group)",
        " atg groupName userName[#Zone] (add to group - add a user to a group)",
        " rfg groupName userName[#Zone] (remove from group - remove a user from a group)",
        " at tokenNamespace Name [Value1] [Value2] [Value3] (add token) ",
        " rt tokenNamespace Name [Value1] (remove token) ",
        " spass Password Key (print a scrambled form of a password for DB)",
        " dspass Password Key (descramble a password and print it)",
        " ctime Time (convert an iRODS time (integer) to local time; & other forms)",
        " suq User ResourceName-or-'total' Value (set user quota)",
        " sgq Group ResourceName-or-'total' Value (set group quota)",
        " lq [Name] List Quotas",
        " cu (calulate usage (for quotas))",
        " rum (remove unused metadata (user-defined AVUs)",
        " asq 'SQL query' [Alias] (add specific query)",
        " rsq 'SQL query' or Alias (remove specific query)",
        " help (or h) [command] (this help, or more details on a command)",
        "Also see 'irmtrash -M -u user' for the admin mode of removing trash and",
        "similar admin modes in irepl, iphymv, and itrim.",
        "The admin can also alias as any user via the 'clientUserName' environment",
        "variable.",
        ""
    };
    printMsgs( Msgs );
    printReleaseInfo( "iadmin" );
}

void
usage( char *subOpt ) {
    char *luMsgs[] = {
        "lu [name] (list user info; details if name entered)",
        "list user information.  ",
        "Just 'lu' will briefly list currently defined users.",
        "If you include a user name, more detailed information is provided.",
        "Usernames can include the zone preceeded by #, for example rods#tempZone.",
        "Users are listed in the userName#ZoneName form.",
        "Also see the luz and lz and the iuserinfo command.",
        ""
    };
    char *luaMsgs[] = {
        "lua [name[#Zone]] (list user authentication (GSI/Kerberos Names), if any)",
        "list user authentication-names for one or all users",
        "Just 'lua' will list all the GSI/Kerberos names currently defined",
        "for all users along with the associated iRODS user names.",
        "If you include a user name, then the auth-names for that user are listed.",
        "Usernames can include the zone preceeded by #, for example rods#tempZone.",
        "Also see the 'luan', 'aua' and 'rua' and the 'iuserinfo' command.",
        ""
    };
    char *luanMsgs[] = {
        "luan Name (list users associated with auth name (GSI/Kerberos)",
        "list the user(s) associated with a give Authentication-Name  ",
        "For example:",
        "  luan '/C=US/O=INC/OU=DICE/CN=Wayne Schroeder/UID=schroeder'",
        "will list the iRODS user(s) with the GSI DN, if any.",
        ""
    };
    char *luzMsgs[] = {
        "luz Zone [User] (list user info for a Zone; details if name entered)",
        "list user information for users of a particular Zone.  ",
        "Just 'luz Zonename' will briefly list currently defined users of that Zone.",
        "If you include a user name, more detailed information is provided.",
        "Also see the lu and lz commands.",
        ""
    };
    char *ltMsgs[] = {
        "lt [name] [subname]",
        "list token information.  ",
        "Just 'lt' lists the types of tokens that are defined",
        "If you include a tokenname, it will list the values that are",
        "allowed for the token type.  For details, lt name subname, ",
        "for example: lt data_type email",
        "The sql wildcard character % can be used on the subname,",
        "for example: lt data_type %DLL",
        ""
    };
    char *lrMsgs[] = {
        "lr [name] (list resource info)",
        "Just 'lr' briefly lists the defined resources.",
        "If you include a resource name, it will list more detailed information.",
        ""
    };
    char *lsMsgs[] = {
        "ls [name] (list directory: subdirs and files)",
        "This was a test function used before we had the ils command.",
        "It lists collections and data-objects in a somewhat different",
        "way than ils.  This is seldom of value but has been left in for now.",
        ""
    };
    char *lzMsgs[] = {
        " lz [name] (list zone info)",
        "Just 'lz' briefly lists the defined zone(s).",
        "If you include a zone name, it will list more detailed information.",
        ""
    };
    char *lgMsgs[] = {
        " lg [name] (list group info (user member list))",
        "Just 'lg' briefly lists the defined groups.",
        "If you include a group name, it will list users who are",
        "members of that group.  Users are listed in the user#zone format.",
        "In addition to 'rodsadmin', any user can use this sub-command; this is",
        "of most value to 'groupadmin' users who can also 'atg' and 'rfg'",
        ""
    };

    char *lgdMsgs[] = {
        " lgd name (list group details)",
        "Lists some details about the user group.",
        ""
    };
    char *lfMsgs[] = {
        " lf DataId (list file details; DataId is the number (from ls))",
        "This was a test function used before we had the ils command.",
        "It lists data-objects in a somewhat different",
        "way than ils.  This is seldom of value but has been left in for now.",

        ""
    };
    char *mkuserMsgs[] = {
        " mkuser Name[#Zone] Type (make user)",
        "Create a new iRODS user in the ICAT database",
        " ",
        "Name is the user name to create",
        "Type is the user type (see 'lt user_type' for a list)",
        "Zone is the user's zone (for remote-zone users)",
        " ",
        "Tip: Use moduser to set a password or other attributes, "
        "     use 'aua' to add a user auth name (GSI DN or Kerberos Principal name)",
        ""
    };

    char *spassMsgs[] = {
        " spass Password Key (print a scrambled form of a password for DB)",
        "Scramble a password, using the input password and key.",
        "This is used during the installation for a little additional security",
        ""
    };

    char *dspassMsgs[] = {
        " dspass Password Key (descramble a password and print it)",
        "Descramble a password, using the input scrambled password and key",
        ""
    };


    char *moduserMsgs[] = {
        " moduser Name[#Zone] [ type | zone | comment | info | password ] newValue",
        "Modifies a field of an existing user definition.",
        "For password authentication, use moduser to set the password.",
        "(The password is transferred in a scrambled form to be more secure.)",
        "Long forms of the field names may also be used:",
        "user_name, user_type_name, zone_name, user_info, or ",
        "r_comment",
        "These are the names listed by 'lu' (and are the database table column names).",
        "Modifying the user's name (user_name) is not allowed; instead remove the user",
        "and create a new one.  rmuser/mkuser will remove (if empty) and create the",
        "needed collections too.",
        "For GSI or Kerberos authentication, use 'aua' to add one or more",
        "user auth names (GSI Distinquished Name (DN) or Kerberos principal name).",
        ""
    };

    char *auaMsgs[] = {
        " aua Name[#Zone] Auth-Name (add user authentication-name (GSI/Kerberos)",
        "Add a user authentication name, a GSI  Distinquished Name (DN) or",
        "Kerberos Principal name, to an iRODS user.  Multiple DNs and/or Principal",
        "names can be associated with each user.",
        "This is used with Kerberos and/or GSI authentication, if enabled.",
        "For example:",
        "  aua rods '/C=US/O=INC/OU=DICE/CN=Wayne Schroeder/UID=schroeder'",
        "Also see 'rua', 'lua', and 'luan'.",
        ""
    };

    char *ruaMsgs[] = {
        " rua Name[#Zone] Auth-Name (remove user authentication-name (GSI/Kerberos)",
        "Remove a user authentication name, a GSI  Distinquished Name (DN) or",
        "Kerberos Principal name, from being associated with an iRODS user.",
        "These are used with Kerberos and/or GSI authentication, if enabled.",
        "Also see 'aua', 'lua', and 'luan'.",
        ""
    };

    char *rppMsgs[] = {
        " rpp Name (remove PAM-derived Password for user Name)",
        "Remove irods short-term (ususally 2 weeks) passwords that are created",
        "when users authenticate via the iRODS PAM authentication method.",
        "For additional security, when using PAM (system passwords), 'iinit' will",
        "create a separate iRODS password that is then used (a subsequent 'iinit'",
        "extend its 'life').  If the user's system password is changed, you",
        "may want to use this rpp command to require the user to re-authenticate.",
        ""
    };


    char *rmuserMsgs[] = {
        " rmuser Name[#Zone] (remove user, where userName: name[@department][#zone])",
        " Remove an irods user.",
        ""
    };

    char *mkdirMsgs[] = {
        "***************************** WARNING ********************************",
        "This command is intended for installation purposes and should never be",
        "called directly by a user.  In order to make a collection please use",
        "the 'imkdir' icommand.",
        ""
    };

    char *rmdirMsgs[] = {
        " rmdir Name (remove directory) ",
        "This is similar to 'irm -f'.",
        ""
    };

    char *mkrescMsgs[] = {
        " mkresc Name Type [Host:Path] [ContextString] (make Resource)",
        "Create (register) a new coordinating or storage resource.",
        " ",
        "Name is the name of the new resource.",
        "Type is the resource type.",
        "Host is the DNS host name.",
        "Path is the defaultPath for the vault.",
        "ContextString is any contextual information relevant to this resource.",
        "  (semi-colon separated key=value pairs e.g. \"a=b;c=d\")",
        " ",
        "A ContextString can be added to a coordinating resource (where there is",
        "no hostname or vault path to be set) by explicitly setting the Host:Path",
        "to an empty string ('').",
        ""
    };

    char *modrescMsgs[] = {
        " modresc Name [name, type, host, path, status, comment, info, freespace, rebalance] Value",
        "         (modify Resource)",
        "Change some attribute of a resource.  For example:",
        "    modresc demoResc comment 'test resource'",
        "The 'host' field is the DNS host name, for example 'datastar.sdsc.edu',",
        "this is displayed as 'resc_net', the resource network address.",
        " ",
        "Setting the resource status to '" RESC_DOWN "' will cause iRODS to ignore that",
        "resource and bypass communications with that server.  '" RESC_UP "' or other strings",
        "without '" RESC_DOWN "' in them will restore use of the resource.  'auto' will allow",
        "the Resource Monitoring System (if running) to set the resource status",
        "to '" RESC_AUTO_UP "' or '" RESC_AUTO_DOWN "'.",
        " ",
        "The freespace value can be simply specified, or if it starts with + or -",
        "the freespace amount will be incremented or decremented by the value.",
        " ",
        "'rebalance' will trigger the rebalancing operation on a coordinating resource node.",
        ""
    };

    char *modrescDataPathsMsgs[] = {
        " modrescdatapaths Name oldpath newpath [user] (update data-object paths,",
        "      sometimes needed after modresc path)",

        " ",
        "Modify the paths for existing iRODS files (data-objects) to match a",
        "change of the resource path that had been done via 'iadmin modresc",
        "Resc path'.  This is only needed if the physical files and directories",
        "of existing iRODS files have been moved, via tools outside of iRODS;",
        "i.e the physical resource has been moved.  If you only changed the",
        "path so that new files will be stored under the new path directory,",
        "you do not need to run this.",
        " ",
        "Each data-object has a physical path associated with it.  If the old",
        "physical paths are no longer valid, you can update them via this.  It",
        "will change the beginning part of the path (the Vault) from the old",
        "path to the new.",
        " ",
        "This does a pattern replacement on the paths for files in the named",
        "resource.  The old and new path strings must begin and end with a",
        "slash (/) to make it more likely the correct patterns are replaced",
        "(should the pattern repeat within a path).",
        " ",
        "If you include the optional user, only iRODS files owned by that",
        "user will be updated.",
        " ",
        "When the command runs, it will tell you how many data-object rows",
        "have been updated.",
        " ",
        "The 'iadmin modresc Rescname path' command now returns the previous",
        "path of the resource which can be used as the oldPath for this",
        "modrescdatapaths command.  It also refers the user to this command.",
        " ",
        "To see if you have any files under a given path, use iquest, for example:",
        "iquest \"select count(DATA_ID) where DATA_PATH like '/iRODS/Vault3/%'\" ",
        "And to restrict it to a certain user add:",
        " and USER_NAME = 'name' ",
        ""
    };

    char *rmrescMsgs[] = {
        " rmresc Name (remove resource)",
        "Remove a storage resource.",
        ""
    };

    char* addchildtorescMsgs[] = {
        " addchildtoresc Parent Child [ContextString] (add child to resource)",
        "Add a child resource to a parent resource.  This creates an 'edge'",
        "between two nodes in a resource tree.",
        " ",
        "Parent is the name of the parent resource.",
        "Child is the name of the child resource.",
        "ContextString is any relevant information that the parent may need in order",
        "  to manage the child.",
        ""
    };

    char* rmchildfromrescMsgs[] = {
        " rmchildfromresc Parent Child (remove child from resource)",
        "Remove a child resource from a parent resource.  This removes an 'edge'",
        "between two nodes in a resource tree.",
        " ",
        "Parent is the name of the parent resource.",
        "Child is the name of the child resource.",
        ""
    };

    char *mkzoneMsgs[] = {
        " mkzone Name Type(remote) [Connection-info] [Comment] (make zone)",
        "Create a new zone definition.  Type must be 'remote' as the local zone",
        "must previously exist and there can be only one local zone definition.",
        "Connection-info (hostname:port) and a Comment field are optional.",
        " ",
        "The connection-info should be the hostname of the ICAT-Enabled-Server (IES)",
        "of the zone.  If it is a non-IES, remote users trying to connect will get",
        "a CAT_INVALID_USER error, even if valid, due to complications in the",
        "way the protocol connections operate when the local server tries to",
        "connect back to the remote zone to authenticate the user.",
        " ",
        "Also see modzone, rmzone, and lz.",
        ""
    };

    char *modzoneMsgs[] = {
        " modzone Name [ name | conn | comment ] newValue  (modify zone)",
        "Modify values in a zone definition, either the name, conn (connection-info),",
        "or comment.  Connection-info is the DNS host string:port, for example:",
        "zuri.unc.edu:1247",
        "When modifying the conn information, it should be the hostname of the",
        "ICAT-Enabled-Server (IES); see 'h mkzone' for more.",
        " ",
        "The name of the local zone can be changed via some special processing and",
        "since it also requires some manual changes, iadmin will explain those and",
        "prompt for confirmation in this case.",
        ""
    };
    char *modzonecollaclMsgs[] = {
        " modzonecollacl null|read userOrGroup /remotezone (set strict-mode root ACLs)",
        "Modify a remote zone's local collection for strict-mode access.",
        " ",
        "This is only needed if you are running with strict access control",
        "enabled (see acAclPolicy in core.re) and you want users to be able to",
        "see (via 'ils /' or other queries) the existing remote zones in the",
        "root ('/') collection.",
        " ",
        "The problem only occurs at the '/' level because for zones there are",
        "both local and remote collections for the zone. As with any query in",
        "strict mode, when the user asks for information on a collection, the",
        "ICAT-generated SQL adds checks to restrict results to data-objects or",
        "sub-collections in that collection to which the user has read or",
        "better access. The problem is that collections for the remote zones",
        "(/zone) do not have ACLs set, even if ichmod is run try to give it",
        "(e.g. read access to public) because ichmod (like ils, iget, iput,",
        "etc) communicates to the appropriate zone based on the beginning part",
        "of the collection name.",
        " ",
        "The following iquest command returns the local ACLs (tempZone is the",
        "local zone and r3 is a remote zone):",
        "  iquest -z tempZone \"select COLL_ACCESS_TYPE where COLL_NAME = '/r3'\" ",
        "The '-z tempZone' is needed to have it connect locally instead of to the",
        "remote r3 zone.  Normally there will be one row returned for the",
        "owner.  With this command, others can be added.  Note that 'ils -A /r3'",
        "will also check with the remote zone, so use the above iquest",
        "command to see the local information.",
        " ",
        "The command syntax is similar to ichmod:",
        "  null|read userOrGroup /remoteZone",
        "Use null to remove ACLs and read access for another user or group.",
        " ",
        "For example, to allow all users to see the remote zones via 'ils /':",
        "iadmin modzoneacl read public /r3",
        " ",
        "To remove it:",
        "iadmin modzoneacl null public /r3",
        " ",
        "Access below this level is controlled at the remote zone.",
        ""
    };


    char *rmzoneMsgs[] = {
        " rmzone Name (remove zone)",
        "Remove a zone definition.",
        "Only remote zones can be removed.",
        ""
    };

    char *mkgroupMsgs[] = {
        " mkgroup Name (make group)",
        "Create a user group.",
        "Also see atg, rfg, and rmgroup.",
        ""
    };

    char *rmgroupMsgs[] = {
        " rmgroup Name (remove group)",
        "Remove a user group.",
        "Also see mkgroup, atg, and rfg.",
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

    char *atMsgs[] = {
        " at tokenNamespace Name [Value1] [Value2] [Value3] [comment] (add token) ",
        "Add a new token.  The most common use of this is to add",
        "data_type or user_type tokens.  See lt to display currently defined tokens.",
        ""
    };

    char *rtMsgs[] = {
        " rt tokenNamespace Name [Value] (remove token) ",
        "Remove a token.  The most common use of this is to remove",
        "data_type or user_type tokens.  See lt to display currently defined tokens.",
        ""
    };

    char *ctimeMsgs[] = {
        " ctime Time (convert a iRODSTime value (integer) to local time",
        "Time values (modify times, access times) are stored in the database",
        "as a Unix Time value.  This is the number of seconds since 1970 and",
        "is the same in all time zones (basically, Coordinated Universal Time).",
        "ils and other utilities will convert it before displaying it, but iadmin",
        "displays the actual value in the database.  You can enter the value to",
        "the ctime command to convert it to your local time.  The following two",
        "additional forms can also be used:",
        " ",
        " ctime now      - convert a current time to an iRODS time integer value.",
        " ",
        " ctime str Timestr  - convert a string time string (YYYY-MM-DD.hh:mm:ss)",
        " to an iRODS integer value time.",
        " ",
        ""
    };

    char *suqMsgs[] = {
        " suq User ResourceName-or-'total' Value (set user quota)",
        "Set a quota for a particular user for either a resource or all iRODS",
        "usage (total).  Use 0 for the value to remove quota limit.  Value is",
        "in bytes.  As with other sub-commands, 'user' is of the form",
        "userName[#zone] where the local zone is default.",
        "Also see sgq, lq and cu.",
        ""
    };

    char *sgqMsgs[] = {
        " sgq Group ResourceName-or-'total' Value (set group quota)",
        "Set a quota for a user-group for either a resource or all iRODS",
        "usage (total).  Use 0 for the value to remove quota limit.  Value is",
        "in bytes.",
        "Also see suq, lq, and cu.",
        ""
    };

    char *lqMsgs[] = {
        " lq [Name] List Quotas",
        "List the quotas that have been set (if any).",
        "If Name is provided, list only that user or group.",
        "Also see suq, sgq, cu, and the 'iquota' command.",
        ""
    };

    char *cuMsgs[] = {
        " cu (calulate usage (for quotas))",
        "Calculate (via DBMS SQL) the usage on resources for each user and",
        "determine if users are over quota.",
        "Also see suq, sgq, and lq.",
        ""
    };

    char *rumMsgs[] = {
        " rum (remove unused metadata (user-defined AVUs)",
        "When users remove user-defined metadata (Attribute-Value-Unit triplets",
        "(AVUs)) on objects (collections, data-objects, etc), or remove the",
        "objects themselves, the associations between those objects and the",
        "AVUs are removed but the actual AVUs (rows in another table) are left",
        "in place.  This is because the SQL processing can be slow for this",
        "because each AVU can be associated with multiple objects.  But this",
        "only needs to be run once in a while, after any number of such",
        "deletions have been done and only if the number of unused AVUs has",
        "gotten large and so is slowing down the DBMS.  This command runs SQL",
        "to remove those unused AVU rows.  For PostgreSQL and Oracle this will",
        "usually only take a few seconds.  For MySQL it is much slower.",
        " ",
        "You can start a periodic rule/microservice to do this automatically,",
        "by running 'irule clients/icommands/bin/delUnusedAVUs.ir'.",
        "A good practice would be to schedule this to run once a night.",
        "See the contents of delUnusedAVUs.ir for more information.",
        ""
    };

    char *asqMsgs[] = {
        " asq 'SQL query' [Alias] (add specific query)",
        "Add a specific query to the list of those allowed.",
        "Care must be taken when defining these to prevent users from accessing",
        "or updating information (in the ICAT tables) that needs to be restricted",
        "(passwords, for example) as the normal general-query access controls are",
        "bypassed via this.  This also requires an understanding of the ICAT schema",
        "(see icatSysTables.sql) to properly link tables in your SQL.",
        "If an Alias is provided, clients can use that instead of the full SQL",
        "string to select the SQL.  Aliases are checked to be sure they are unique",
        "but the same SQL can have multiple aliases.",
        "These can be executed via 'iquest --sql'.",
        "Use 'iquest --sql ls' to see the currently defined list.",
        "If 'iquest --sql ls' fails see icatSysInserts.sql for the definitions of two",
        "built-in specific queries (r_specific_query) that are needed.",
        "To add a specific query with single quotes(') within, use double",
        "quotes(\") around the SQL.",
        "Also see rsq.",
        ""
    };

    char *rsqMsgs[] = {
        " rsq 'SQL query' or Alias (remove specific query)",
        "Remove a specific query from the list of those allowed.",
        "Use 'iquest --sql ls' to see the currently defined list.",
        "Also see asq.",
        ""
    };


    char *helpMsgs[] = {
        " help (or h) [command] (general help, or more details on a command)",
        " If you specify a command, a brief description of that command",
        " will be displayed.",
        ""
    };

    /*
      char *noMsgs[]={
      "Help has not been written yet",
      ""};
    */

    char *subCmds[] = {"lu", "lua", "luan", "luz", "lt", "lr",
                       "ls", "lz",
                       "lg", "lgd", "lf", "mkuser",
                       "moduser", "aua", "rua", "rpp",
                       "rmuser", "mkdir", "rmdir", "mkresc",
                       "modresc", "modrescdatapaths", "rmresc",
                       "addchildtoresc", "rmchildfromresc",
                       "mkzone", "modzone", "modzonecollacl", "rmzone",
                       "mkgroup", "rmgroup", "atg",
                       "rfg", "at", "rt", "spass", "dspass",
                       "pv", "ctime",
                       "suq", "sgq", "lq", "cu",
                       "rum", "asq", "rsq",
                       "help", "h",
                       ""
                      };

    char **pMsgs[] = { luMsgs, luaMsgs, luanMsgs, luzMsgs, ltMsgs, lrMsgs,
                       lsMsgs, lzMsgs,
                       lgMsgs, lgdMsgs, lfMsgs, mkuserMsgs,
                       moduserMsgs, auaMsgs, ruaMsgs, rppMsgs,
                       rmuserMsgs, mkdirMsgs, rmdirMsgs, mkrescMsgs,
                       modrescMsgs, modrescDataPathsMsgs, rmrescMsgs,
                       addchildtorescMsgs, rmchildfromrescMsgs,
                       mkzoneMsgs, modzoneMsgs, modzonecollaclMsgs, rmzoneMsgs,
                       mkgroupMsgs, rmgroupMsgs, atgMsgs,
                       rfgMsgs, atMsgs, rtMsgs, spassMsgs,
                       dspassMsgs, ctimeMsgs,
                       suqMsgs, sgqMsgs, lqMsgs, cuMsgs,
                       rumMsgs, asqMsgs, rsqMsgs,
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
