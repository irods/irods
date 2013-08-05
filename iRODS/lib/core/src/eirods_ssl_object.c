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
        const rsComm_t& _comm ) : 
        network_object( _comm ),
        ssl_ctx_( _comm.ssl_ctx ), 
        ssl_( _comm.ssl ) {
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
        return SUCCESS();
    } // resolve
        
    // =-=-=-=-=-=-=-
    // accessor for rule engine variables
    error ssl_object::get_re_vars( 
        keyValPair_t& _kvp ) {
        network_object::get_re_vars( _kvp );

        return SUCCESS();

    } // get_re_vars
 
}; // namespace eirods



