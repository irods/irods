// =-=-=-=-=-=-=-
#include "authPluginRequest.h"
#include "irods_native_auth_object.hpp"
#include "irods_auth_object.hpp"
#include "irods_auth_factory.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_constants.hpp"
#include "irods_pluggable_auth_scheme.hpp"
#include "rsAuthPluginRequest.hpp"

void _rsSetAuthRequestGetChallenge( const char* );

/// =-=-=-=-=-=-=-
/// @brief auth request api call which will delegate the
///        request to the correct plugin given the requested
///        auth scheme in the input struct
int rsAuthPluginRequest(
    rsComm_t*             _comm,
    authPluginReqInp_t*   _req_inp,
    authPluginReqOut_t**  _req_out ) {

    // =-=-=-=-=-=-=-
    // check our incoming params
    if ( !_comm ) {
        rodsLog( LOG_ERROR, "rsAuthPluginRequest - null comm pointer" );
        return SYS_INVALID_INPUT_PARAM;

    }
    else if ( !_req_inp ) {
        rodsLog( LOG_ERROR, "rsAuthPluginRequest - null input pointer" );
        return SYS_INVALID_INPUT_PARAM;

    }

    // =-=-=-=-=-=-=-
    // check the auth scheme
    std::string auth_scheme = irods::AUTH_NATIVE_SCHEME;
    if ( strlen( _req_inp->auth_scheme_ ) > 0 ) {
        auth_scheme = _req_inp->auth_scheme_;
    }

    // set the auth_scheme in the comm
    if ( _comm->auth_scheme != NULL ) {
        free( _comm->auth_scheme );
    }
    _comm->auth_scheme = strdup( auth_scheme.c_str() );
    // =-=-=-=-=-=-=-
    // store the scheme in a singleton for use in the following rsAuthResponse call
    irods::pluggable_auth_scheme& plug_a = irods::pluggable_auth_scheme::get_instance();
    plug_a.set( auth_scheme );

    // =-=-=-=-=-=-=-
    // handle old school memory allocation
    ( *_req_out ) = ( authPluginReqOut_t* )malloc( sizeof( authPluginReqOut_t ) );

    // =-=-=-=-=-=-=-
    // construct an auth object given the native scheme
    irods::auth_object_ptr auth_obj;
    irods::error ret = irods::auth_factory( auth_scheme, &_comm->rError, auth_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // set the context of the auth object for communication
    // to the plugin itself
    auth_obj->context( _req_inp->context_ );

    // =-=-=-=-=-=-=-
    // resolve an auth plugin given the auth object
    irods::plugin_ptr ptr;
    ret = auth_obj->resolve( irods::AUTH_INTERFACE, ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }
    irods::auth_ptr auth_plugin = boost::dynamic_pointer_cast <
                                  irods::auth > ( ptr );

    // =-=-=-=-=-=-=-
    // call client side init - 'establish creds'
    ret = auth_plugin->call( _comm, irods::AUTH_AGENT_AUTH_REQUEST, auth_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // send back the results
    strncpy( ( *_req_out )->result_, auth_obj->request_result().c_str(), auth_obj->request_result().size() + 1 );

    // =-=-=-=-=-=-=-
    // win!
    return 0;

} // rsAuthPluginRequest
