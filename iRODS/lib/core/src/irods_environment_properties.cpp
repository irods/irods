/*
 * irods_environment_properties.cpp
 *
 */

#include "irods_environment_properties.hpp"
#include "irods_get_full_path_for_config_file.hpp"

#include "rods.hpp"
#include "irods_log.hpp"
#include "irods_lookup_table.hpp"
#include "irods_home_directory.hpp"
#include "readServerConfig.hpp"

#include <string>
#include <algorithm>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;



#define BUF_LEN 500


namespace irods {

    const std::string environment_properties::LEGACY_ENV_FILE = "/.irods/.irodsEnv";
    const std::string environment_properties::JSON_ENV_FILE   = "/.irods/irods_environment.json";

// Access method for singleton
    environment_properties& environment_properties::getInstance() {
        static environment_properties instance;
        return instance;
    }


    error environment_properties::capture_if_needed() {
        error result = SUCCESS();
        if ( !captured_ ) {
            result = capture();
        }
        return result;
    }

    environment_properties::environment_properties() :
        captured_( false ) {

        key_map_[ CFG_IRODS_USER_NAME_KW ]                  = "irodsUserName";
        key_map_[ CFG_IRODS_HOST_KW ]                       = "irodsHost";
        key_map_[ CFG_IRODS_PORT_KW ]                       = "irodsPort";
        key_map_[ CFG_IRODS_XMSG_HOST_KW ]                  = "xmsgHost";
        key_map_[ CFG_IRODS_XMSG_PORT_KW ]                  = "xmsgPort";
        key_map_[ CFG_IRODS_HOME_KW ]                       = "irodsHome";
        key_map_[ CFG_IRODS_CWD_KW ]                        = "irodsCwd";
        key_map_[ CFG_IRODS_AUTHENTICATION_SCHEME_KW ]      = "irodsAuthScheme";
        key_map_[ CFG_IRODS_DEFAULT_RESOURCE_KW ]           = "irodsDefResource";
        key_map_[ CFG_IRODS_ZONE_KW ]                       = "irodsZone";
        key_map_[ CFG_IRODS_GSI_SERVER_DN_KW ]              = "irodsServerDn";
        key_map_[ CFG_IRODS_LOG_LEVEL_KW ]                  = "irodsLogLevel";
        key_map_[ CFG_IRODS_AUTHENTICATION_FILE_NAME_KW ]   = "irodsAuthFileName";
        key_map_[ CFG_IRODS_DEBUG_KW ]                      = "irodsDebug";
        key_map_[ CFG_IRODS_CLIENT_SERVER_POLICY_KW ]       = "irodsClientServerPolicy";
        key_map_[ CFG_IRODS_CLIENT_SERVER_NEGOTIATION_KW ]  = "irodsClientServerNegotiation";
        key_map_[ CFG_IRODS_ENCRYPTION_KEY_SIZE_KW ]        = "irodsEncryptionKeySize";
        key_map_[ CFG_IRODS_ENCRYPTION_SALT_SIZE_KW ]       = "irodsEncryptionSaltSize";
        key_map_[ CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW ] = "irodsEncryptionNumHashRounds";
        key_map_[ CFG_IRODS_ENCRYPTION_ALGORITHM_KW ]       = "irodsEncryptionAlgorithm";
        key_map_[ CFG_IRODS_DEFAULT_HASH_SCHEME_KW ]        = "irodsDefaultHashScheme";
        key_map_[ CFG_IRODS_MATCH_HASH_POLICY_KW ]          = "irodsMatchHashPolicy";

        key_map_[ "irodsUserName" ]                = CFG_IRODS_USER_NAME_KW;
        key_map_[ "irodsHost" ]                    = CFG_IRODS_HOST_KW;
        key_map_[ "irodsPort" ]                    = CFG_IRODS_PORT_KW;
        key_map_[ "xmsgHost" ]                     = CFG_IRODS_XMSG_HOST_KW;
        key_map_[ "xmsgPort" ]                     = CFG_IRODS_XMSG_PORT_KW;
        key_map_[ "irodsHome" ]                    = CFG_IRODS_HOME_KW;
        key_map_[ "irodsCwd" ]                     = CFG_IRODS_CWD_KW;
        key_map_[ "irodsAuthScheme" ]              = CFG_IRODS_AUTHENTICATION_SCHEME_KW;
        key_map_[ "irodsDefResource" ]             = CFG_IRODS_DEFAULT_RESOURCE_KW;
        key_map_[ "irodsZone" ]                    = CFG_IRODS_ZONE_KW;
        key_map_[ "irodsServerDn" ]                = CFG_IRODS_GSI_SERVER_DN_KW;
        key_map_[ "irodsLogLevel" ]                = CFG_IRODS_LOG_LEVEL_KW;
        key_map_[ "irodsAuthFileName" ]            = CFG_IRODS_AUTHENTICATION_FILE_NAME_KW;
        key_map_[ "irodsDebug" ]                   = CFG_IRODS_DEBUG_KW;
        key_map_[ "irodsClientServerPolicy" ]      = CFG_IRODS_CLIENT_SERVER_POLICY_KW;
        key_map_[ "irodsClientServerNegotiation" ] = CFG_IRODS_CLIENT_SERVER_NEGOTIATION_KW;
        key_map_[ "irodsEncryptionKeySize" ]       = CFG_IRODS_ENCRYPTION_KEY_SIZE_KW;
        key_map_[ "irodsEncryptionSaltSize" ]      = CFG_IRODS_ENCRYPTION_SALT_SIZE_KW;
        key_map_[ "irodsEncryptionNumHashRounds" ] = CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW;
        key_map_[ "irodsEncryptionAlgorithm" ]     = CFG_IRODS_ENCRYPTION_ALGORITHM_KW;
        key_map_[ "irodsDefaultHashScheme" ]       = CFG_IRODS_DEFAULT_HASH_SCHEME_KW;
        key_map_[ "irodsMatchHashPolicy" ]         = CFG_IRODS_MATCH_HASH_POLICY_KW;

    } // ctor

    error environment_properties::get_json_environment_file(
        std::string& _env_file,
        std::string& _session_file ) {
        // capture parent process id for use in creation of 'session'
        // file which contains the cwd for a given session.  this cwd
        // is only updated by the icd command which writes a new irods
        // CFG_IRODS_CWD_KW to the session dir which repaves the existing
        // one in the original irodsEnv file.
        std::stringstream ppid_str; ppid_str << getppid();

        // if a json version exists, then attempt to capture
        // that
        std::string json_file( IRODS_HOME_DIRECTORY );
        std::string json_session_file( IRODS_HOME_DIRECTORY );
        std::string env_var = to_env( CFG_IRODS_ENVIRONMENT_FILE_KW );
        char* irods_env = getenv(
                              to_env(
                                  CFG_IRODS_ENVIRONMENT_FILE_KW ).c_str() );
        if ( irods_env && strlen( irods_env ) > 0 ) {
            json_file = irods_env;
            // "cwd" is used in this case instead of the ppid given the
            // assumption that scripts will set the env file var to allow
            // for more complex interactions possibly yielding complex pid
            // hierarchies.  this routes all references to the same session
            // for this given use case
            json_session_file = json_file + ".cwd";

        }
        else {
            char* home_dir = getenv( "HOME" );
            // if home env exists, prefer that for run in place
            // or use of existing irods service accound
            if ( home_dir ) {
                json_file = home_dir;
            }

            json_file += JSON_ENV_FILE;
            json_session_file = json_file + "." + ppid_str.str();

        }

        _env_file     = json_file;
        _session_file = json_session_file;

        return SUCCESS();

    } // get_json_environment_file

    error environment_properties::get_legacy_environment_file(
        std::string& _env_file,
        std::string& _session_file ) {
        // capture parent process id for use in creation of 'session'
        // file which contains the cwd for a given session.  this cwd
        // is only updated by the icd command which writes a new irods
        // CFG_IRODS_CWD_KW to the session dir which repaves the existing
        // one in the original irodsEnv file.
        std::stringstream ppid_str; ppid_str << getppid();

        std::string legacy_file( IRODS_HOME_DIRECTORY );
        std::string legacy_session_file( IRODS_HOME_DIRECTORY );
        char* irods_env = getenv(
                              to_env(
                                  CFG_IRODS_ENVIRONMENT_FILE_KW ).c_str() );
        if ( irods_env && strlen( irods_env ) != 0 ) {
            legacy_file = irods_env;

            // "cwd" is used in this case instead of the ppid given the
            // assumption that scripts will set the env file var to allow
            // for more complex interactions possibly yielding complex pid
            // hierarchies.  this routes all references to the same session
            // for this given use case
            legacy_session_file = legacy_file + ".cwd";

        }
        else {
            char* home_dir = getenv( "HOME" );
            // if home env exists, prefer that for run in place
            // or use of existing irods service accound
            if ( home_dir ) {
                legacy_file = home_dir;
            }

            legacy_file += LEGACY_ENV_FILE;
            legacy_session_file = legacy_file + "." + ppid_str.str();

        }

        _env_file     = legacy_file;
        _session_file = legacy_session_file;

        return SUCCESS();

    } // get_legacy_environment_file

    error environment_properties::capture() {
        std::string json_file, json_session_file;
        error ret = get_json_environment_file(
                        json_file,
                        json_session_file );

        bool do_parse_legacy = false;
        if ( ret.ok() ) {
            if ( fs::exists( json_file ) ) {
                ret = capture_json( json_file );
                if ( !ret.ok() ) {
                    // debug - irods::log( PASS( ret ) );
                    do_parse_legacy = true;

                }
                else {
                    config_props_.set< std::string >(
                        CFG_IRODS_ENVIRONMENT_FILE_KW,
                        json_file );
                    ret = capture_json( json_session_file );
                    if ( !ret.ok() ) {
                        // debug - irods::log( PASS( ret ) );

                    }

                    config_props_.set< std::string >(
                        CFG_IRODS_SESSION_ENVIRONMENT_FILE_KW,
                        json_session_file );

                }

            }
            else {
                do_parse_legacy = true;

            }

        }
        else {
            do_parse_legacy = true;

        }

        if ( do_parse_legacy ) {
            std::string legacy_file, legacy_session_file;
            ret = get_legacy_environment_file(
                      legacy_file,
                      legacy_session_file );
            if ( ret.ok() ) {
                ret = capture_legacy( legacy_file );
                if ( !ret.ok() ) {
                    // debug - irods::log( PASS( ret ) );

                }
                config_props_.set< std::string >(
                    CFG_IRODS_ENVIRONMENT_FILE_KW,
                    legacy_file );

                // session file ( written by icd ) already moved to json
                ret = capture_json( legacy_session_file );
                if ( !ret.ok() ) {
                    // debug - irods::log( PASS( ret ) );

                }

                config_props_.set< std::string >(
                    CFG_IRODS_SESSION_ENVIRONMENT_FILE_KW,
                    legacy_session_file );

            }
            else {
                // debug - irods::log( PASS( ret ) );

            }

        } // do parse legacy

        // set the captured flag so we no its already been captured
        captured_ = true;
        return ret;

    } // capture

    error environment_properties::capture_json(
        const std::string& _fn ) {
        error ret = config_props_.load( _fn );

        return ret;

    } // capture_json


    error environment_properties::capture_legacy(
        const std::string& _fn ) {

        std::ifstream in_file(
            _fn.c_str(),
            std::ios::in );
        if ( !in_file.is_open() ) {
            std::string msg( "failed to open legacy file [" );
            msg += _fn;
            msg += "]";
            return ERROR(
                       SYS_INVALID_FILE_PATH,
                       msg );
        }

        std::string line;
        while ( getline( in_file, line ) ) {
            // left trim whitespace
            line.erase(
                line.begin(),
                std::find_if(
                    line.begin(),
                    line.end(),
                    std::not1(
                        std::ptr_fun<int, int>(
                            std::isspace ) ) ) );
            // skip comments
            if ( '#' == line[0] ) {
                continue;
            }

            std::vector< std::string > toks;
            try {
                boost::split(
                    toks,
                    line,
                    boost::is_any_of( "\t " ),
                    boost::token_compress_on );
            }
            catch ( boost::exception& ) {
                rodsLog( LOG_ERROR, "boost::split failed on line [%s]", line.c_str() );
                continue;
            }
            if ( toks.size() != 2 ||
                    toks[0].empty()  ||
                    toks[1].empty() ) {
                rodsLog(
                    LOG_NOTICE,
                    "environment_properties :: invalid line [%s]",
                    line.c_str() );
                continue;
            }

            std::string& key = toks[0];
            if ( key_map_.has_entry( key ) ) {
                std::string pkey = key;
                key = key_map_[ key ];

            } //if has entry

            std::string& val = toks[1];

            char front = *( val.begin() );
            if ( '"'  == front ||
                    '\'' == front ) {
                val.erase( 0, 1 );
            }

            char back = *( val.rbegin() );
            if ( '"'  == back ||
                    '\'' == back ) {
                val.erase( val.size() - 1 );
            }

            error ret;
            if ( CFG_IRODS_PORT_KW                        == key ||
                    CFG_IRODS_XMSG_PORT_KW                   == key ||
                    CFG_IRODS_LOG_LEVEL_KW                   == key ||
                    CFG_IRODS_ENCRYPTION_KEY_SIZE_KW         == key ||
                    CFG_IRODS_ENCRYPTION_SALT_SIZE_KW        == key ||
                    CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW  == key ) {
                try {
                    int i_val = boost::lexical_cast< int >( val );
                    ret = config_props_.set< int >( key, i_val );
                }
                catch ( ... ) {
                    rodsLog(
                        LOG_ERROR,
                        "environment_properties :: lexical_cast failed for [%s]-[%s]",
                        key.c_str(),
                        val.c_str() );
                }

            }
            else {
                ret = config_props_.set <
                      std::string > (
                          key,
                          val );
            }
            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );

            }

        } // while

        in_file.close();

        return SUCCESS();

    } // environment_properties::capture_legacy


} // namespace irods

























