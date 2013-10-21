/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See authResponse.h for a description of this API call.*/

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_native_auth_object.h"
#include "eirods_auth_object.h"
#include "eirods_auth_factory.h"
#include "eirods_auth_plugin.h"
#include "eirods_auth_manager.h"
#include "eirods_auth_constants.h"
#include "eirods_kvp_string_parser.h"

// =-=-=-=-=-=-=-
// irods includes
#include "authRequest.h"
#include "authResponse.h"
#include "authCheck.h"
#include "miscServerFunct.h"

int rsAuthResponse( 
    rsComm_t*          _comm, 
    authResponseInp_t* _resp ) {
    // =-=-=-=-=-=-=-
    // check our incoming params
    if( !_comm ) {
        rodsLog( LOG_ERROR, "rsAuthRequest - null comm pointer" );
        return SYS_INVALID_INPUT_PARAM;
    }
    if( !_resp ) {
        rodsLog( LOG_ERROR, "rsAuthRequest - null auth response pointer" );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // auth scheme should be in the username buffer as a
    // key-value pair
    std::string auth_scheme = eirods::AUTH_NATIVE_SCHEME;
     
    // =-=-=-=-=-=-=-
    // parse the response string, if it has the keys
    // and values then we set the scheme and repave
    // the response string
    eirods::kvp_map_t kvp;
    eirods::error ret = eirods::parse_kvp_string( 
                            _resp->username,
                            kvp );
    if( ( kvp.find( eirods::AUTH_USER_KEY )   != kvp.end() ) &&
        ( kvp.find( eirods::AUTH_SCHEME_KEY ) != kvp.end() ) ) {
        // =-=-=-=-=-=-=-
        // keys are found so cache them and repave
        // the username
        auth_scheme          = kvp[ eirods::AUTH_SCHEME_KEY ];
        std::string username = kvp[ eirods::AUTH_USER_KEY ];
        strncpy( 
            _resp->username,
            username.c_str(),
            LONG_NAME_LEN ); 
    }

    // =-=-=-=-=-=-=-
    // construct an auth object given the scheme
    eirods::auth_object_ptr auth_obj;
    ret = eirods::auth_factory( 
              auth_scheme, 
              &_comm->rError,
              auth_obj );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve an auth plugin given the auth object
    eirods::plugin_ptr ptr;
    ret = auth_obj->resolve( 
              eirods::AUTH_INTERFACE,
              ptr );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        return ret.code();
    }
    eirods::auth_ptr auth_plugin = boost::dynamic_pointer_cast< eirods::auth >( ptr );

    // =-=-=-=-=-=-=-
    // call client side init - 'establish creds'
    ret = auth_plugin->call< 
              rsComm_t*,
              authResponseInp_t* >( 
                  eirods::AUTH_AGENT_AUTH_RESPONSE,
                  auth_obj,
                  _comm,
                  _resp );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // win!
    return 0;


} // rsAuthResponse 

int
chkProxyUserPriv (rsComm_t *rsComm, int proxyUserPriv)
{
    if (strcmp (rsComm->proxyUser.userName, rsComm->clientUser.userName) 
      == 0) return 0;

    /* remote privileged user can only do things on behalf of users from
     * the same zone */
    if (proxyUserPriv >= LOCAL_PRIV_USER_AUTH ||
      (proxyUserPriv >= REMOTE_PRIV_USER_AUTH &&
      strcmp (rsComm->proxyUser.rodsZone,rsComm->clientUser.rodsZone) == 0)) {
	return 0;
    } else {
        rodsLog (LOG_ERROR,
         "rsAuthResponse: proxyuser %s with %d no priv to auth clientUser %s",
             rsComm->proxyUser.userName,
             proxyUserPriv,
             rsComm->clientUser.userName);
         return (SYS_PROXYUSER_NO_PRIV);
    }
}

