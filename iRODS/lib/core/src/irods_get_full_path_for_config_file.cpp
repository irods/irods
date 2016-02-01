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
#include "irods_home_directory.hpp"
#include "irods_get_full_path_for_config_file.hpp"

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief service which searches for a specified config file and returns its location
    error get_full_path_for_config_file(
        const std::string& _cfg_file,
        std::string&       _full_path ) {
        namespace fs = boost::filesystem;

        // =-=-=-=-=-=-=-
        // build a list of paths which will be searched for a
        // given config file.  this list is searched in the order in
        // which the paths are added to the vector
        std::vector< std::string > search_paths;
        search_paths.push_back( IRODS_HOME_DIRECTORY + "/iRODS/server/config/reConfigs/" );
        search_paths.push_back( IRODS_HOME_DIRECTORY + "/iRODS/server/config/" );
        search_paths.push_back( IRODS_HOME_DIRECTORY + "/iRODS/config/" );
        search_paths.push_back( "/etc/irods/" );

        std::vector< std::string >::iterator itr = search_paths.begin();
        // =-=-=-=-=-=-=-
        // walk the list of paths and determine if a file exists
        for ( ; itr != search_paths.end(); ++itr ) {
            fs::path fn( *itr + _cfg_file );
            if ( fs::exists( fn ) ) {
                _full_path = fn.string();

                rodsLog( LOG_DEBUG, "config file found [%s]", _full_path.c_str() );
                return SUCCESS();

            }

        } // for itr

        // =-=-=-=-=-=-=-
        // config file not found in any of the search paths
        std::string msg( "config file not found [" );
        msg += _cfg_file + "]";
        return ERROR( SYS_INVALID_INPUT_PARAM,
                      msg );

    } // get_full_path_for_config_file

}; // namespace irods
