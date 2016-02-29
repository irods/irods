#ifndef __PAM_AUTH_OBJECT_HPP__
#define __PAM_AUTH_OBJECT_HPP__

#include "irods_error.hpp"
#include "irods_auth_object.hpp"

#include "rcConnect.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {

/// =-=-=-=-=-=-=-
/// @brief constant defining the native auth scheme string
    const std::string AUTH_PAM_SCHEME( "pam" );

/// =-=-=-=-=-=-=-
/// @brief object for a native irods authenticaion sceheme
    class pam_auth_object : public auth_object {
        public:
            /// =-=-=-=-=-=-=-
            /// @brief Ctor
            pam_auth_object( rError_t* _r_error );
            virtual ~pam_auth_object();
            pam_auth_object( const pam_auth_object& );

            /// =-=-=-=-=-=-=-
            /// @brief assignment operator
            virtual pam_auth_object&  operator=( const pam_auth_object& );

            /// =-=-=-=-=-=-=-
            /// @brief Comparison operator
            virtual bool operator==( const pam_auth_object& ) const;

            /// =-=-=-=-=-=-=-
            /// @brief Plugin resolution operation
            virtual error resolve(
                const std::string&, // interface for which to resolve
                plugin_ptr& );      // ptr to resolved plugin

            /// =-=-=-=-=-=-=-
            /// @brief serialize object to key-value pairs
            virtual error get_re_vars( rule_engine_vars_t& );

        private:

    }; // class pam_auth_object

/// @brief Helpful typedef
    typedef boost::shared_ptr<pam_auth_object> pam_auth_object_ptr;

}; // namespace irods

#endif // __PAM_AUTH_OBJECT_HPP__
