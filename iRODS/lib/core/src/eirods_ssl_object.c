/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_ssl_object.h"
#include "eirods_network_manager.h"

namespace eirods {
    // =-=-=-=-=-=-=-
    // public - ctor
    ssl_object::ssl_object() :
        network_object(),
        ssl_ctx_(0),
        ssl_(0) {

    } // ctor

    // =-=-=-=-=-=-=-
    // public - ctor
    ssl_object::ssl_object(
        const rcComm_t& _comm ) : 
        network_object( _comm ),
        ssl_ctx_( _comm.ssl_ctx ), 
        ssl_( _comm.ssl ),
        host_( _comm.host ) {
    } // ctor

    // =-=-=-=-=-=-=-
    // public - ctor
    ssl_object::ssl_object(
        const rsComm_t& _comm ) : 
        network_object( _comm ),
        ssl_ctx_( _comm.ssl_ctx ), 
        ssl_( _comm.ssl ),
        host_( "" ) {
    } // ctor

    // =-=-=-=-=-=-=-
    // public - cctor
    ssl_object::ssl_object( 
        const ssl_object& _rhs ) : 
        network_object( _rhs ) {
        ssl_ctx_ = _rhs.ssl_ctx_;
        ssl_     = _rhs.ssl_;

    } // cctor

    // =-=-=-=-=-=-=-
    // public - dtor
    ssl_object::~ssl_object() {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    ssl_object& ssl_object::operator=( 
        const ssl_object& _rhs ) {
        network_object::operator=( _rhs );
        ssl_ctx_ = _rhs.ssl_ctx_;
        ssl_     = _rhs.ssl_;

        return *this;

    } // operator=
 
    // =-=-=-=-=-=-=-
    // public - assignment operator
    bool ssl_object::operator==( 
        const ssl_object& _rhs ) const {
        bool ret = network_object::operator==( _rhs );
        ret &= ( ssl_ctx_ == _rhs.ssl_ctx_ );
        ret &= ( ssl_     == _rhs.ssl_     );

        return ret;

    } // operator==
 
    // =-=-=-=-=-=-=-
    // public - resolver for resource_manager
    error ssl_object::resolve( 
        resource_manager&, 
        resource_ptr& ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, 
                      "object does not support resource_manager" );     
    } // resolve
    
    // =-=-=-=-=-=-=-
    // public - resolver for ssl_manager
    error ssl_object::resolve( 
        network_manager& _mgr,  
        network_ptr&     _ptr ) {
        // =-=-=-=-=-=-=-
        // ask the network manager for a SSL resource
        error ret = _mgr.resolve( SSL_NETWORK_PLUGIN, _ptr );
        if( !ret.ok() ) {
            // =-=-=-=-=-=-=-
            // attempt to load the plugin, in this case the type,
            // instance name, key etc are all tcp as there is only
            // the need for one instance of a tcp object, etc.
            std::string empty_context( "" );
            ret = _mgr.init_from_type( 
                      SSL_NETWORK_PLUGIN,
                      SSL_NETWORK_PLUGIN,
                      SSL_NETWORK_PLUGIN,
                      empty_context,
                      _ptr );
            if( !ret.ok() ) {
                return PASS( ret );

            } else {
                return SUCCESS();

            }

        } // if !ok


        return SUCCESS();
    } // resolve
        
    // =-=-=-=-=-=-=-
    // accessor for rule engine variables
    error ssl_object::get_re_vars( 
        keyValPair_t& _kvp ) {
        network_object::get_re_vars( _kvp );

        return SUCCESS();

    } // get_re_vars
 
    // =-=-=-=-=-=-=-
    // convertion to client comm ptr
    error ssl_object::to_client( rcComm_t* _comm ) {
        if( !_comm ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
        }
        
        network_object::to_client( _comm );

        _comm->ssl     = ssl_;
        _comm->ssl_ctx = ssl_ctx_;
        
        return SUCCESS();

    } // to_client
     
    // =-=-=-=-=-=-=-
    // convertion to client comm ptr
    error ssl_object::to_server( rsComm_t* _comm ) {
        if( !_comm ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
        }

        network_object::to_server( _comm );

        _comm->ssl     = ssl_;
        _comm->ssl_ctx = ssl_ctx_;
        
        return SUCCESS();

    } // to_server


}; // namespace eirods



