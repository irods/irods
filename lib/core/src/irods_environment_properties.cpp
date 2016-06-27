/*
 * irods_environment_properties.cpp
 *
 */

#include "irods_environment_properties.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_default_paths.hpp"
#include "irods_exception.hpp"

#include "rods.h"
#include "irods_log.hpp"
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

    error get_json_environment_file(
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
        std::string json_file;
        try {
            json_file = get_irods_home_directory().string();
        } catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            return ERROR(-1, "failed to get irods home directory");
        }
        std::string json_session_file = json_file;
        std::string env_var = to_env( CFG_IRODS_ENVIRONMENT_FILE_KW );
        char* irods_env = getenv(env_var.c_str());
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

            json_file += IRODS_JSON_ENV_FILE;
            json_session_file = json_file + "." + ppid_str.str();

        }

        _env_file     = json_file;
        _session_file = json_session_file;

        return SUCCESS();

    } // get_json_environment_file

    error get_legacy_environment_file(
        std::string& _env_file,
        std::string& _session_file ) {
        // capture parent process id for use in creation of 'session'
        // file which contains the cwd for a given session.  this cwd
        // is only updated by the icd command which writes a new irods
        // CFG_IRODS_CWD_KW to the session dir which repaves the existing
        // one in the original irodsEnv file.
        std::stringstream ppid_str; ppid_str << getppid();

        std::string legacy_file;
        try {
            legacy_file = get_irods_home_directory().string();
        } catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            return ERROR(-1, "failed to get irods home directory");
        }

        std::string legacy_session_file = legacy_file;
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

            legacy_file += IRODS_LEGACY_ENV_FILE;
            legacy_session_file = legacy_file + "." + ppid_str.str();

        }

        _env_file     = legacy_file;
        _session_file = legacy_session_file;

        if ( fs::exists( legacy_file ) ) {
            rodsLog( LOG_ERROR, "Warning: use of legacy configuration [%s] is deprecated.", legacy_file.c_str() );
        }

        return SUCCESS();

    } // get_legacy_environment_file


// Access method for singleton
    environment_properties& environment_properties::instance() {
        static environment_properties instance;
        return instance;
    }

    environment_properties::environment_properties()
    {
        legacy_key_map_[ "irodsUserName" ]                = CFG_IRODS_USER_NAME_KW;
        legacy_key_map_[ "irodsHost" ]                    = CFG_IRODS_HOST_KW;
        legacy_key_map_[ "irodsPort" ]                    = CFG_IRODS_PORT_KW;
        legacy_key_map_[ "xmsgHost" ]                     = CFG_IRODS_XMSG_HOST_KW;
        legacy_key_map_[ "xmsgPort" ]                     = CFG_IRODS_XMSG_PORT_KW;
        legacy_key_map_[ "irodsHome" ]                    = CFG_IRODS_HOME_KW;
        legacy_key_map_[ "irodsCwd" ]                     = CFG_IRODS_CWD_KW;
        legacy_key_map_[ "irodsAuthScheme" ]              = CFG_IRODS_AUTHENTICATION_SCHEME_KW;
        legacy_key_map_[ "irodsDefResource" ]             = CFG_IRODS_DEFAULT_RESOURCE_KW;
        legacy_key_map_[ "irodsZone" ]                    = CFG_IRODS_ZONE_KW;
        legacy_key_map_[ "irodsServerDn" ]                = CFG_IRODS_GSI_SERVER_DN_KW;
        legacy_key_map_[ "irodsLogLevel" ]                = CFG_IRODS_LOG_LEVEL_KW;
        legacy_key_map_[ "irodsAuthFileName" ]            = CFG_IRODS_AUTHENTICATION_FILE_KW;
        legacy_key_map_[ "irodsDebug" ]                   = CFG_IRODS_DEBUG_KW;
        legacy_key_map_[ "irodsClientServerPolicy" ]      = CFG_IRODS_CLIENT_SERVER_POLICY_KW;
        legacy_key_map_[ "irodsClientServerNegotiation" ] = CFG_IRODS_CLIENT_SERVER_NEGOTIATION_KW;
        legacy_key_map_[ "irodsEncryptionKeySize" ]       = CFG_IRODS_ENCRYPTION_KEY_SIZE_KW;
        legacy_key_map_[ "irodsEncryptionSaltSize" ]      = CFG_IRODS_ENCRYPTION_SALT_SIZE_KW;
        legacy_key_map_[ "irodsEncryptionNumHashRounds" ] = CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW;
        legacy_key_map_[ "irodsEncryptionAlgorithm" ]     = CFG_IRODS_ENCRYPTION_ALGORITHM_KW;
        legacy_key_map_[ "irodsDefaultHashScheme" ]       = CFG_IRODS_DEFAULT_HASH_SCHEME_KW;
        legacy_key_map_[ "irodsMatchHashPolicy" ]         = CFG_IRODS_MATCH_HASH_POLICY_KW;

        capture();
    } // ctor



    void environment_properties::capture() {
        std::string json_file, json_session_file;
        error ret = get_json_environment_file(
                        json_file,
                        json_session_file );

        if ( ret.ok() && fs::exists( json_file ) ) {
            try {
                capture_json( json_file );
                config_props_.set< std::string >( CFG_IRODS_ENVIRONMENT_FILE_KW, json_file );
                try {
                    capture_json( json_session_file );
                } catch ( const irods::exception& e ) {
                    // debug - irods::log( PASS( ret ) );
                }
                config_props_.set< std::string >( CFG_IRODS_SESSION_ENVIRONMENT_FILE_KW, json_session_file );
                return;
            } catch ( const irods::exception& e ) {
                rodsLog( LOG_ERROR, e.what() );
            }
        }

        //capture legacy environment
        std::string legacy_file, legacy_session_file;
        ret = get_legacy_environment_file(
                    legacy_file,
                    legacy_session_file );
        if ( ret.ok() ) {
            try {
                capture_legacy( legacy_file );
                config_props_.set< std::string >( CFG_IRODS_ENVIRONMENT_FILE_KW, legacy_file );
            } catch ( const irods::exception& e ) {
                // debug - irods::log( PASS( ret ) );
            }

            // session file ( written by icd ) already moved
            // to json
            try {
                capture_json( legacy_session_file );
            } catch ( const irods::exception& e ) {
                // debug - irods::log( PASS( ret ) );
            }

            config_props_.set< std::string >( CFG_IRODS_SESSION_ENVIRONMENT_FILE_KW, legacy_session_file );

        }
        else {
            // debug - irods::log( PASS( ret ) );

        }


    } // capture

    void environment_properties::capture_json(
        const std::string& _fn ) {
        error ret = config_props_.load( _fn );
        if (!ret.ok() ) {
            THROW(ret.code(), ret.result());
        }

    } // capture_json


    void environment_properties::capture_legacy(
        const std::string& _fn ) {

        std::ifstream in_file(
            _fn.c_str(),
            std::ios::in );
        if ( !in_file.is_open() ) {
            std::string msg( "failed to open legacy file [" );
            msg += _fn;
            msg += "]";
            THROW( SYS_INVALID_FILE_PATH, msg );
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
            try {
                //if has entry
                key = legacy_key_map_.at( key );
            } catch ( const std::out_of_range& ) {}

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

            if ( CFG_IRODS_PORT_KW                        == key ||
                    CFG_IRODS_XMSG_PORT_KW                   == key ||
                    CFG_IRODS_LOG_LEVEL_KW                   == key ||
                    CFG_IRODS_ENCRYPTION_KEY_SIZE_KW         == key ||
                    CFG_IRODS_ENCRYPTION_SALT_SIZE_KW        == key ||
                    CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW  == key ) {
                try {
                    int i_val = boost::lexical_cast< int >( val );
                    config_props_.set< int >( key, i_val );
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
                config_props_.set < std::string > ( key, val );
            }

        } // while

        in_file.close();

    } // environment_properties::capture_legacy

    void environment_properties::remove(const std::string& _key ) {
        config_props_.remove(_key);
    }

} // namespace irods
