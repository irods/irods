#ifndef __IRODS_GET_FULL_PATH_FOR_CONFIG_FILE_HPP__
#define __IRODS_GET_FULL_PATH_FOR_CONFIG_FILE_HPP__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// irods includes
#include "irods_error.hpp"

namespace irods {

/// =-=-=-=-=-=-=-
/// @brief service which searches for a specified config file and returns its location
    error get_full_path_for_config_file(
        const std::string&, // config file name
        std::string& );     // full path for config file
    error get_full_path_for_unmoved_configs(
        const std::string&, // config file name
        std::string& );     // full path for config file

}; // namespace irods

#endif // __IRODS_GET_FULL_PATH_FOR_CONFIG_FILE_HPP__
