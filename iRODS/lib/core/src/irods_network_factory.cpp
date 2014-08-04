// =-=-=-=-=-=-=-
#include "irods_network_factory.hpp"
#include "irods_client_server_negotiation.hpp"

namespace irods {
// =-=-=-=-=-=-=-
// super basic free factory function to create either a tcp
// object or an ssl object based on wether ssl has been enabled
irods::error network_factory(
    rcComm_t*                   _comm,
    irods::network_object_ptr& _ptr ) {
    // =-=-=-=-=-=-=-
    // param check
    if ( !_comm ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
    }

    // currently our only criteria on the network object
    // is whether SSL has been enabled. if it has then we
    // want an ssl object which will resolve to an ssl
    // network plugin
    if ( irods::CS_NEG_USE_SSL == _comm->negotiation_results ) {
        irods::ssl_object* ssl = new irods::ssl_object( *_comm );
        if ( !ssl ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "ssl allocation failed" );
        }

        _ptr.reset( dynamic_cast< irods::network_object* >( ssl ) );

    }
    else {
        // otherwise we just need a tcp object
        irods::tcp_object* tcp = new irods::tcp_object( *_comm );
        if ( !tcp ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "tcp allocation failed" );
        }

        _ptr.reset( dynamic_cast< irods::network_object* >( tcp ) );

    }

    return SUCCESS();

} // network_factory

// =-=-=-=-=-=-=-
// version for server connection as well
irods::error network_factory(
    rsComm_t*                   _comm,
    irods::network_object_ptr& _ptr ) {

    // param check
    if ( !_comm ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
    }

    // currently our only criteria on the network object
    // is wether SSL has been enabled.  if it has then we
    // want an ssl object which will resolve to an ssl
    // network plugin
    if ( irods::CS_NEG_USE_SSL == _comm->negotiation_results ) {
        irods::ssl_object* ssl = new irods::ssl_object( *_comm );
        if ( !ssl ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "ssl allocation failed" );
        }

        _ptr.reset( ssl );

    }
    else {
        // otherwise we just need a tcp object
        irods::tcp_object* tcp = new irods::tcp_object( *_comm );
        if ( !tcp ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "tcp allocation failed" );
        }

        _ptr.reset( tcp );

    }

    return SUCCESS();

} // network_factory

}; // namespace irods
