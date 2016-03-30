#ifndef __IRODS_NETWORK_MANAGER_HPP__
#define __IRODS_NETWORK_MANAGER_HPP__

// =-=-=-=-=-=-=-
#include "irods_network_plugin.hpp"

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief definition of the network interface
    const std::string NETWORK_INTERFACE( "irods_network_interface" );

/// =-=-=-=-=-=-=-
/// @brief singleton class which manages the lifetime of
///        network plugins
    class network_manager {
        public:
            // =-=-=-=-=-=-=-
            // constructors
            network_manager();
            network_manager( const network_manager& );

            // =-=-=-=-=-=-=-
            // destructor
            virtual ~network_manager();

            /// =-=-=-=-=-=-=-
            /// @brief interface which will return a network plugin
            ///        given its instance name
            error resolve(
                std::string,    // key / instance name of plugin
                network_ptr& ); // plugin instance

            /// =-=-=-=-=-=-=-
            /// @brief given a type, load up a network plugin from
            ///        a shared object
            error init_from_type(
                const int&,           // proc type
                const std::string&,   // type
                const std::string&,   // key
                const std::string&,   // instance name
                const std::string&,   // context
                network_ptr& ); // plugin instance

        private:
            // =-=-=-=-=-=-=-
            // attributes
            lookup_table< network_ptr > plugins_;

    }; // class network_manager

    extern network_manager netwk_mgr;

}; // namespace irods

#endif // __IRODS_NETWORK_MANAGER_HPP__




