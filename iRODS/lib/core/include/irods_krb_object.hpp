#ifndef _KRB_AUTH_OBJECT_HPP_
#define _KRB_AUTH_OBJECT_HPP_

#include "irods_error.hpp"
#include "irods_auth_object.hpp"
#include "irods_stacktrace.hpp"

#include <gssapi.h>

namespace irods {

// constant key for krb auth object
    const std::string KRB_AUTH_PLUGIN( "KRB" );
    const std::string AUTH_KRB_SCHEME( "krb" );

    /**
     * @brief Auth object for KRB authentication
     */
    class krb_auth_object : public auth_object {
        public:
            /// @brief Constructor
            krb_auth_object( rError_t* _r_error );
            virtual ~krb_auth_object();

            // Accessors

            /// @brief Returns the KRB credentials
            virtual gss_cred_id_t creds( void ) const {
                return creds_;
            }

            /// @brief Returns the socket number
            virtual int sock( void ) const {
                return sock_;
            }

            /// @brief Returns the serverDN
            virtual const std::string& service_name( void ) const {
                return service_name_;
            }

            /// @brief Returns the digest
            virtual const std::string& digest( void ) const {
                return digest_;
            }

            /// =-=-=-=-=-=-=-
            /// @brief serialize object to key-value pairs
            virtual error get_re_vars( rule_engine_vars_t& );

            // Mutators

            /// @brief Sets the KRB credentials
            virtual void creds( gss_cred_id_t _creds ) {
                creds_ = _creds;
            }

            /// @brief Sets the socket number
            virtual void sock( int s ) {
                sock_ = s;
            }

            /// @brief Sets the serverDN
            virtual void service_name( const std::string& s ) {
                service_name_ = s;
            }

            /// @brief Sets the digest
            virtual void digest( const std::string& d ) {
                digest_ = d;
            }

            // Methods

            /// @brief undocumented
            error resolve( const std::string& _name, plugin_ptr& _plugin ); // resolve plugin

            /// @brief Comparison operator
            bool operator==( const krb_auth_object& _rhs ) const;


        private:
            gss_cred_id_t creds_;
            int sock_;
            std::string service_name_;
            std::string digest_;
    };

    typedef boost::shared_ptr<krb_auth_object> krb_auth_object_ptr;

}; // namespace irods

#endif // _KRB_AUTH_OBJECT_HPP_
