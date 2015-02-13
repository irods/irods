/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
// =-=-=-=-=-=-=-
// irods includes
#include "rods.hpp"
#include "parseCommandLine.hpp"
#include "rcMisc.hpp"
#include "rodsClient.hpp"

// =-=-=-=-=-=-=-
#include "irods_native_auth_object.hpp"
#include "irods_pam_auth_object.hpp"
#include "irods_gsi_object.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_auth_constants.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_environment_properties.hpp"

#include "boost/lexical_cast.hpp"

#include <jansson.h>
#include <iostream>
#include <fstream>

void usage( char *prog );
void usageTTL();


/* Uncomment the line below if you want TTL to be required for all
   users; i.e. all credentials will be time-limited.  This is only
   enforced on the client side so users can bypass this restriction by
   building their own iinit but it would strongly encourage the use of
   time-limited credentials. */
/* Uncomment the line below if you also want a default TTL if none
   is specified by the user. This TTL is specified in hours. */

#define TTYBUF_LEN 100
#define UPDATE_TEXT_LEN NAME_LEN*10

/*
 Attempt to make the ~/.irods directory in case it doesn't exist (may
 be needed to write the .irodsA file and perhaps the
 irods_environment.json file).
 */
int
mkrodsdir() {
    char dirName[NAME_LEN];
    int mode;
    char *getVar;
#ifdef windows_platform
    getVar = iRODSNt_gethome();
#else
    getVar = getenv( "HOME" );
#endif
    rstrcpy( dirName, getVar, NAME_LEN );
    rstrcat( dirName, "/.irods", NAME_LEN );
    mode = 0700;
#ifdef _WIN32
    iRODSNt_mkdir( dirName, mode );
#else
    int error_code = mkdir( dirName, mode );
    int errsv = errno;
    if ( error_code != 0 && errsv != EEXIST ) {
        rodsLog( LOG_NOTICE, "mkdir failed in mkrodsdir with error code %d", error_code );
    }
#endif
    return ( 0 ); /* no error messages as it normally fails */
}

void
printUpdateMsg() {
    printf( "One or more fields in your iRODS environment file (irods_environment.json) are\n" );
    printf( "missing; please enter them.\n" );
}
int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int i = 0, ix = 0, status = 0;
    int echoFlag = 0;
    char *password = 0;
    rodsEnv my_env;
    rcComm_t *Conn = 0;
    rErrMsg_t errMsg;
    rodsArguments_t myRodsArgs;
    int useGsi = 0;
    int doPassword = 0;
    int doingEnvFileUpdate = 0;
    int ttl = 0;

    status = parseCmdLineOpt( argc, argv, "ehvVlZ", 1, &myRodsArgs );
    if ( status != 0 ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    if ( myRodsArgs.echo == True ) {
        echoFlag = 1;
    }

    if ( myRodsArgs.help == True && myRodsArgs.ttl == True ) {
        usageTTL();
        exit( 0 );
    }

    if ( myRodsArgs.help == True ) {
        usage( argv[0] );
        exit( 0 );
    }

    if ( myRodsArgs.longOption == True ) {
        rodsLogLevel( LOG_NOTICE );
        setenv( PRINT_RODS_ENV_STR, "1", 0 );
    }

    status = getRodsEnv( &my_env );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    if ( myRodsArgs.ttl == True ) {
        ttl = myRodsArgs.ttlValue;
        if ( ttl < 1 ) {
            printf( "Time To Live value needs to be a positive integer\n" );
            exit( 1 );
        }
    }

    ix = myRodsArgs.optind;

    password = "";
    if ( ix < argc ) {
        password = argv[ix];
    }

    if ( myRodsArgs.longOption == True ) {
        /* just list the env */
        exit( 0 );
    }

    /*
     Create ~/.irods/ if it does not exist
     */
    mkrodsdir();

    json_t* json_env = json_object();
    if ( !json_env ) {
        printf( "call to json_object() failed.\n" );
        return SYS_MALLOC_ERR;
    }

    json_object_set(
        json_env,
        irods::CFG_IRODS_AUTHENTICATION_SCHEME_KW.c_str(),
        json_string( irods::AUTH_NATIVE_SCHEME.c_str() ) );

    /*
       Check on the key Environment values, prompt and save
       them if not already available.
     */
    if ( strlen( my_env.rodsHost ) == 0 ) {
        if ( doingEnvFileUpdate == 0 ) {
            doingEnvFileUpdate = 1;
            printUpdateMsg();
        }
        printf( "Enter the host name (DNS) of the server to connect to:" );
        std::string response;
        getline( std::cin, response );
        snprintf(
            my_env.rodsHost,
            NAME_LEN,
            "%s",
            response.c_str() );
        json_object_set(
            json_env,
            "irods_host",
            json_string( my_env.rodsHost ) );
    }
    if ( my_env.rodsPort == 0 ) {
        if ( doingEnvFileUpdate == 0 ) {
            doingEnvFileUpdate = 1;
            printUpdateMsg();
        }
        printf( "Enter the port number:" );
        std::string response;
        getline( std::cin, response );
        try {
            my_env.rodsPort = boost::lexical_cast< int >( response );

        }
        catch ( const boost::bad_lexical_cast& ) {
            my_env.rodsPort = 0;

        }

        json_object_set(
            json_env,
            "irods_port",
            json_integer( my_env.rodsPort ) );
    }
    if ( strlen( my_env.rodsUserName ) == 0 ) {
        if ( doingEnvFileUpdate == 0 ) {
            doingEnvFileUpdate = 1;
            printUpdateMsg();
        }
        printf( "Enter your irods user name:" );
        std::string response;
        getline( std::cin, response );
        snprintf(
            my_env.rodsUserName,
            NAME_LEN,
            "%s",
            response.c_str() );
        json_object_set(
            json_env,
            "irods_user_name",
            json_string( my_env.rodsUserName ) );
    }
    if ( strlen( my_env.rodsZone ) == 0 ) {
        if ( doingEnvFileUpdate == 0 ) {
            doingEnvFileUpdate = 1;
            printUpdateMsg();
        }
        printf( "Enter your irods zone:" );
        std::string response;
        getline( std::cin, response );
        snprintf(
            my_env.rodsZone,
            NAME_LEN,
            "%s",
            response.c_str() );
        json_object_set(
            json_env,
            "irods_zone",
            json_string( my_env.rodsZone ) );
    }
    if ( doingEnvFileUpdate ) {
        printf( "Those values will be added to your environment file (for use by\n" );
        printf( "other i-commands) if the login succeeds.\n\n" );
    }

    /*
      Now, get the password
     */
    doPassword = 1;
    // =-=-=-=-=-=-=-
    // ensure scheme is lower case for comparison
    std::string lower_scheme = my_env.rodsAuthScheme;
    std::transform(
        lower_scheme.begin(),
        lower_scheme.end(),
        lower_scheme.begin(),
        ::tolower );

    if ( irods::AUTH_GSI_SCHEME == lower_scheme ) {
        useGsi = 1;
        doPassword = 0;
    }

    if ( irods::AUTH_PAM_SCHEME == lower_scheme ) {
        doPassword = 0;
    }

    if ( strcmp( my_env.rodsUserName, ANONYMOUS_USER ) == 0 ) {
        doPassword = 0;
    }
    if ( useGsi == 1 ) {
        printf( "Using GSI, attempting connection/authentication\n" );
    }
    if ( doPassword == 1 ) {
        if ( myRodsArgs.verbose == True ) {
            i = obfSavePw( echoFlag, 1, 1, password );
        }
        else {
            i = obfSavePw( echoFlag, 0, 0, password );
        }

        if ( i != 0 ) {
            rodsLogError( LOG_ERROR, i, "Save Password failure" );
            exit( 1 );
        }
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    /* Connect... */
    Conn = rcConnect( my_env.rodsHost, my_env.rodsPort, my_env.rodsUserName,
                      my_env.rodsZone, 0, &errMsg );
    if ( Conn == NULL ) {
        rodsLog( LOG_ERROR,
                 "Saved password, but failed to connect to server %s",
                 my_env.rodsHost );
        exit( 2 );
    }

    // =-=-=-=-=-=-=-
    // PAM auth gets special consideration, and also includes an
    // auth by the usual convention
    bool pam_flg = false;
    if ( irods::AUTH_PAM_SCHEME == lower_scheme ) {
        // =-=-=-=-=-=-=-
        // set a flag stating that we have done pam and the auth
        // scheme needs overridden
        pam_flg = true;

        // =-=-=-=-=-=-=-
        // build a context string which includes the ttl and password
        std::stringstream ttl_str;
        ttl_str << ttl;
        std::string context = irods::AUTH_TTL_KEY      +
                              irods::kvp_association() +
                              ttl_str.str()            +
                              irods::kvp_delimiter()   +
                              irods::AUTH_PASSWORD_KEY +
                              irods::kvp_association() +
                              password;
        // =-=-=-=-=-=-=-
        // pass the context with the ttl as well as an override which
        // demands the pam authentication plugin
        status = clientLogin( Conn, context.c_str(), irods::AUTH_PAM_SCHEME.c_str() );
        if ( status != 0 ) {
            exit( 8 );
        }

        // =-=-=-=-=-=-=-
        // if this succeeded, do the regular login below to check
        // that the generated password works properly.
    } // if pam

    // =-=-=-=-=-=-=-
    // since we might be using PAM
    // and check that the user/password is OK
    const char* auth_scheme = ( pam_flg ) ?
                              irods::AUTH_NATIVE_SCHEME.c_str() :
                              my_env.rodsAuthScheme;
    status = clientLogin( Conn, 0, auth_scheme );
    if ( status != 0 ) {


        rcDisconnect( Conn );
        exit( 7 );
    }

    printErrorStack( Conn->rError );
    if ( ttl > 0 && !pam_flg ) {
        /* if doing non-PAM TTL, now get the
        short-term password (after initial login) */
        status = clientLoginTTL( Conn, ttl );
        if ( status != 0 ) {


            rcDisconnect( Conn );
            exit( 8 );
        }
        /* And check that it works */
        status = clientLogin( Conn );
        if ( status != 0 ) {


            rcDisconnect( Conn );
            exit( 7 );
        }
    }

    rcDisconnect( Conn );

    /* Save updates to irods_environment.json. */
    if ( doingEnvFileUpdate == 1 ) {
        std::string env_file, session_file;
        irods::error ret = irods::environment_properties::get_json_environment_file(
                               env_file,
                               session_file );
        if ( ret.ok() ) {
            char* tmp_buf = json_dumps(
                                json_env,
                                JSON_INDENT( 4 ) );
            std::ofstream f(
                env_file.c_str(),
                std::ios::out );
            if ( f.is_open() ) {
                f << tmp_buf << std::endl;
                f.close();

            }
            else {
                printf(
                    "failed to open environment file [%s]\n",
                    env_file.c_str() );

            }

        }
        else {
            printf(
                "failed to get environment file - %Ld\n",
                ret.code() );

        }

    } // if doingEnvFileUpdate

    return 0;

} // main


void usage( char *prog ) {
    printf( "Creates a file containing your iRODS password in a scrambled form,\n" );
    printf( "to be used automatically by the icommands.\n" );
    printf( "Usage: %s [-ehvVl] [--ttl TimeToLive]\n", prog );
    printf( " -e  echo the password as you enter it (normally there is no echo)\n" );
    printf( " -l  list the iRODS environment variables (only)\n" );
    printf( " -v  verbose\n" );
    printf( " -V  Very verbose\n" );
    printf( "--ttl ttl  set the password Time To Live (specified in hours)\n" );
    printf( "           Run 'iinit -h --ttl' for more\n" );

    printf( " -h  this help\n" );
    printReleaseInfo( "iinit" );
}

void usageTTL() {
    printf( "When using regular iRODS passwords you can use --ttl (Time To Live)\n" );
    printf( "to request a credential (a temporary password) that will be valid\n" );
    printf( "for only the number of hours you specify (up to a limit set by the\n" );
    printf( "administrator).  This is more secure, as this temporary password\n" );
    printf( "(not your permanent one) will be stored in the obfuscated\n" );
    printf( "credential file (.irodsA) for use by the other i-commands.\n" );
    printf( "\n" );
    printf( "When using PAM, iinit always generates a temporary iRODS password\n" );
    printf( "for use by the other i-commands, using a time-limit set by the\n" );
    printf( "administrator (usually a few days).  With the --ttl option, you can\n" );
    printf( "specify how long this derived password will be valid, within the\n" );
    printf( "limits set by the administrator.\n" );
}

