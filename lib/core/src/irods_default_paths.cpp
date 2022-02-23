#include "irods/irods_default_paths.hpp"

#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"

#include <boost/filesystem.hpp>

#include <dlfcn.h>

#include <iterator>

namespace fs = boost::filesystem;

namespace irods
{
    fs::path
    get_irods_lib_directory() {
        Dl_info dl_info;
        const int dladdr_ret = dladdr(__FUNCTION__, &dl_info);
        if (dladdr_ret == 0) {
            THROW(-1, "dladdr returned 0");
        }

        if (dl_info.dli_fname == nullptr) {
            THROW(-1, "dli_fname is null");
        }
        try {
            fs::path path{dl_info.dli_fname};
            path = fs::canonical(path);
            path.remove_filename();
            return path;
        } catch(const fs::filesystem_error& e) {
            THROW(SYS_INTERNAL_ERR, e.what());
        }
    }

    fs::path
    get_irods_root_directory() {
        try {
            fs::path path = get_irods_lib_directory();
            fs::path install_libdir{IRODS_DEFAULT_PATH_LIBDIR};
            install_libdir = install_libdir.lexically_normal();
            long install_path_len = std::distance(install_libdir.begin(), install_libdir.end());
            if (install_libdir.is_absolute()) {
                install_path_len--;
            }
            for (long i = 0; i < install_path_len; i++) {
                path.remove_filename(); // Removes directories (usually usr and lib) between libirods_common.so and base of irods install
            }
            return path;
        } catch(const fs::filesystem_error& e) {
            THROW(SYS_INTERNAL_ERR, e.what());
        }
    }

    // we can tolerate a little re-prefixing if we ignore common base directories.
    // For example, if libdir is "usr/lib" and bindir is "usr/bin", we can ignore
    // "usr" when traversing from one to the other.
    static inline fs::path
    get_irods_directory_impl(fs::path install_path) {
        // if input path is absolute, do not traverse
        if (install_path.is_absolute()) {
            return install_path;
        }
        fs::path path = get_irods_lib_directory();
        fs::path install_libdir{IRODS_DEFAULT_PATH_LIBDIR};
        install_libdir = install_libdir.lexically_normal();
        // if install_libdir is absolute, we can't be smart about it
        if (install_libdir.is_absolute()) {
            for (auto ptr = ++install_libdir.begin(); ptr != install_libdir.end(); ptr++) {
                path.remove_filename();
            }
            return path / install_path;
        }
        fs::path install_path_rel = install_path.lexically_relative(install_libdir);
        // if there is no common base, we can't be smart about it
        if (install_path_rel.empty()) {
            for (auto ptr = install_libdir.begin(); ptr != install_libdir.end(); ptr++) {
                path.remove_filename();
            }
            return path / install_path;
        }

        path /= install_path_rel;
        return path.lexically_normal();
    }

    fs::path
    get_irods_config_directory() {
        fs::path install_confdir{IRODS_DEFAULT_PATH_SYSCONFDIR};
        install_confdir = install_confdir.lexically_normal();
        install_confdir.append("irods");
        return get_irods_directory_impl(install_confdir);
    }

    fs::path
    get_irods_home_directory() {
        fs::path install_homedir{IRODS_DEFAULT_PATH_HOMEDIR};
        install_homedir = install_homedir.lexically_normal();
        return get_irods_directory_impl(install_homedir);
    }

    fs::path
    get_irods_default_plugin_directory() {
        fs::path install_plugdir{IRODS_DEFAULT_PATH_PLUGINDIR};
        install_plugdir = install_plugdir.lexically_normal();
        return get_irods_directory_impl(install_plugdir);
    }
} // namespace irods
