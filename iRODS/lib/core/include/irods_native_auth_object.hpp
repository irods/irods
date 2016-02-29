#ifndef __NATIVE_AUTH_OBJECT_HPP__
#define __NATIVE_AUTH_OBJECT_HPP__

#include "irods_error.hpp"
#include "irods_auth_object.hpp"

#include "rcConnect.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {

/// =-=-=-=-=-=-=-
/// @brief constant defining the native auth scheme string
    const std::string AUTH_NATIVE_SCHEME( "native" );

/// =-=-=-=-=-=-=-
/// @brief object for a native irods authenticaion sceheme
    class native_auth_object : public auth_object {
        public:
            /// =-=-=-=-=-=-=-
            /// @brief Ctor
            native_auth_object( rError_t* _r_error );
            native_auth_object( const native_auth_object& );
            virtual ~native_auth_object();

            /// =-=-=-=-=-=-=-
            /// @brief assignment operator
            virtual native_auth_object&  operator=( const native_auth_object& );

            /// =-=-=-=-=-=-=-
            /// @brief Comparison operator
            virtual bool operator==( const native_auth_object& ) const;

            /// =-=-=-=-=-=-=-
            /// @brief Plugin resolution operation
            virtual error resolve(
                const std::string&, // interface for which to resolve
                plugin_ptr& );      // ptr to resolved plugin

            /// =-=-=-=-=-=-=-
            /// @brief serialize object to key-value pairs
            virtual error get_re_vars( rule_engine_vars_t& );

            /// =-=-=-=-=-=-=-
            /// @brief accessors
            std::string digest() const {
                return digest_;
            }

            /// =-=-=-=-=-=-=-
            /// @brief mutators
            void digest( const std::string& _dd ) {
                digest_ = _dd;
            }

        private:
            /// =-=-=-=-=-=-=-
            /// @brief md5 digest computed
            std::string digest_;

    }; // class native_auth_object

/// @brief Helpful typedef
    typedef boost::shared_ptr<native_auth_object> native_auth_object_ptr;

}; // namespace irods

#endif // __NATIVE_AUTH_OBJECT_HPP__
