#ifndef IRODS_DEFAULT_PATHS_HPP
#define IRODS_DEFAULT_PATHS_HPP

#include <boost/filesystem.hpp>

namespace irods {
    boost::filesystem::path get_irods_root_directory();
    boost::filesystem::path get_irods_config_directory();
    boost::filesystem::path get_irods_home_directory();
    boost::filesystem::path get_irods_default_plugin_directory();
}

#endif
