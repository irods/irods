#include <dlfcn.h>
#include <boost/filesystem.hpp>
#include "irods_exception.hpp"


namespace irods {
    boost::filesystem::path
    get_irods_root_directory() {
        Dl_info dl_info;
        const int dladdr_ret = dladdr(__FUNCTION__, &dl_info);
        if (dladdr_ret == 0) {
            THROW(-1, "dladdr returned 0");
        }

        if (dl_info.dli_fname == nullptr) {
            THROW(-1, "dli_fname is null");
        }
        try {
            boost::filesystem::path path{dl_info.dli_fname};
            path = boost::filesystem::canonical(path);
            path.remove_filename().remove_filename().remove_filename(); // Removes filename and the two directories (usr and lib) between libirods_common.so and base of irods install
            return path;
        } catch(const boost::filesystem::filesystem_error& e) {
            THROW(-1, e.what());
        }
    }

    boost::filesystem::path
    get_irods_config_directory() {
        boost::filesystem::path path{get_irods_root_directory()};
        path.append("etc").append("irods");
        return path;
    }

    boost::filesystem::path
    get_irods_home_directory() {
        boost::filesystem::path path{get_irods_root_directory()};
        path.append("var").append("lib").append("irods");
        return path;
    }

    boost::filesystem::path
    get_irods_default_plugin_directory() {
        boost::filesystem::path path{get_irods_root_directory()};
        path.append("usr").append("lib").append("irods").append("plugins");
        return path;
    }
}
