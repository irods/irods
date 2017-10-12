#ifndef __IRODS_LOAD_PLUGIN_HPP__
#define __IRODS_LOAD_PLUGIN_HPP__

// =-=-=-=-=-=-=-
// My Includes
#include "irods_log.hpp"
#include "irods_plugin_name_generator.hpp"
#include "irods_configuration_keywords.hpp"
#include "getRodsEnv.h"
#include "rodsErrorTable.h"
#include "irods_default_paths.hpp"
#include "irods_exception.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/static_assert.hpp>
#include <boost/filesystem.hpp>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>


namespace irods {

    static error resolve_plugin_path(
        const std::string& _type,
        std::string&       _path ) {
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
    error load_plugin( PluginType*&       _plugin,
                       const std::string& _plugin_name,
                       const std::string& _interface,
                       const std::string& _instance_name,
                       const Ts&... _args ) {
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
        void*  handle  = dlopen( so_name.c_str(), RTLD_LAZY | RTLD_LOCAL );
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

}; // namespace irods



#endif // __IRODS_LOAD_PLUGIN_HPP__
