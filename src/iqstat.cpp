#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/irods_query.hpp>
#include <irods/rcMisc.h>
#include <irods/rods.h>
#include <irods/rodsClient.h>
#include <irods/rodsLog.h>
#include <irods/user_administration.hpp>

#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#define MAX_SQL 300
#define BIG_STR 200

char cwd[BIG_STR];

int debug = 0;

rcComm_t *Conn{};
rodsEnv myEnv;

void usage();

auto throw_if_id_cannot_be_converted_to_int(const std::string& key) -> void;

/*
 print the results of a general query.
 */
void
printGenQueryResults( rcComm_t *Conn, int status, genQueryOut_t *genQueryOut,
                      char *descriptions[], int formatFlag ) {
    int i, j;
    char localTime[TIME_LEN];
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
                }
                if ( formatFlag == 1 ) {
                    printf( "\n" );
                }
            }
        }
    }
}

/*
Via a general query, show rule information
*/

namespace
{
    /* This structure encapsulates a number of parameters having generally to do with
       the output choices of "brief" or "long" formats when listing the information
       for each rule ID; in a nutshell, which columns are listed, and how many.
       The info given in a "brief" is a subset (of the first two) of the "long" listing.
     */
    struct output_format_parameters {
        int columns[20]{};
        int brief_format_len{};
        int long_format_len{};

        constexpr output_format_parameters() {
            int i = 0;
            columns[i++] = COL_RULE_EXEC_ID;
            columns[i++] = COL_RULE_EXEC_NAME;

            brief_format_len = i;

            columns[i++] = COL_RULE_EXEC_REI_FILE_PATH;
            columns[i++] = COL_RULE_EXEC_USER_NAME;
            columns[i++] = COL_RULE_EXEC_ADDRESS;
            columns[i++] = COL_RULE_EXEC_TIME;
            columns[i++] = COL_RULE_EXEC_FREQUENCY;
            columns[i++] = COL_RULE_EXEC_PRIORITY;
            columns[i++] = COL_RULE_EXEC_ESTIMATED_EXE_TIME;
            columns[i++] = COL_RULE_EXEC_NOTIFICATION_ADDR;
            columns[i++] = COL_RULE_EXEC_LAST_EXE_TIME;
            columns[i++] = COL_RULE_EXEC_STATUS;

            long_format_len = i;
        }
    };
}

auto show_RuleExec( char *userName,
                    const char *ruleName="",
                    int allFlag = false,
                    bool brief = false ) -> int;

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    rErrMsg_t errMsg;

    rodsArguments_t myRodsArgs;

    char userName[NAME_LEN];

    rodsLogLevel( LOG_ERROR );

    int status = parseCmdLineOpt( argc, argv, "alu:vVh", 0, &myRodsArgs );
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

    if ( myRodsArgs.user && myRodsArgs.all ) {
        std::fprintf(stderr, "Incompatible options: -u and -a cannot be used together\n");
        return 2;
    }

    if ( myRodsArgs.user ) {
        snprintf( userName, sizeof( userName ), "%s", myRodsArgs.userString );
    }
    else {
        snprintf( userName, sizeof( userName ), "%s", myEnv.rodsUserName );
    }

    const auto disconnect = irods::at_scope_exit{[] { rcDisconnect(Conn); }};

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    status = clientLogin( Conn );
    if ( status != 0 ) {
        if ( !debug ) {
            return 3;
        }
    }

    try{
        int nArgs { argc - myRodsArgs.optind };
        if (nArgs > 0) {
            throw_if_id_cannot_be_converted_to_int(argv[myRodsArgs.optind]);
        }

        status = show_RuleExec( userName,
                               nArgs > 0 ? argv[myRodsArgs.optind] : "",
                               myRodsArgs.all,
                               myRodsArgs.longOption == 0 && nArgs == 0 );
    }
    catch (const irods::exception& e) {
        fprintf(stderr, "Error: %s\n", e.client_display_what());
        status = 1;
    }

    if (Conn) {
        printErrorStack( Conn->rError );
    }

    return status;
}

/*
Print the main usage/help information.
 */
void usage() {
    char *msgs[] = {
        "Usage:     iqstat [-luvVh] [-u USER] [RULE_ID]",
        " ",
        "Show information about pending iRODS rule executions (i.e. delayed rules).",
        "By default, only the invoking user's rules are shown.",
        " ",
        "Options:",
        " -a        Display requests of all users.",
        " -l        Force use of the long format.",
        " -u USER   Display requests for the specified user (may not be used together with -a).",
        " RULE_ID   Limits the query to the specified integer rule ID (and implies long format).",
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

auto show_RuleExec( char *userName,
                    const char *ruleName,
                    int allFlag,
                    bool brief ) -> int
{
    genQueryInp_t genQueryInp{};
    genQueryOut_t *genQueryOut{};

    constexpr output_format_parameters output_format;

    auto num_cols_selected = (brief ? output_format.brief_format_len
                                    : output_format.long_format_len);

    for (int i = 0; i < num_cols_selected; i++) {
        addInxIval(&genQueryInp.selectInp, output_format.columns[i], 0);
    }


    if (!allFlag) {
        char v1[BIG_STR];
        snprintf( v1, BIG_STR, "='%s'", userName );
        addInxVal(&genQueryInp.sqlCondInp, COL_RULE_EXEC_USER_NAME, v1);
    }

    std::string diagnostic;
    if (ruleName != NULL && *ruleName != '\0') {
        char v2[BIG_STR];
        diagnostic = (boost::format( " (and matching key '%s')" ) % ruleName).str();
        snprintf( v2, BIG_STR, "='%s'", ruleName );
        addInxVal(&genQueryInp.sqlCondInp, COL_RULE_EXEC_ID, v2);
    }

    genQueryInp.maxRows = 50;

    // idempotent version of clearGenQueryOut.
    const auto clear_gq_out = [&o = genQueryOut] {
        clearGenQueryOut(o);
        o->attriCnt = 0;
    };

    // TODO's : should we set attriCnt = 0 as the last thing in the clearGenQueryOut() routine?
    // And...investigate 1. if this ^^^ is the best way or via init'ing via memset(ptr,0,sizeof(...))
    //                   2. reimplementation of iqstat using the C++ query iterator
    //                   3. possibility of memory leaks on rcGenQuery/rsGenQuery side

    // RAII for both GenQuery variables.
    const auto cleanup_gq_in_out = [&, &i = genQueryInp, &o = genQueryOut] {
        clearGenQueryInp(&i);
        clear_gq_out();
        free(o); o = nullptr;
    };

    // Deploy the RAII.
    const auto cleanup_gq = irods::at_scope_exit{[&] { cleanup_gq_in_out(); }};

    int status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    if ( status == CAT_NO_ROWS_FOUND ) {
        // Need to determine which table "no rows" refers to: USER or RULE.
        namespace ia = irods::experimental::administration;
        if (ia::client::exists(*Conn, ia::user{userName})) {
            if ( allFlag ) {
                printf( "No delayed rules pending%s\n", diagnostic.c_str() );
            }
            else {
                printf( "No delayed rules pending for user %s%s\n", userName, diagnostic.c_str());
            }
            return 0;
        }
        else {
            printf( "User %s does not exist.\n", userName );
            return 0;
        }
    }
    else if (status < 0) {
        char* errno_message{};
        const char* main_message = rodsErrorName(status, &errno_message);
        std::cerr << "rcGenQuery failed with error "
                  << status << " "
                  << main_message << " "
                  << errno_message << std::endl;
        std::free(errno_message);
        return 1;
    }

    if (brief) {
        printf( "id     name\n" );
    }

    static char *columnNames[] = {"id", "name", "rei_file_path", "user_name",
                                  "address", "time", "frequency", "priority",
                                  "estimated_exe_time", "notification_addr",
                                  "last_exe_time", "exec_status"
                                 };

    auto increment_count_and_print_results = [&] {
        printGenQueryResults( Conn, status, genQueryOut,
                              brief ? NULL: columnNames,
                              int{brief} );
    };

    increment_count_and_print_results();

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        clear_gq_out();
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if (!brief && genQueryOut->rowCnt > 0 ) {
            printf( "----\n" );
        }
        increment_count_and_print_results();
    }
    return 0;
}

auto throw_if_id_cannot_be_converted_to_int(const std::string& key) -> void
{
    std::string k{key};
    boost::algorithm::trim(k);
    try {
        std::stoul(k);
    }
    catch(const std::invalid_argument&){
        THROW(SYS_INVALID_INPUT_PARAM, "Delay rule ID has incorrect format.");
    }
    catch(const std::out_of_range&){
        THROW(SYS_INVALID_INPUT_PARAM, "Delay rule ID is too large.");
    }
    catch(const std::exception& e){
        auto message = std::string{ "Error parsing delay rule ID: "} + e.what();
        THROW(SYS_INVALID_INPUT_PARAM, message);
    }
    catch(...){
        THROW(SYS_INVALID_INPUT_PARAM, "Unknown error parsing delay rule ID.");
    }
}
