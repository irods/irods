/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_tcp_object.h"
#include "eirods_network_manager.h"

namespace eirods {
    // =-=-=-=-=-=-=-
    // public - ctor
    tcp_object::tcp_object() :
        network_object() {

    } // ctor

    // =-=-=-=-=-=-=-
    // public - ctor
    tcp_object::tcp_object(
        const rsComm_t& _comm ) : 
        network_object( _comm ) {

    } // ctor

    // =-=-=-=-=-=-=-
    // public - cctor
    tcp_object::tcp_object( 
        const tcp_object& _rhs ) :
        network_object( _rhs ) {

    } // cctor

    // =-=-=-=-=-=-=-
    // public - dtor
    tcp_object::~tcp_object() {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    tcp_object& tcp_object::operator=( 
        const tcp_object& _rhs ) {
        network_object::operator=( _rhs );

        return *this;

    } // operator=

    // =-=-=-=-=-=-=-
    // public - resolver for resource_manager
    error tcp_object::resolve( 
        resource_manager&, 
        resource_ptr& ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, 
                      "object does not support resource_manager" );     
    } // resolve
    
    // =-=-=-=-=-=-=-
    // public - resolver for tcp_manager
    error tcp_object::resolve( 
        network_manager& _mgr,  
        network_ptr&     _ptr ) {

        return SUCCESS();
    } // resolve
        
    // =-=-=-=-=-=-=-
    // accessor for rule engine variables
    error tcp_object::get_re_vars( 
        keyValPair_t& _kvp ) {
        network_object::get_re_vars( _kvp );

        return SUCCESS();

    } // get_re_vars
 
}; // namespace eirods



