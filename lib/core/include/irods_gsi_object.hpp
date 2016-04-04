#ifndef _GSI_AUTH_OBJECT_HPP_
#define _GSI_AUTH_OBJECT_HPP_

#include "irods_error.hpp"
#include "irods_auth_object.hpp"

#include <gssapi.h>

namespace irods {

// constant key for gsi auth object
    const std::string GSI_AUTH_PLUGIN( "GSI" );
    const std::string AUTH_GSI_SCHEME( "gsi" );

    /**
     * @brief Auth object for GSI authentication
     */
    class gsi_auth_object : public auth_object {
        public:
            /// @brief Constructor
            gsi_auth_object( rError_t* _r_error );
            virtual ~gsi_auth_object();

            // Accessors

            /// @brief Returns the GSI credentials
            virtual gss_cred_id_t creds( void ) const {
                return creds_;
            }

            /// @brief Returns the socket number
            virtual int sock( void ) const {
                return sock_;
            }

            /// @brief Returns the serverDN
            virtual const std::string& server_dn( void ) const {
                return server_dn_;
            }

            /// @brief Returns the digest
            virtual const std::string& digest( void ) const {
                return digest_;
            }

            /// =-=-=-=-=-=-=-
            /// @brief serialize object to key-value pairs
            virtual error get_re_vars( rule_engine_vars_t& );

            // Mutators

            /// @brief Sets the GSI credentials
            virtual void creds( gss_cred_id_t _creds ) {
                creds_ = _creds;
            }

            /// @brief Sets the socket number
            virtual void sock( int s ) {
                sock_ = s;
            }

            /// @brief Sets the serverDN
            virtual void server_dn( const std::string& s ) {
                server_dn_ = s;
            }

            /// @brief Sets the digest
            virtual void digest( const std::string& d ) {
                digest_ = d;
            }

            // Methods

            /// @brief undocumented
            error resolve( const std::string& _name, plugin_ptr& _plugin ); // resolve plugin

            /// @brief Comparison operator
            bool operator==( const gsi_auth_object& _rhs ) const;


        private:
            gss_cred_id_t creds_;
            int sock_;
            std::string server_dn_;
            std::string digest_;
    };

    typedef boost::shared_ptr<gsi_auth_object> gsi_auth_object_ptr;

}; // namespace irods

#endif // _GSI_AUTH_OBJECT_HPP_
