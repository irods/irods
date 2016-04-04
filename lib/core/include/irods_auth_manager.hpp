#ifndef _AUTH_MANAGER_HPP_
#define _AUTH_MANAGER_HPP_

#include "irods_error.hpp"
#include "irods_auth_types.hpp"

namespace irods {

/// @brief String identifying this as an auth plugin interface
    const std::string AUTH_INTERFACE( "irods_auth_interface" );

    /**
     * @brief Class which manages the lifetime of auth plugins
     */
    class auth_manager {
        public:
            /// @brief Constructor
            auth_manager();

            /// @brief Copy constructor
            auth_manager( const auth_manager& _rhs );

            virtual ~auth_manager();

            /// @brief undocumented
            error resolve( const std::string& _key, auth_ptr& _value );

            /// @brief Load up a plugin corresponding to the specified type.
            error init_from_type(
                const int&,               // process type
                const std::string& _type, // type
                const std::string& _key,  // key
                const std::string& _inst, // instance name
                const std::string& _ctx,  // context
                auth_ptr& _rtn_auth       // returned plugin instance
            );

        private:
            lookup_table<auth_ptr> plugins_;
    };

/// @brief The global auth_manager instance
    extern auth_manager auth_mgr;

}; // namespace irods

#endif // _AUTH_MANAGER_HPP_
