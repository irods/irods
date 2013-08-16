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
        const rcComm_t& _comm ) : 
        network_object( _comm ) {

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
    // public - assignment operator
    bool tcp_object::operator==( 
        const tcp_object& _rhs ) const {
        return network_object::operator==( _rhs );

    } // operator=

    // =-=-=-=-=-=-=-
    // public - resolver for resource_manager
    error tcp_object::resolve( 
        resource_manager&, 
        resource_ptr& ) {
        return ERROR( 
                   SYS_INVALID_INPUT_PARAM, 
                   "object does not support resource_manager" );     
    } // resolve
    
    // =-=-=-=-=-=-=-
    // public - resolver for tcp_manager
    error tcp_object::resolve( 
        network_manager& _mgr,  
        network_ptr&     _ptr ) {
        // =-=-=-=-=-=-=-
        // ask the network manager for a tcp resource
        error ret = _mgr.resolve( TCP_NETWORK_PLUGIN, _ptr );
        if( !ret.ok() ) {
            // =-=-=-=-=-=-=-
            // attempt to load the plugin, in this case the type,
            // instance name, key etc are all tcp as there is only
            // the need for one instance of a tcp object, etc.
            std::string empty_context( "" );
            ret = _mgr.init_from_type( 
                      TCP_NETWORK_PLUGIN,
                      TCP_NETWORK_PLUGIN,
                      TCP_NETWORK_PLUGIN,
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
    error tcp_object::get_re_vars( 
        keyValPair_t& _kvp ) {
        return network_object::get_re_vars( _kvp );

    } // get_re_vars
 
}; // namespace eirods



