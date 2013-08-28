


#ifndef __EIRODS_SSL_OBJECT_H__
#define __EIRODS_SSL_OBJECT_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_network_object.h"

// =-=-=-=-=-=-=-
// ssl includes
#include "ssl.h"

namespace eirods {

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
        virtual error resolve( resource_manager&, resource_ptr& );
        virtual error resolve( network_manager&,  network_ptr&  );
  
        // =-=-=-=-=-=-=-
        // convertion to client comm ptr
        virtual error to_client( rcComm_t* );
         
        // =-=-=-=-=-=-=-
        // convertion to client comm ptr
        virtual error to_server( rsComm_t* );

        // =-=-=-=-=-=-=-
        // accessor for rule engine variables
        virtual error get_re_vars( keyValPair_t& );

        // =-=-=-=-=-=-=-
        // Accessors
        virtual SSL_CTX*    ssl_ctx() { return ssl_ctx_; }
        virtual SSL*        ssl()     { return ssl_;     }
        virtual std::string host()    { return host_;    }

        // =-=-=-=-=-=-=-
        // mutators
        virtual void ssl_ctx( SSL_CTX* _c )        { ssl_ctx_ = _c; }
        virtual void ssl( SSL* _s )                { ssl_     = _s; }
        virtual void host( const std::string& _h ) { host_    = _h; }

    private:
        SSL_CTX*    ssl_ctx_;
        SSL*        ssl_;
        std::string host_;        
    }; // class ssl_object

    /// =-=-=-=-=-=-=-
    /// @brief typedef for shared tcp object ptr
    typedef boost::shared_ptr< ssl_object > ssl_object_ptr;

}; // namespace eirods

#endif // __EIRODS_SSL_OBJECT_H__
