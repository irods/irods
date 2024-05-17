#ifndef __IRODS_NETWORK_MANAGER_HPP__
#define __IRODS_NETWORK_MANAGER_HPP__

// =-=-=-=-=-=-=-
#include "irods/irods_network_plugin.hpp"

#include <shared_mutex>
#include <string>
#include <tuple>
#include <unordered_map>

#include <boost/container_hash/hash.hpp>

namespace irods
{
    /// =-=-=-=-=-=-=-
    /// @brief definition of the network interface
    const std::string NETWORK_INTERFACE("irods_network_interface");

    /// =-=-=-=-=-=-=-
    /// @brief singleton class which manages the lifetime of
    ///        network plugins
    class network_manager
    {
      public:
        // =-=-=-=-=-=-=-
        // constructors
        network_manager();
        network_manager(const network_manager&);

        // =-=-=-=-=-=-=-
        // destructor
        virtual ~network_manager();

        /// =-=-=-=-=-=-=-
        /// @brief return a network plugin, loading from a shared object if
        ///        if necessary
        error get_plugin(
            const int&,           // proc type
            const std::string&,   // type
            const std::string&,   // instance name
            const std::string&,   // context
            network_ptr&);        // plugin instance

        /// =-=-=-=-=-=-=-
        /// @brief interface which will return a network plugin
        ///        given its instance name
        [[deprecated("irods::network_manager::resolve is deprecated. Use irods::network_manager::get_plugin instead.")]]
        error resolve(
            const std::string&,   // key / instance name of plugin
            network_ptr&);        // plugin instance

        /// =-=-=-=-=-=-=-
        /// @brief given a type, load up a network plugin from
        ///        a shared object
        [[deprecated("irods::network_manager::init_from_type is deprecated. Use irods::network_manager::get_plugin instead.")]]
        error init_from_type(
            const int&,           // proc type
            const std::string&,   // type
            const std::string&,   // key
            const std::string&,   // instance name
            const std::string&,   // context
            network_ptr&);        // plugin instance

      private:
        using plugin_map_key = std::tuple<
            const int,            // proc type
            const std::string,    // plugin type
            const std::string,    // instance name
            const std::string>;   // context
        using plugin_map_hasher = boost::hash<plugin_map_key>;
        using plugin_lookup_table = std::unordered_map<plugin_map_key, network_ptr, plugin_map_hasher>;

        plugin_lookup_table plugins_;
        std::shared_mutex table_lock_;

        lookup_table<network_ptr> plugins_old_;


    }; // class network_manager

    extern network_manager netwk_mgr;

}; // namespace irods

#endif // __IRODS_NETWORK_MANAGER_HPP__
