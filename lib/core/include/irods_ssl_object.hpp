#ifndef __IRODS_SSL_OBJECT_HPP__
#define __IRODS_SSL_OBJECT_HPP__

// =-=-=-=-=-=-=-
#include "irods_network_object.hpp"
#include "irods_buffer_encryption.hpp"

// =-=-=-=-=-=-=-
// ssl includes
#include "openssl/ssl.h"

namespace irods {

// =-=-=-=-=-=-=-
// constant key for tcp network object
    const std::string SSL_NETWORK_PLUGIN( "ssl" );

// =-=-=-=-=-=-=-
// SSL Network Object
    class ssl_object : public network_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            ssl_object();
            ssl_object( const rcComm_t& );
            ssl_object( const rsComm_t& );
            ssl_object( const ssl_object& );

            // =-=-=-=-=-=-=-
            // Destructors
            virtual ~ssl_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual ssl_object& operator=( const ssl_object& );

            /// @brief Comparison operator
            virtual bool operator==( const ssl_object& _rhs ) const;

            // =-=-=-=-=-=-=-
            // plugin resolution operation
            virtual error resolve(
                const std::string&, // plugin interface name
                plugin_ptr& );      // resolved plugin

            // =-=-=-=-=-=-=-
            // convertion to client comm ptr
            virtual error to_client( rcComm_t* );

            // =-=-=-=-=-=-=-
            // convertion to client comm ptr
            virtual error to_server( rsComm_t* );

            // =-=-=-=-=-=-=-
            // accessor for rule engine variables
            virtual error get_re_vars( rule_engine_vars_t& );

            // =-=-=-=-=-=-=-
            // Accessors
            virtual SSL_CTX*              ssl_ctx()       const {
                return ssl_ctx_;
            }
            virtual SSL*                  ssl()           const {
                return ssl_;
            }
            virtual std::string           host()          const {
                return host_;
            }
            virtual buffer_crypt::array_t shared_secret() const {
                return shared_secret_;
            }

            virtual int         key_size()             const {
                return key_size_;
            }
            virtual int         salt_size()            const {
                return salt_size_;
            }
            virtual int         num_hash_rounds()      const {
                return num_hash_rounds_;
            }
            virtual std::string encryption_algorithm() const {
                return encryption_algorithm_;
            }

            // =-=-=-=-=-=-=-
            // mutators
            virtual void ssl_ctx( SSL_CTX* _c )                 {
                ssl_ctx_       = _c;
            }
            virtual void ssl( SSL* _s )                         {
                ssl_           = _s;
            }
            virtual void host( const std::string& _h )          {
                host_          = _h;
            }
            virtual void shared_secret( const buffer_crypt::array_t& _s ) {
                shared_secret_ = _s;
            }

            virtual void key_size( int _s )                            {
                key_size_             = _s;
            }
            virtual void salt_size( int _s )                           {
                salt_size_            = _s;
            }
            virtual void num_hash_rounds( int _h )                     {
                num_hash_rounds_      = _h;
            }
            virtual void encryption_algorithm( const std::string& _a ) {
                encryption_algorithm_ = _a;
            }

        private:
            SSL_CTX*              ssl_ctx_;
            SSL*                  ssl_;
            std::string           host_;
            buffer_crypt::array_t shared_secret_;

            int         key_size_;
            int         salt_size_;
            int         num_hash_rounds_;
            std::string encryption_algorithm_;

    }; // class ssl_object

/// =-=-=-=-=-=-=-
/// @brief typedef for shared tcp object ptr
    typedef boost::shared_ptr< ssl_object > ssl_object_ptr;

}; // namespace irods

#endif // __IRODS_SSL_OBJECT_HPP__
