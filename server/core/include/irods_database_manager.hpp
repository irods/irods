#ifndef __IRODS_DATABASE_MANAGER_HPP__
#define __IRODS_DATABASE_MANAGER_HPP__

#include "irods_database_plugin.hpp"

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief definition of the database interface
    const std::string DATABASE_INTERFACE( "irods_database_interface" );

/// =-=-=-=-=-=-=-
/// @brief singleton class which manages the lifetime of
///        database plugins
    class database_manager {
        public:
            // =-=-=-=-=-=-=-
            // constructors
            database_manager();
            database_manager( const database_manager& );

            // =-=-=-=-=-=-=-
            // destructor
            virtual ~database_manager();

            /// =-=-=-=-=-=-=-
            /// @brief interface which will return a database plugin
            ///        given its instance name
            error resolve(
                std::string,    // key / instance name of plugin
                database_ptr& ); // plugin instance

            /// =-=-=-=-=-=-=-
            /// @brief given a type, load up a database plugin from
            ///        a shared object
            error init_from_type(
                const std::string&,   // type
                const std::string&,   // key
                const std::string&,   // instance name
                const std::string&,   // context
                database_ptr& ); // plugin instance

            error load_database_plugin(
                database_ptr&,        // plugin
                const std::string&,   // plugin name
                const std::string&,   // instance name
                const std::string& ); // context string

        private:
            // =-=-=-=-=-=-=-
            // attributes
            lookup_table< database_ptr > plugins_;

    }; // class database_manager

    extern database_manager db_mgr;

}; // namespace irods

#endif // __IRODS_DATABASE_MANAGER_HPP__




