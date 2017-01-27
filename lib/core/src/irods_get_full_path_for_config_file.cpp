// =-=-=-=-=-=-=-
// boost includes
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

// =-=-=-=-=-=-=-
// stl includes
#include <vector>

// =-=-=-=-=-=-=-
// irods includes
#include "rodsErrorTable.h"
#include "irods_log.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_default_paths.hpp"
#include "irods_exception.hpp"

namespace irods {
    error get_full_path_for_config_file(
        const std::string& _cfg_file,
        std::string&       _full_path ) {
        namespace fs = boost::filesystem;

        try {
            fs::path path = get_irods_config_directory();
            path.append(_cfg_file);
            if ( fs::exists(path) ) {
                _full_path = path.string();
                rodsLog( LOG_DEBUG, "config file found [%s]", _full_path.c_str() );
                return SUCCESS();
            }
        } catch (const irods::exception& e) {
            irods::log(e);
            return ERROR(-1, "failed to get irods home directory");
        }

        std::string msg( "config file not found [" );
        msg += _cfg_file + "]";
        return ERROR(SYS_INVALID_INPUT_PARAM, msg);
    }

    error get_full_path_for_unmoved_configs(
        const std::string& _cfg_file,
        std::string&       _full_path ) {
        namespace fs = boost::filesystem;

        try {
            fs::path path = get_irods_home_directory();
            path.append("config").append(_cfg_file);
            if ( fs::exists(path) ) {
                _full_path = path.string();
                rodsLog( LOG_DEBUG, "config file found [%s]", _full_path.c_str() );
                return SUCCESS();
            }
        } catch (const irods::exception& e) {
            irods::log(e);
            return ERROR(-1, "failed to get irods home directory");
        }

        std::string msg( "config file not found [" );
        msg += _cfg_file + "]";
        return ERROR(SYS_INVALID_INPUT_PARAM, msg);
    }
}
