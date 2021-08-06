/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This routine sets up the rodsEnv structure using the contents of the
  irods_environment.json file and possibly some environment variables.
  For each of the irods_environment.json  items, if an environment variable
  with the same name exists, it overrides the possible environment item.
  This is called by the various Rods commands and the agent.

  This routine also fills in irodsHome and irodsCwd if they are not
  otherwise defined, and if values needed to create them are available.

  The '#' character indicates a comment line.  Items may be enclosed in
  quotes, but do not need to be.  One or more spaces, or a '=', will
  precede the item values.

  The items are defined in the rodsEnv struct.

  If an error occurs, a message may logged or displayed but the
  structure is filled with whatever values are available.

  There is also an 'appendRodsEnv' function to add text to
  the env file, either creating it or appending to it.
*/

#include "rods.h"
#include "rodsErrorTable.h"
#include "getRodsEnv.h"
#include "rodsLog.h"
#include "irods_log.hpp"
#include "irods_version.h"
#include "irods_environment_properties.hpp"
#include "irods_configuration_keywords.hpp"

#define BUF_LEN 100
#define LARGE_BUF_LEN MAX_NAME_LEN+20

#define RODS_ENV_FILE "/.irods/irods_environment.json"  /* under the HOME directory */
extern "C" {

    extern int ProcessType;
    extern char *rstrcpy( char *dst, const char *src, int len ); // why do they not just include the header? - harry

    char *findNextTokenAndTerm( char *inPtr );

    int getRodsEnvFromFile( rodsEnv *rodsEnvArg );
    int getRodsEnvFromEnv( rodsEnv *rodsEnvArg );
    int createRodsEnvDefaults( rodsEnv *rodsEnvArg );

    static char authFileName  [ LONG_NAME_LEN ] = "";
    static char configFileName[ LONG_NAME_LEN ] = "";

    char *
    getRodsEnvFileName() {
        return configFileName;
    }

    /* Return the auth filename, if any */
    /* Used by obf routines so that the env struct doesn't have to be passed
       up and down the calling chain */
    char *
    getRodsEnvAuthFileName() {
        return authFileName;
    }

    /* convert either an integer value or a name matching the defines, to
       a value for the Logging Level */
    int
    convertLogLevel( char *inputStr ) {
        int i;
        i = atoi( inputStr );
        if ( i > 0 && i <= LOG_SQL ) {
            return i;
        }
        if ( strcmp( inputStr, "LOG_SQL" ) == 0 ) {
            return LOG_SQL;
        }
        if ( strcmp( inputStr, "LOG_SYS_FATAL" ) == 0 ) {
            return LOG_SYS_FATAL;
        }
        if ( strcmp( inputStr, "LOG_SYS_WARNING" ) == 0 ) {
            return LOG_SYS_WARNING;
        }
        if ( strcmp( inputStr, "LOG_ERROR" ) == 0 ) {
            return LOG_ERROR;
        }
        if ( strcmp( inputStr, "LOG_NOTICE" ) == 0 ) {
            return LOG_NOTICE;
        }
        if ( strcmp( inputStr, "LOG_DEBUG" ) == 0 ) {
            return LOG_DEBUG;
        }
        if ( strcmp( inputStr, "LOG_DEBUG6" ) == 0 ) {
            return LOG_DEBUG6;
        }
        if ( strcmp( inputStr, "LOG_DEBUG7" ) == 0 ) {
            return LOG_DEBUG7;
        }
        if ( strcmp( inputStr, "LOG_DEBUG8" ) == 0 ) {
            return LOG_DEBUG8;
        }
        if ( strcmp( inputStr, "LOG_DEBUG9" ) == 0 ) {
            return LOG_DEBUG9;
        }
        if ( strcmp( inputStr, "LOG_DEBUG10" ) == 0 ) {
            return LOG_DEBUG10;
        }
        return 0;
    }

    int getRodsEnv( rodsEnv *rodsEnvArg ) {
        if ( !rodsEnvArg ) {
            return SYS_INVALID_INPUT_PARAM;
        }
        _getRodsEnv( *rodsEnvArg );

        return 0;
    }

    void _getRodsEnv( rodsEnv &rodsEnvArg ) {
        memset( &rodsEnvArg, 0, sizeof( rodsEnv ) );
        getRodsEnvFromFile( &rodsEnvArg );
        getRodsEnvFromEnv( &rodsEnvArg );
        createRodsEnvDefaults( &rodsEnvArg );
    }

    void _reloadRodsEnv( rodsEnv &rodsEnvArg ) {
        try {
            irods::environment_properties::instance().capture();
        } catch ( const irods::exception& e ) {
            irods::log(e);
            return;
        }

        memset( &rodsEnvArg, 0, sizeof( rodsEnv ) );
        getRodsEnvFromFile( &rodsEnvArg );
        getRodsEnvFromEnv( &rodsEnvArg );
        createRodsEnvDefaults( &rodsEnvArg );
    }

    static
    int capture_string_property(
        const std::string&             _key,
        char*                          _val ) {
        try {
            sprintf(_val, "%s", irods::get_environment_property<const std::string>(_key).c_str());
            return 0;
        } catch ( const irods::exception& e ) {
            if ( e.code() == KEY_NOT_FOUND ) {
                rodsLog( LOG_DEBUG10, "%s is not defined", _key.c_str() );
            } else {
                irods::log(e);
            }
            return e.code();
        }

    } // capture_string_property

    static
    int capture_integer_property(
        const std::string&             _key,
        int&                           _val ) {
        try {
            _val = irods::get_environment_property<const int>(_key);
            return 0;
        } catch ( const irods::exception& e ) {
            if ( e.code() == KEY_NOT_FOUND ) {
                rodsLog( LOG_DEBUG10, "%s is not defined", _key.c_str() );
            } else {
                irods::log(e);
            }
            return e.code();
        }

    } // capture_integer_property

    int getRodsEnvFromFile(
        rodsEnv* _env ) {
        if ( !_env ) {
            return SYS_INVALID_INPUT_PARAM;
        }

        // defaults for advanced settings
        _env->irodsMaxSizeForSingleBuffer       = 32;
        _env->irodsDefaultNumberTransferThreads = 4;
        _env->irodsTransBufferSizeForParaTrans  = 4;
        _env->irodsConnectionPoolRefreshTime    = 300;

        // default auth scheme
        snprintf(
            _env->rodsAuthScheme,
            sizeof( _env->rodsAuthScheme ),
            "native" );

        capture_string_property(
            irods::CFG_IRODS_SESSION_ENVIRONMENT_FILE_KW,
            configFileName );

        capture_string_property(
            irods::CFG_IRODS_USER_NAME_KW,
            _env->rodsUserName );

        capture_string_property(
            irods::CFG_IRODS_HOST_KW,
            _env->rodsHost );

        capture_string_property(
            irods::CFG_IRODS_XMSG_HOST_KW,
            _env->xmsgHost );

        capture_string_property(
            irods::CFG_IRODS_HOME_KW,
            _env->rodsHome );

        capture_string_property(
            irods::CFG_IRODS_CWD_KW,
            _env->rodsCwd );

        capture_string_property(
            irods::CFG_IRODS_AUTHENTICATION_SCHEME_KW,
            _env->rodsAuthScheme );

        capture_integer_property(
            irods::CFG_IRODS_PORT_KW,
            _env->rodsPort );

        capture_integer_property(
            irods::CFG_IRODS_XMSG_PORT_KW,
            _env->xmsgPort );

        capture_string_property(
            irods::CFG_IRODS_DEFAULT_RESOURCE_KW,
            _env->rodsDefResource );

        capture_string_property(
            irods::CFG_IRODS_ZONE_KW,
            _env->rodsZone );

        capture_string_property(
            irods::CFG_IRODS_CLIENT_SERVER_POLICY_KW,
            _env->rodsClientServerPolicy );

        capture_string_property(
            irods::CFG_IRODS_CLIENT_SERVER_NEGOTIATION_KW,
            _env->rodsClientServerNegotiation );

        capture_integer_property(
            irods::CFG_IRODS_ENCRYPTION_KEY_SIZE_KW,
            _env->rodsEncryptionKeySize );

        capture_integer_property(
            irods::CFG_IRODS_ENCRYPTION_SALT_SIZE_KW,
            _env->rodsEncryptionSaltSize );

        capture_integer_property(
            irods::CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW,
            _env->rodsEncryptionNumHashRounds );

        capture_string_property(
            irods::CFG_IRODS_ENCRYPTION_ALGORITHM_KW,
            _env->rodsEncryptionAlgorithm );

        capture_string_property(
            irods::CFG_IRODS_DEFAULT_HASH_SCHEME_KW,
            _env->rodsDefaultHashScheme );

        capture_string_property(
            irods::CFG_IRODS_MATCH_HASH_POLICY_KW,
            _env->rodsMatchHashPolicy );

        capture_string_property(
            irods::CFG_IRODS_DEBUG_KW,
            _env->rodsDebug );

        _env->rodsLogLevel = 0;
        int status = capture_integer_property(
                         irods::CFG_IRODS_LOG_LEVEL_KW,
                         _env->rodsLogLevel );
        if ( status == 0 && _env->rodsLogLevel > 0 ) {
            if( _env->rodsLogLevel < LOG_SYS_FATAL ) {
                _env->rodsLogLevel = LOG_SYS_FATAL;
            }
            rodsLogLevel( _env->rodsLogLevel );
        }

        memset( _env->rodsAuthFile, 0, sizeof( _env->rodsAuthFile ) );
        status = capture_string_property(
                     irods::CFG_IRODS_AUTHENTICATION_FILE_KW,
                     _env->rodsAuthFile );
        if ( status == 0 ) {
            rstrcpy(
                authFileName,
                _env->rodsAuthFile,
                LONG_NAME_LEN - 1 );
        }

        // legacy ssl environment variables
        capture_string_property(
            irods::CFG_IRODS_SSL_CA_CERTIFICATE_PATH,
            _env->irodsSSLCACertificatePath );

        capture_string_property(
            irods::CFG_IRODS_SSL_CA_CERTIFICATE_FILE,
            _env->irodsSSLCACertificateFile );

        capture_string_property(
            irods::CFG_IRODS_SSL_VERIFY_SERVER,
            _env->irodsSSLVerifyServer );

        capture_string_property(
            irods::CFG_IRODS_SSL_CERTIFICATE_CHAIN_FILE,
            _env->irodsSSLCertificateChainFile );

        capture_string_property(
            irods::CFG_IRODS_SSL_CERTIFICATE_KEY_FILE,
            _env->irodsSSLCertificateKeyFile );

        capture_string_property(
            irods::CFG_IRODS_SSL_DH_PARAMS_FILE,
            _env->irodsSSLDHParamsFile );

        // control plane variables
        capture_string_property(
            irods::CFG_IRODS_SERVER_CONTROL_PLANE_KEY,
            _env->irodsCtrlPlaneKey );

        capture_integer_property(
            irods::CFG_IRODS_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW,
            _env->irodsCtrlPlaneEncryptionNumHashRounds );

        capture_string_property(
            irods::CFG_IRODS_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW,
            _env->irodsCtrlPlaneEncryptionAlgorithm );

        capture_integer_property(
            irods::CFG_IRODS_SERVER_CONTROL_PLANE_PORT,
            _env->irodsCtrlPlanePort );

        capture_integer_property(
            irods::CFG_IRODS_MAX_SIZE_FOR_SINGLE_BUFFER,
            _env->irodsMaxSizeForSingleBuffer );

        capture_integer_property(
            irods::CFG_IRODS_DEF_NUMBER_TRANSFER_THREADS,
            _env->irodsDefaultNumberTransferThreads );

        capture_integer_property(
            irods::CFG_IRODS_TRANS_BUFFER_SIZE_FOR_PARA_TRANS,
            _env->irodsTransBufferSizeForParaTrans );

        capture_integer_property(
            irods::CFG_IRODS_CONNECTION_POOL_REFRESH_TIME,
            _env->irodsConnectionPoolRefreshTime );

        capture_string_property(
            irods::CFG_IRODS_PLUGINS_HOME_KW,
            _env->irodsPluginHome );

        return 0;
    }


    static
    void capture_string_env_var(
        const std::string& _key,
        char*              _val ) {
        char* env = getenv(
                        irods::to_env( _key ).c_str() );
        if ( env ) {
            strncpy(
                _val,
                env,
                strlen( env ) + 1 );
            rodsLog(
                LOG_DEBUG,
                "captured env [%s]-[%s]",
                _key.c_str(),
                _val );
        }

    } // capture_string_env_var

    static
    void capture_integer_env_var(
        const std::string& _key,
        int&               _val ) {
        char* env = getenv(
                        irods::to_env( _key ).c_str() );
        if ( env ) {
            _val = atoi( env );
            rodsLog(
                LOG_DEBUG,
                "captured env [%s]-[%d]",
                _key.c_str(),
                _val );
        }

    } // capture_integer_env_var

    int
    get_legacy_ssl_variables(
        rodsEnv* _env ) {
        if ( !_env ) {
            rodsLog(
                LOG_ERROR,
                "get_legacy_ssl_variables - null env pointer" );
            return SYS_INVALID_INPUT_PARAM;

        }

        char* val = 0;

        val = getenv( "irodsSSLCACertificatePath" );
        if ( val ) {
            snprintf(
                _env->irodsSSLCACertificatePath,
                sizeof( _env->irodsSSLCACertificatePath ),
                "%s", val );
        }

        val = getenv( "irodsSSLCACertificateFile" );
        if ( val ) {
            snprintf(
                _env->irodsSSLCACertificateFile,
                sizeof( _env->irodsSSLCACertificateFile ),
                "%s", val );
        }

        val = getenv( "irodsSSLVerifyServer" );
        if ( val ) {
            snprintf(
                _env->irodsSSLVerifyServer,
                sizeof( _env->irodsSSLVerifyServer ),
                "%s", val );
        }

        val = getenv( "irodsSSLCertificateChainFile" );
        if ( val ) {
            snprintf(
                _env->irodsSSLCertificateChainFile,
                sizeof( _env->irodsSSLCertificateChainFile ),
                "%s", val );
        }

        val = getenv( "irodsSSLCertificateKeyFile" );
        if ( val ) {
            snprintf(
                _env->irodsSSLCertificateKeyFile,
                sizeof( _env->irodsSSLCertificateKeyFile ),
                "%s", val );
        }

        val = getenv( "irodsSSLDHParamsFile" );
        if ( val ) {
            snprintf(
                _env->irodsSSLDHParamsFile,
                sizeof( _env->irodsSSLDHParamsFile ),
                "%s", val );
        }

        return 0;

    } // get_legacy_ssl_variables

    int
    getRodsEnvFromEnv(
        rodsEnv* _env ) {
        if ( !_env ) {
            return SYS_INVALID_INPUT_PARAM;
        }

        int status = get_legacy_ssl_variables( _env );
        if ( status < 0 ) {
            return status;

        }

        std::string env_var = irods::CFG_IRODS_USER_NAME_KW;
        capture_string_env_var(
            env_var,
            _env->rodsUserName );

        env_var = irods::CFG_IRODS_HOST_KW;
        capture_string_env_var(
            env_var,
            _env->rodsHost );

        env_var = irods::CFG_IRODS_XMSG_HOST_KW;
        capture_string_env_var(
            env_var,
            _env->xmsgHost );

        env_var = irods::CFG_IRODS_PORT_KW;
        capture_integer_env_var(
            env_var,
            _env->rodsPort );

        env_var = irods::CFG_IRODS_XMSG_PORT_KW;
        capture_integer_env_var(
            env_var,
            _env->xmsgPort );

        env_var = irods::CFG_IRODS_HOME_KW;
        capture_string_env_var(
            env_var,
            _env->rodsHome );

        env_var = irods::CFG_IRODS_CWD_KW;
        capture_string_env_var(
            env_var,
            _env->rodsCwd );

        env_var = irods::CFG_IRODS_AUTHENTICATION_SCHEME_KW;
        capture_string_env_var(
            env_var,
            _env->rodsAuthScheme );

        env_var = irods::CFG_IRODS_DEFAULT_RESOURCE_KW;
        capture_string_env_var(
            env_var,
            _env->rodsDefResource );

        env_var = irods::CFG_IRODS_ZONE_KW;
        capture_string_env_var(
            env_var,
            _env->rodsZone );

        env_var = irods::CFG_IRODS_CLIENT_SERVER_POLICY_KW;
        capture_string_env_var(
            env_var,
            _env->rodsClientServerPolicy );

        env_var = irods::CFG_IRODS_CLIENT_SERVER_NEGOTIATION_KW;
        capture_string_env_var(
            env_var,
            _env->rodsClientServerNegotiation );

        env_var = irods::CFG_IRODS_ENCRYPTION_KEY_SIZE_KW;
        capture_integer_env_var(
            env_var,
            _env->rodsEncryptionKeySize );

        env_var = irods::CFG_IRODS_ENCRYPTION_SALT_SIZE_KW;
        capture_integer_env_var(
            env_var,
            _env->rodsEncryptionSaltSize );

        env_var = irods::CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW;
        capture_integer_env_var(
            env_var,
            _env->rodsEncryptionNumHashRounds );

        env_var = irods::CFG_IRODS_ENCRYPTION_ALGORITHM_KW;
        capture_string_env_var(
            env_var,
            _env->rodsEncryptionAlgorithm );

        env_var = irods::CFG_IRODS_DEFAULT_HASH_SCHEME_KW;
        capture_string_env_var(
            env_var,
            _env->rodsDefaultHashScheme );

        env_var = irods::CFG_IRODS_MATCH_HASH_POLICY_KW;
        capture_string_env_var(
            env_var,
            _env->rodsMatchHashPolicy );

        _env->rodsLogLevel = 0;
        env_var = irods::CFG_IRODS_LOG_LEVEL_KW;
        capture_integer_env_var(
            env_var,
            _env->rodsLogLevel );
        if( _env->rodsLogLevel ) {
            if( _env->rodsLogLevel < LOG_SYS_FATAL ) {
                _env->rodsLogLevel = LOG_SYS_FATAL;
            }

            rodsLogLevel( _env->rodsLogLevel );
        }

        memset( _env->rodsAuthFile, 0, sizeof( _env->rodsAuthFile ) );
        env_var = irods::CFG_IRODS_AUTHENTICATION_FILE_KW;
        capture_string_env_var(
            env_var,
            _env->rodsAuthFile );
        if ( strlen( _env->rodsAuthFile ) > 0 ) {
            rstrcpy( authFileName, _env->rodsAuthFile, LONG_NAME_LEN );

        }

        env_var = irods::CFG_IRODS_DEBUG_KW;
        capture_string_env_var(
            env_var,
            _env->rodsDebug );

        // legacy ssl environment variables
        env_var = irods::CFG_IRODS_SSL_CA_CERTIFICATE_PATH;
        capture_string_env_var(
            env_var,
            _env->irodsSSLCACertificatePath );
        env_var = irods::CFG_IRODS_SSL_CA_CERTIFICATE_FILE;
        capture_string_env_var(
            env_var,
            _env->irodsSSLCACertificateFile );
        env_var = irods::CFG_IRODS_SSL_VERIFY_SERVER;
        capture_string_env_var(
            env_var,
            _env->irodsSSLVerifyServer );
        env_var = irods::CFG_IRODS_SSL_VERIFY_SERVER;
        capture_string_env_var(
            env_var,
            _env->irodsSSLVerifyServer );
        env_var = irods::CFG_IRODS_SSL_CERTIFICATE_CHAIN_FILE;
        capture_string_env_var(
            env_var,
            _env->irodsSSLCertificateChainFile );
        env_var = irods::CFG_IRODS_SSL_CERTIFICATE_KEY_FILE;
        capture_string_env_var(
            env_var,
            _env->irodsSSLCertificateKeyFile );
        env_var = irods::CFG_IRODS_SSL_DH_PARAMS_FILE;
        capture_string_env_var(
            env_var,
            _env->irodsSSLDHParamsFile );

        env_var = irods::CFG_IRODS_MAX_SIZE_FOR_SINGLE_BUFFER;
        capture_integer_env_var(
            env_var,
            _env->irodsMaxSizeForSingleBuffer );

        env_var = irods::CFG_IRODS_DEF_NUMBER_TRANSFER_THREADS;
        capture_integer_env_var(
            env_var,
            _env->irodsDefaultNumberTransferThreads );

        env_var = irods::CFG_IRODS_TRANS_BUFFER_SIZE_FOR_PARA_TRANS;
        capture_integer_env_var(
            env_var,
            _env->irodsTransBufferSizeForParaTrans );

        env_var = irods::CFG_IRODS_PLUGINS_HOME_KW;
        capture_string_env_var(
            env_var,
            _env->irodsPluginHome );

        return 0;
    }

    int printRodsEnv(
        FILE*    _fout ) {
        if( !_fout ) {
            fprintf(
                stderr,
                "printRodsEnv :: null input param(s)\n" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        try {
            const auto& prop_map = irods::environment_properties::instance().map();

            fprintf(
                    _fout,
                    "irods_version - %d.%d.%d\n",
                    IRODS_VERSION_MAJOR,
                    IRODS_VERSION_MINOR,
                    IRODS_VERSION_PATCHLEVEL);

            for( auto itr = prop_map.cbegin(); itr != prop_map.cend(); ++itr ) {

                try {
                    int val = boost::any_cast< int >( itr->second );
                    fprintf(
                            _fout,
                            "%s - %d\n",
                            itr->first.c_str(),
                            val );
                    continue;
                }
                catch ( const boost::bad_any_cast& ) {
                }

                try {
                    const std::string& val = boost::any_cast< const std::string& >( itr->second );
                    fprintf(
                            _fout,
                            "%s - %s\n",
                            itr->first.c_str(),
                            val.c_str() );
                    continue;
                }
                catch ( const boost::bad_any_cast& ) {
                    fprintf(
                            stderr,
                            "failed to cast %s",
                            itr->first.c_str() );
                }

            } // for itr
        } catch ( const irods::exception& e ) {
            fprintf( stderr,
                "%s",
                e.what() );
            return e.code();
        }

        return 0;

    } // printRodsEnv


    /* build a couple default values from others if appropriate */
    int
    createRodsEnvDefaults( rodsEnv *rodsEnvArg ) {
        if ( strlen( rodsEnvArg->rodsHome ) == 0 &&
                strlen( rodsEnvArg->rodsUserName ) > 0 &&
                strlen( rodsEnvArg->rodsZone ) > 0 ) {
            snprintf( rodsEnvArg->rodsHome,  MAX_NAME_LEN, "/%s/home/%s",
                        rodsEnvArg->rodsZone, rodsEnvArg->rodsUserName );
            rodsLog( LOG_NOTICE, "created irodsHome=%s", rodsEnvArg->rodsHome );
        }
        if ( strlen( rodsEnvArg->rodsCwd ) == 0 &&
                strlen( rodsEnvArg->rodsHome ) > 0 ) {
            rstrcpy( rodsEnvArg->rodsCwd, rodsEnvArg->rodsHome, MAX_NAME_LEN );
            rodsLog( LOG_NOTICE, "created irodsCwd=%s", rodsEnvArg->rodsCwd );
        }

        return 0;
    }


    /*
      find the next delimited token and terminate the string with matching quotes
    */
    char *findNextTokenAndTerm( char *inPtr ) {
        char *myPtr = 0;
        char *savePtr = 0;
        char *nextPtr = 0;
        int whiteSpace = 0;
        myPtr = inPtr;
        whiteSpace = 1;
        for ( ;; myPtr++ ) {
            if ( *myPtr == ' ' || *myPtr == '=' ) {
                continue;
            }
            if ( *myPtr == '"' && whiteSpace ) {
                myPtr++;
                savePtr = myPtr;
                for ( ;; ) {
                    if ( *myPtr == '"' ) {
                        nextPtr = myPtr + 1;
                        if ( *nextPtr == ' ' || *nextPtr == '\n'  || *nextPtr == '\0' ) {
                            /* embedded "s are OK */
                            *myPtr = '\0';
                            return savePtr;
                        }
                    }
                    if ( *myPtr == '\n' ) {
                        *myPtr = '\0';
                    }
                    if ( *myPtr == '\0' ) {
                        /* terminated without a corresponding ", so backup and
                           put the starting one back */
                        savePtr--;
                        *savePtr = '"';
                        return savePtr;
                    }
                    myPtr++;
                }
            }
            if ( *myPtr == '\'' && whiteSpace ) {
                myPtr++;
                savePtr = myPtr;
                for ( ;; ) {
                    if ( *myPtr == '\'' ) {
                        nextPtr = myPtr + 1;
                        if ( *nextPtr == ' ' || *nextPtr == '\n'  || *nextPtr == '\0' ) {
                            /* embedded 's are OK */
                            *myPtr = '\0';
                            return savePtr;
                        }
                    }
                    if ( *myPtr == '\n' ) {
                        *myPtr = '\0';
                    }
                    if ( *myPtr == '\0' ) {
                        /* terminated without a corresponding ", so backup and
                           put the starting one back */
                        savePtr--;
                        *savePtr = '\'';
                        return savePtr;
                    }
                    myPtr++;
                }
            }
            if ( whiteSpace ) {
                savePtr = myPtr;
            }
            whiteSpace = 0;
            if ( *myPtr == '\n' ) {
                *myPtr = '\0';
            }
            if ( *myPtr == '\r' ) {
                *myPtr = '\0';
            }
            if ( *myPtr == '\0' ) {
                return savePtr;
            }
        }
    }

} // extern "C"
