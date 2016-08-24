/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See authResponse.h for a description of this API call.*/

// =-=-=-=-=-=-=-
#include "irods_native_auth_object.hpp"
#include "irods_auth_object.hpp"
#include "irods_auth_factory.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_constants.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_pluggable_auth_scheme.hpp"
#include "rsAuthResponse.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "authRequest.h"
#include "authResponse.h"
#include "authCheck.h"
#include "miscServerFunct.hpp"

int rsAuthResponse(
    rsComm_t*          _comm,
    authResponseInp_t* _resp ) {
    // =-=-=-=-=-=-=-
    // check our incoming params
    if ( !_comm ) {
        rodsLog( LOG_ERROR, "rsAuthRequest - null comm pointer" );
        return SYS_INVALID_INPUT_PARAM;
    }
    if ( !_resp ) {
        rodsLog( LOG_ERROR, "rsAuthRequest - null auth response pointer" );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // get the auth scheme from the singleton cache and
    // if it is not empty use that as our auth scheme
    // native is the default scheme otherwise
    irods::pluggable_auth_scheme& plug_a = irods::pluggable_auth_scheme::get_instance();
    std::string auth_scheme = plug_a.get( );
    if ( auth_scheme.empty() ) {
        auth_scheme = irods::AUTH_NATIVE_SCHEME;
    }

    // =-=-=-=-=-=-=-
    // empty out the scheme for good measure
    plug_a.set( "" );

    // =-=-=-=-=-=-=-
    // construct an auth object given the scheme
    irods::auth_object_ptr auth_obj;
    irods::error ret = irods::auth_factory( auth_scheme, &_comm->rError, auth_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve an auth plugin given the auth object
    irods::plugin_ptr ptr;
    ret = auth_obj->resolve( irods::AUTH_INTERFACE, ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }
    irods::auth_ptr auth_plugin = boost::dynamic_pointer_cast< irods::auth >( ptr );

    // =-=-=-=-=-=-=-
    // call client side init - 'establish creds'
    ret = auth_plugin->call < authResponseInp_t* > ( _comm, irods::AUTH_AGENT_AUTH_RESPONSE, auth_obj, _resp );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // win!
    return 0;


} // rsAuthResponse

int
chkProxyUserPriv( rsComm_t *rsComm, int proxyUserPriv ) {
    if ( strcmp( rsComm->proxyUser.userName, rsComm->clientUser.userName )
            == 0 ) {
        return 0;
    }

    /* remote privileged user can only do things on behalf of users from
     * the same zone */
    if ( proxyUserPriv >= LOCAL_PRIV_USER_AUTH ||
            ( proxyUserPriv >= REMOTE_PRIV_USER_AUTH &&
              strcmp( rsComm->proxyUser.rodsZone, rsComm->clientUser.rodsZone ) == 0 ) ) {
        return 0;
    }
    else {
        rodsLog( LOG_ERROR,
                 "rsAuthResponse: proxyuser %s with %d no priv to auth clientUser %s",
                 rsComm->proxyUser.userName,
                 proxyUserPriv,
                 rsComm->clientUser.userName );
        return SYS_PROXYUSER_NO_PRIV;
    }
}
