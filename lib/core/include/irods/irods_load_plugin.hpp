#ifndef __IRODS_LOAD_PLUGIN_HPP__
#define __IRODS_LOAD_PLUGIN_HPP__

// =-=-=-=-=-=-=-
// My Includes
#include "irods/irods_log.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_plugin_name_generator.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/getRodsEnv.h"
#include "irods/rodsErrorTable.h"
#include "irods/irods_default_paths.hpp"
#include "irods/irods_exception.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/static_assert.hpp>
#include <boost/filesystem.hpp>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>
#include <elf.h>

namespace irods
{
    inline error resolve_plugin_path(const std::string& _type, std::string& _path)
    {
        namespace fs = boost::filesystem;

        fs::path plugin_home;

        rodsEnv env;
        int status = getRodsEnv( &env );
        if ( !status ) {
            if ( strlen( env.irodsPluginHome ) > 0 ) {
                plugin_home = env.irodsPluginHome;
            }

        }

        if (plugin_home.empty()) {
            try {
                plugin_home = get_irods_default_plugin_directory();
            } catch (const irods::exception& e) {
                irods::log(e);
                return ERROR(SYS_INVALID_INPUT_PARAM, "failed to get default plugin directory");
            }
        }

        plugin_home.append(_type);

        try {
            if ( !fs::exists( plugin_home ) ) {
                std::string msg( "does not exist [" );
                msg += plugin_home.string();
                msg += "]";
                return ERROR(
                           SYS_INVALID_INPUT_PARAM,
                           msg );

            }

            fs::path p = fs::canonical( plugin_home );

            _path = plugin_home.string();

            if ( fs::path::preferred_separator != *_path.rbegin() ) {
                _path += fs::path::preferred_separator;
            }

            rodsLog(
                LOG_DEBUG,
                "resolved plugin home [%s]",
                _path.c_str() );

            return SUCCESS();

        }
        catch ( const fs::filesystem_error& _e ) {
            std::string msg( "does not exist [" );
            msg += plugin_home.string();
            msg += "]\n";
            msg += _e.what();
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       msg );
        }

    } // resolve_plugin_path

    /// \brief attempt to parse DT_FLAGS_1 to identify flags to pass to dlopen().
    ///
    /// \param[in] _plugin_path Path to the plugin to be examined.
    ///
    /// \returns An int containing flags to pass to dlopen().
    ///
    /// \throws std::exception
    ///
    /// \since 4.3.1
    // NOLINTNEXTLINE(readability-function-cognitive-complexity) this is technically a parser
    inline int get_plugin_rtld_flags(const std::string& _plugin_path)
    {
        using logger = irods::experimental::log::server;

        // Start with defaults.
        int rtld_lazy_or_now = RTLD_LAZY;
        int rtld_local_or_global = RTLD_LOCAL;
        int rtld_nodelete = 0;

        try {
            std::ifstream lib(_plugin_path, std::ios::binary);
            if (!lib) {
                // clang-format off
                logger::error({
                    {"plugin_path", _plugin_path},
                    {"log_message", "Could not parse DT_FLAGS_1: failed to open plugin file"}
                });
                // clang-format on
                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                return rtld_lazy_or_now | rtld_local_or_global | rtld_nodelete;
            }

            // make sure we're actually reading an ELF file
            const std::array<char, 4> elf_mag{{ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3}};
            std::array<char, 4> elf_mag_in{};
            // read magic number
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            lib.read(reinterpret_cast<char*>(elf_mag_in.data()), elf_mag_in.size());
            if (elf_mag != elf_mag_in) {
                // clang-format off
                logger::error({
                    {"plugin_path", _plugin_path},
                    {"log_message", "Could not parse DT_FLAGS_1: invalid file format (ELF magic number mismatch)"}
                });
                // clang-format on
                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                return rtld_lazy_or_now | rtld_local_or_global | rtld_nodelete;
            }

            // get ELFCLASS
            std::uint8_t ei_class{};
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            lib.read(reinterpret_cast<char*>(&ei_class), sizeof(ei_class));

            // seek back to the beginning
            lib.seekg(0);

            // read ELF header, pull out needed values
            std::uint64_t e_shoff{}; // offset to section header table
            std::uint16_t e_shentsize{}; // size of section header table entries
            std::uint64_t e_shnum{}; // number of section header table entries
            std::uint32_t e_shstrndx{}; // section table header index for string table containing section names
            if (ELFCLASS32 == ei_class) {
                Elf32_Ehdr elf_hdr_in{};
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                lib.read(reinterpret_cast<char*>(&elf_hdr_in), sizeof(elf_hdr_in));
                e_shoff = elf_hdr_in.e_shoff;
                e_shentsize = elf_hdr_in.e_shentsize;
                e_shnum = elf_hdr_in.e_shnum;
                e_shstrndx = elf_hdr_in.e_shstrndx;
            }
            else if (ELFCLASS64 == ei_class) {
                Elf64_Ehdr elf_hdr_in{};
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                lib.read(reinterpret_cast<char*>(&elf_hdr_in), sizeof(elf_hdr_in));
                e_shoff = elf_hdr_in.e_shoff;
                e_shentsize = elf_hdr_in.e_shentsize;
                e_shnum = elf_hdr_in.e_shnum;
                e_shstrndx = elf_hdr_in.e_shstrndx;
            }
            else {
                // clang-format off
                logger::warn({
                    {"plugin_path", _plugin_path},
                    {"log_message", "Could not parse DT_FLAGS_1: unknown or invalid ELFCLASS"},
                    {"ELFCLASS", std::to_string(static_cast<std::uint32_t>(ei_class))}
                });
                // clang-format on
                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                return rtld_lazy_or_now | rtld_local_or_global | rtld_nodelete;
            }

            // TODO: some platforms do not require a section table for libraries
            // For now, if there's no section table, we're done.
            if (0 == e_shoff) {
                // clang-format off
                logger::warn({
                    {"plugin_path", _plugin_path},
                    {"log_message", "Could not parse DT_FLAGS_1: No section header table"}
                });
                // clang-format on
                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                return rtld_lazy_or_now | rtld_local_or_global | rtld_nodelete;
            }

            // seek to section table
            lib.seekg(e_shoff);

            // some data may be contained in the first entry
            if ((0 == e_shnum) || (SHN_XINDEX == e_shstrndx)) {
                if (ei_class == ELFCLASS32) {
                    Elf32_Shdr s_hdr_in{};
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    lib.read(reinterpret_cast<char*>(&s_hdr_in), sizeof(s_hdr_in));
                    if (0 == e_shnum) {
                        e_shnum = s_hdr_in.sh_size;
                    }
                    if (SHN_XINDEX == e_shstrndx) {
                        e_shstrndx = s_hdr_in.sh_link;
                    }
                }
                else {
                    Elf64_Shdr s_hdr_in{};
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    lib.read(reinterpret_cast<char*>(&s_hdr_in), sizeof(s_hdr_in));
                    if (0 == e_shnum) {
                        e_shnum = s_hdr_in.sh_size;
                    }
                    if (SHN_XINDEX == e_shstrndx) {
                        e_shstrndx = s_hdr_in.sh_link;
                    }
                }
            }

            // offset of section name string table section header
            const std::uint64_t e_shstroff = e_shoff + (static_cast<std::uint64_t>(e_shstrndx) * e_shentsize);
            // seek to string table section header
            lib.seekg(e_shstroff);

            std::uint64_t e_strtoff{}; // offset of string table
            std::uint64_t e_strtsz{}; // size of string table
            if (ELFCLASS32 == ei_class) {
                Elf32_Shdr s_hdr_in{};
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                lib.read(reinterpret_cast<char*>(&s_hdr_in), sizeof(s_hdr_in));
                e_strtoff = s_hdr_in.sh_offset;
                e_strtsz = s_hdr_in.sh_size;
            }
            else {
                Elf64_Shdr s_hdr_in{};
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                lib.read(reinterpret_cast<char*>(&s_hdr_in), sizeof(s_hdr_in));
                e_strtoff = s_hdr_in.sh_offset;
                e_strtsz = s_hdr_in.sh_size;
            }

            // seek to section name string table
            lib.seekg(e_strtoff);

            // read in the entire string table
            std::vector<char> e_strtbl_vec(e_strtsz);
            lib.read(e_strtbl_vec.data(), e_strtsz);

            // Implementation note:
            // On the off chance that there is more than one relevant section header or more
            // than one DT_FLAGS_1 dynamic entry, we do not stop after the first section header
            // or dynamic entry match. Should there be more than one DT_FLAGS_1 dynamic entry,
            // the effects are cumulative. Flags set by one dynamic entry cannot be unset by
            // another, even if they are in different dynamic sections/segments.

            // TODO: some platforms do not include a section header for the dynamic table
            // TODO: some platforms do not mark the dynamic table section header as SHT_DYNAMIC
            for (std::uint64_t sh_idx = 1; sh_idx < e_shnum; sh_idx++) {
                // seek to relevant section header
                lib.seekg(e_shoff + (sh_idx * e_shentsize));

                std::uint32_t sh_name{}; // section name (offset in section name string table)
                std::uint32_t sh_type{}; // section type
                std::uint64_t sh_offset{}; // section offset
                std::uint64_t sh_size{}; // section size
                std::uint64_t sh_entsize{}; // size of individual entities within the section
                std::string_view s_name{}; // section name
                if (ELFCLASS32 == ei_class) {
                    Elf32_Shdr s_hdr_in{};
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    lib.read(reinterpret_cast<char*>(&s_hdr_in), sizeof(s_hdr_in));

                    sh_type = s_hdr_in.sh_type;
                    if (SHT_DYNAMIC != sh_type) {
                        continue;
                    }
                    sh_name = s_hdr_in.sh_name;
                    s_name = std::string_view(&e_strtbl_vec.data()[sh_name]);
                    if (!s_name.starts_with(".dynamic")) {
                        continue;
                    }

                    sh_offset = s_hdr_in.sh_offset;
                    sh_size = s_hdr_in.sh_size;
                    sh_entsize = s_hdr_in.sh_entsize;
                }
                else {
                    Elf64_Shdr s_hdr_in{};
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    lib.read(reinterpret_cast<char*>(&s_hdr_in), sizeof(s_hdr_in));

                    sh_type = s_hdr_in.sh_type;
                    if (SHT_DYNAMIC != sh_type) {
                        continue;
                    }
                    sh_name = s_hdr_in.sh_name;
                    s_name = std::string_view(&e_strtbl_vec.data()[sh_name]);
                    if (!s_name.starts_with(".dynamic")) {
                        continue;
                    }

                    sh_offset = s_hdr_in.sh_offset;
                    sh_size = s_hdr_in.sh_size;
                    sh_entsize = s_hdr_in.sh_entsize;
                }

                // offset to the end of the section
                const std::uint64_t sh_offset_end = sh_offset + sh_size;

                for (std::uint64_t de_off = sh_offset; de_off < sh_offset_end; de_off += sh_entsize) {
                    // seek to dynamic entry
                    lib.seekg(de_off);

                    std::uint64_t dt_flags_1{}; // DT_FLAGS_1 value
                    if (ELFCLASS32 == ei_class) {
                        Elf32_Dyn de_in{};
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                        lib.read(reinterpret_cast<char*>(&de_in), sizeof(de_in));

                        if (DT_FLAGS_1 != de_in.d_tag) {
                            continue;
                        }

                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                        dt_flags_1 = de_in.d_un.d_val;
                    }
                    else {
                        Elf64_Dyn de_in{};
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                        lib.read(reinterpret_cast<char*>(&de_in), sizeof(de_in));

                        if (DT_FLAGS_1 != de_in.d_tag) {
                            continue;
                        }

                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                        dt_flags_1 = de_in.d_un.d_val;
                    }

                    // check for flags
                    if ((dt_flags_1 & static_cast<std::uint64_t>(DF_1_NOW)) == DF_1_NOW) {
                        rtld_lazy_or_now = RTLD_NOW;
                    }
                    if ((dt_flags_1 & static_cast<std::uint64_t>(DF_1_GLOBAL)) == DF_1_GLOBAL) {
                        rtld_local_or_global = RTLD_GLOBAL;
                    }
                    if ((dt_flags_1 & static_cast<std::uint64_t>(DF_1_NODELETE)) == DF_1_NODELETE) {
                        rtld_nodelete = RTLD_NODELETE;
                    }
                }
            }
        }
        catch (const std::exception& _e) {
            // clang-format off
            logger::error({
                {"plugin_path", _plugin_path},
                {"log_message", "Caught exception while parsing DT_FLAGS_1"},
                {"exception", _e.what()}
            });
            // clang-format on
        }
        catch (...) {
            // clang-format off
            logger::error({
                {"plugin_path", _plugin_path},
                {"log_message", "Caught unknown exception while parsing DT_FLAGS_1"}
            });
            // clang-format on
        }
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        return rtld_lazy_or_now | rtld_local_or_global | rtld_nodelete;
    } // get_plugin_rtld_flags

    /**
     * \fn PluginType* load_plugin( PluginType*& _plugin, const std::string& _plugin_name, const std::string& _interface, const std::string& _instance_name, const Ts&... _args );
     *
     * \brief load a plugin object from a given shared object / dll name
     *
     * \user developer
     *
     * \ingroup core
     *
     * \since   4.0
     *
     *
     * \usage
     * ms_table_entry* tab_entry;\n
     * tab_entry = load_plugin( "some_microservice_name", "/var/lib/irods/server/bin" );
     *
     * \param[in] _plugin          - the plugin instance
     * \param[in] _plugin_name     - name of plugin you wish to load, which will have
     *                                  all non-alphanumeric characters removed, as found in
     *                                  a file named "lib" clean_plugin_name + ".so"
     * \param[in] _interface       - plugin interface: resource, network, auth, etc.
     * \param[in] _instance_name   - the name of the plugin after it is loaded
     * \param[in] _args            - arguments to pass to the loaded plugin
     *
     * \return PluginType*
     * \retval non-null on success
     **/
    template< typename PluginType, typename ...Ts >
    error load_plugin(PluginType*&       _plugin,
                      const std::string& _plugin_name,
                      const std::string& _interface,
                      const std::string& _instance_name,
                      const Ts&...       _args)
    {
        namespace fs = boost::filesystem;

        // resolve the plugin path
        std::string plugin_home;
        error ret = resolve_plugin_path( _interface, plugin_home );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // Generate the shared lib name
        std::string so_name;
        plugin_name_generator name_gen;
        ret = name_gen( _plugin_name, plugin_home, so_name );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to generate an appropriate shared library name for plugin: \"";
            msg << _plugin_name << "\".";
            return PASSMSG( msg.str(), ret );
        }

        try {
            if ( !fs::exists( so_name ) ) {
                std::string msg( "shared library does not exist [" );
                msg += so_name;
                msg += "]";
                return ERROR( PLUGIN_ERROR_MISSING_SHARED_OBJECT, msg );
            }
        }
        catch ( const fs::filesystem_error& _e ) {
            std::string msg( "boost filesystem exception when checking existence of [" );
            msg += so_name;
            msg += "] with message [";
            msg += _e.what();
            msg += "]";
            return ERROR( PLUGIN_ERROR_MISSING_SHARED_OBJECT, msg );
        }

        // =-=-=-=-=-=-=-
        // try to open the shared object
        const auto rtld_flags = get_plugin_rtld_flags(so_name);
        void* handle = dlopen(so_name.c_str(), rtld_flags);
        if ( !handle ) {
            std::stringstream msg;
            msg << "failed to open shared object file [" << so_name
                << "] :: dlerror: is [" << dlerror() << "]";
            return ERROR( PLUGIN_ERROR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // clear any existing dlerrors
        dlerror();

        // =-=-=-=-=-=-=-
        // attempt to load the plugin factory function from the shared object
        typedef PluginType* ( *factory_type )( const std::string& , const Ts&... );
        factory_type factory = reinterpret_cast< factory_type >( dlsym( handle, "plugin_factory" ) );
        char* err = dlerror();
        if ( 0 != err || !factory ) {
            std::stringstream msg;
            msg << "failed to load symbol from shared object handle - plugin_factory"
                << " :: dlerror is [" << err << "]";
            dlclose( handle );
            return ERROR( PLUGIN_ERROR, msg.str() );
        }

        rodsLog(LOG_DEBUG, "load_plugin - calling plugin_factory() in [%s]", so_name.c_str());

        // =-=-=-=-=-=-=-
        // using the factory pointer create the plugin
        _plugin = factory( _instance_name, _args... );
        if ( !_plugin ) {
            std::stringstream msg;
            msg << "failed to create plugin object for [" << _plugin_name << "]";
            dlclose( handle );
            return ERROR( PLUGIN_ERROR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // notify world of success
        // TODO :: add hash checking and provide hash value for log also
        rodsLog(
            LOG_DEBUG,
            "load_plugin - loaded [%s]",
            _plugin_name.c_str() );

        return SUCCESS();

    } // load_plugin
} // namespace irods

#endif // __IRODS_LOAD_PLUGIN_HPP__
