#include <irods/authentication_plugin_framework.hpp>
#include <irods/irods_auth_constants.hpp>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_environment_properties.hpp>
#include <irods/irods_gsi_object.hpp>
#include <irods/irods_kvp_string_parser.hpp>
#include <irods/irods_kvp_string_parser.hpp>
#include <irods/irods_native_auth_object.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/irods_pam_auth_object.hpp>
#include <irods/parseCommandLine.h>
#include <irods/rcConnect.h>
#include <irods/rcMisc.h>
#include <irods/rods.h>
#include <irods/rodsClient.h>

#include <boost/lexical_cast.hpp>

#include <nlohmann/json.hpp>

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
const char *AUTH_OPENID_SCHEME = "openid";

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
    return 0; /* no error messages as it normally fails */
}

void
printUpdateMsg() {
    printf( "One or more fields in your iRODS environment file (irods_environment.json) are\n" );
    printf( "missing; please enter them.\n" );
}

int main( int argc, char **argv )
{
    signal( SIGPIPE, SIG_IGN );

    int i = 0, ix = 0, status = 0;
    int echoFlag = 0;
    rodsEnv my_env;
    rcComm_t *Conn = 0;
    rErrMsg_t errMsg;
    rodsArguments_t myRodsArgs;
    int doPassword = 0;
    bool doingEnvFileUpdate = false;

    status = parseCmdLineOpt( argc, argv, "ehvVlZ", 1, &myRodsArgs );
    if ( status != 0 ) {
        printf( "Use -h for help.\n" );
        return 1;
    }

    if ( myRodsArgs.echo == True ) {
        echoFlag = 1;
    }

    if ( myRodsArgs.help == True && myRodsArgs.ttl == True ) {
        usageTTL();
        return 0;
    }

    if ( myRodsArgs.help == True ) {
        usage( argv[0] );
        return 0;
    }

    if ( myRodsArgs.longOption == True ) {
        printRodsEnv( stdout );
    }

    status = getRodsEnv( &my_env );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        return 1;
    }

    int ttl = 0;
    if ( myRodsArgs.ttl == True ) {
        ttl = myRodsArgs.ttlValue;
        if ( ttl < 1 ) {
            printf( "Time To Live value needs to be a positive integer\n" );
            return 1;
        }
    }

    ix = myRodsArgs.optind;

    const char *password = "";
    if ( ix < argc ) {
        password = argv[ix];
    }

    if ( myRodsArgs.longOption == True ) {
        /* just list the env */
        return 0;
    }

    // Create ~/.irods/ if it does not exist
    mkrodsdir();

    using json = nlohmann::json;

    auto json_env = json::object();

    /*
       Check on the key Environment values, prompt and save
       them if not already available.
     */
    if ( strlen( my_env.rodsHost ) == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        printf( "Enter the host name (DNS) of the server to connect to: " );
        std::string response;
        getline( std::cin, response );
        snprintf( my_env.rodsHost, NAME_LEN, "%s", response.c_str() );
        json_env["irods_host"] = my_env.rodsHost;
    }
    if ( my_env.rodsPort == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        printf( "Enter the port number: " );
        std::string response;
        getline( std::cin, response );
        try {
            my_env.rodsPort = boost::lexical_cast< int >( response );
        }
        catch ( const boost::bad_lexical_cast& ) {
            my_env.rodsPort = 0;
        }

        json_env["irods_port"] = my_env.rodsPort;
    }
    if ( strlen( my_env.rodsUserName ) == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        printf( "Enter your irods user name: " );
        std::string response;
        getline( std::cin, response );
        snprintf( my_env.rodsUserName, NAME_LEN, "%s", response.c_str() );
        json_env["irods_user_name"] = my_env.rodsUserName;
    }
    if ( strlen( my_env.rodsZone ) == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        printf( "Enter your irods zone: " );
        std::string response;
        getline( std::cin, response );
        snprintf( my_env.rodsZone, NAME_LEN, "%s", response.c_str() );
        json_env["irods_zone_name"] = my_env.rodsZone;
    }
    if ( strlen( my_env.rodsAuthScheme ) == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        printf( "Enter your irods authentication scheme: " );
        std::string response;
        getline( std::cin, response );
        snprintf( my_env.rodsAuthScheme, NAME_LEN, "%s", response.c_str() );
        json_env[irods::KW_CFG_IRODS_AUTHENTICATION_SCHEME] = my_env.rodsAuthScheme;
    }

    if ( doingEnvFileUpdate ) {
        printf( "Those values will be added to your environment file (for use by\n" );
        printf( "other iCommands) if the login succeeds.\n\n" );
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

    int useGsi = 0;
    if ( AUTH_OPENID_SCHEME == lower_scheme ) {
        doPassword = 0;
    }
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
            return 1;
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
        return 2;
    }

    // =-=-=-=-=-=-=-
    // PAM auth gets special consideration, and also includes an
    // auth by the usual convention
    bool pam_flg = false;
    const auto use_legacy_authentication = irods::experimental::auth::use_legacy_authentication(*Conn);
    if (use_legacy_authentication && irods::AUTH_PAM_SCHEME == lower_scheme) {
        // =-=-=-=-=-=-=-
        // set a flag stating that we have done pam and the auth
        // scheme needs overridden
        pam_flg = true;

        // =-=-=-=-=-=-=-
        // build a context string which includes the ttl and password
        std::stringstream ttl_str;  ttl_str << ttl;
		irods::kvp_map_t ctx_map;
		ctx_map[ irods::AUTH_TTL_KEY ] = ttl_str.str();
		ctx_map[ irods::AUTH_PASSWORD_KEY ] = password;
		std::string ctx_str = irods::escaped_kvp_string(
		                          ctx_map);
        // =-=-=-=-=-=-=-
        // pass the context with the ttl as well as an override which
        // demands the pam authentication plugin
        status = clientLogin( Conn, ctx_str.c_str(), irods::AUTH_PAM_SCHEME.c_str() );
        if ( status != 0 ) {
            return 8;
        }

        // =-=-=-=-=-=-=-
        // if this succeeded, do the regular login below to check
        // that the generated password works properly.
    } // if pam

    if ( strcmp( my_env.rodsAuthScheme, AUTH_OPENID_SCHEME ) == 0 ) {
        irods::kvp_map_t ctx_map;
        try {
            std::string client_provider_cfg = irods::get_environment_property<std::string&>( "openid_provider" );
            ctx_map["provider"] = client_provider_cfg;
        }
        catch ( const irods::exception& e ) {
            if ( e.code() == KEY_NOT_FOUND ) {
                rodsLog( LOG_NOTICE, "KEY_NOT_FOUND: openid_provider not defined" );
            }
            else {
                rodsLog( LOG_DEBUG, "unknown error" );
                irods::log( e );
            }
        }
        ctx_map["nobuildctx"] = "1";
        ctx_map["reprompt"] = "1";
        if ( password && *password ) {
            // for openid, if password not empty send as initialization value
            ctx_map["iinit_arg"] = password;
        }

        std::string ctx_str = irods::escaped_kvp_string( ctx_map );
        status = clientLogin( Conn, ctx_str.c_str(), AUTH_OPENID_SCHEME );
        if ( status != 0 ) {
            rcDisconnect( Conn );
            return 7;
        }
    }
    else {
        if (use_legacy_authentication) {
            // =-=-=-=-=-=-=-
            // since we might be using PAM
            // and check that the user/password is OK
            const char* auth_scheme = ( pam_flg ) ?
                                  irods::AUTH_NATIVE_SCHEME.c_str() :
                                  my_env.rodsAuthScheme;
            status = clientLogin( Conn, 0, auth_scheme );
            if ( status != 0 ) {
                rcDisconnect( Conn );
                return 7;
            }

            printErrorStack( Conn->rError );
            if ( ttl > 0 && !pam_flg ) {
                /* if doing non-PAM TTL, now get the
                short-term password (after initial login) */
                status = clientLoginTTL( Conn, ttl );
                if ( status != 0 ) {
                    rcDisconnect( Conn );
                    return 8;
                }
                /* And check that it works */
                status = clientLogin( Conn );
                if ( status != 0 ) {
                    rcDisconnect( Conn );
                    return 7;
                }
            }
        }
        else {
            nlohmann::json ctx;
            if (irods::AUTH_PAM_SCHEME == lower_scheme) {
                ctx = nlohmann::json{
                    {irods::AUTH_TTL_KEY, std::to_string(ttl)},
                    {irods::AUTH_PASSWORD_KEY, password}
                };
            }

            if (const int ec = clientLogin(Conn, ctx.dump().data()); ec != 0) {
                rcDisconnect(Conn);
                return 7;
            }
        }
    }

    rcDisconnect( Conn );

    /* Save updates to irods_environment.json. */
    if ( doingEnvFileUpdate ) {
        std::string env_file, session_file;
        irods::error ret = irods::get_json_environment_file( env_file, session_file );
        if ( ret.ok() ) {
            json obj_to_dump;

            if (std::ifstream in{env_file}; in) {
                try {
                    in >> obj_to_dump;
                }
                catch (const json::parse_error& e) {
                    obj_to_dump = json_env;
                    std::cerr << "Failed to parse environment file: " << e.what() << '\n'
                              << "Falling back to original environment settings.";
                }

                obj_to_dump.merge_patch(json_env);
            }
            else {
                obj_to_dump = json_env;
            }

            std::ofstream f( env_file.c_str(), std::ios::out );
            if ( f.is_open() ) {
                f << obj_to_dump.dump(4) << std::endl;
                f.close();
            }
            else {
                printf( "failed to open environment file [%s]\n", env_file.c_str() );
            }
        }
        else {
            printf( "failed to get environment file - %ji\n", ( intmax_t )ret.code() );
        }
    } // if doingEnvFileUpdate

    return 0;
} // main


void usage( char *prog ) {
    printf( "Creates a file containing your iRODS password in a scrambled form,\n" );
    printf( "to be used automatically by the icommands.\n" );
    printf( "Usage: %s [-ehvVl] [--ttl TimeToLive] [password]\n", prog );
    printf( " -e  echo the password as you enter it (normally there is no echo)\n" );
    printf( " -l  list the iRODS environment variables (only)\n" );
    printf( " -v  verbose\n" );
    printf( " -V  Very verbose\n" );
    printf( "--ttl ttl  set the password Time To Live (specified in hours)\n" );
    printf( "           Run 'iinit -h --ttl' for more\n" );
    printf( " -h  this help\n" );
    printf( "\n" );
    printf( "Note that the password will be in clear-text if provided via the\n" );
    printf( "command line.  Providing the password this way will bypass the\n" );
    printf( "password prompt.\n" );
    printReleaseInfo( "iinit" );
}

void usageTTL() {
    printf( "When using regular iRODS passwords you can use --ttl (Time To Live)\n" );
    printf( "to request a credential (a temporary password) that will be valid\n" );
    printf( "for only the number of hours you specify (up to a limit set by the\n" );
    printf( "administrator).  This is more secure, as this temporary password\n" );
    printf( "(not your permanent one) will be stored in the obfuscated\n" );
    printf( "credential file (.irodsA) for use by the other iCommands.\n" );
    printf( "\n" );
    printf( "When using PAM, iinit always generates a temporary iRODS password\n" );
    printf( "for use by the other iCommands, using a time-limit set by the\n" );
    printf( "administrator (usually a few days).  With the --ttl option, you can\n" );
    printf( "specify how long this derived password will be valid, within the\n" );
    printf( "limits set by the administrator.\n" );
}
