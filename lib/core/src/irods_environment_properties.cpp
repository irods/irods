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
            irods::log(e);
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

// Access method for singleton
    environment_properties& environment_properties::instance() {
        static environment_properties instance;
        return instance;
    }

    environment_properties::environment_properties()
    {
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
                irods::log(e);
            }
        }
        else {
            irods::log( PASS( ret ) );
        }

    } // capture

    void environment_properties::capture_json(
        const std::string& _fn ) {
        error ret = config_props_.load( _fn );
        if (!ret.ok() ) {
            THROW(ret.code(), ret.result());
        }

    } // capture_json

    void environment_properties::remove(const std::string& _key ) {
        config_props_.remove(_key);
    }

} // namespace irods
