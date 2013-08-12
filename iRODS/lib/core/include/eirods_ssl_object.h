


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
        // accessor for rule engine variables
        virtual error get_re_vars( keyValPair_t& );

        // =-=-=-=-=-=-=-
        // Accessors
        virtual SSL_CTX* ssl_ctx() { return ssl_ctx_; }
        virtual SSL*     ssl()     { return ssl_;     }

    private:
        SSL_CTX* ssl_ctx_;
        SSL*     ssl_;
        
    }; // class ssl_object

}; // namespace eirods

#endif // __EIRODS_SSL_OBJECT_H__
