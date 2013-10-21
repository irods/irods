/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See authRequest.h for a description of this API call.*/

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_native_auth_object.h"
#include "eirods_auth_object.h"
#include "eirods_auth_factory.h"
#include "eirods_auth_plugin.h"
#include "eirods_auth_manager.h"
#include "eirods_auth_constants.h"

// =-=-=-=-=-=-=-
// irods includes
#include "authRequest.h"

int get64RandomBytes(char *buf);
static char buf[CHALLENGE_LEN+MAX_PASSWORD_LEN+1];

int rsAuthRequest(
    rsComm_t*          _comm, 
    authRequestOut_t** _req ) {
    // =-=-=-=-=-=-=-
    // check our incoming params
    if( !_comm ) {
        rodsLog( LOG_ERROR, "rsAuthRequest - null comm pointer" );
        return SYS_INVALID_INPUT_PARAM;
    }
    if( !_req ) {
        rodsLog( LOG_ERROR, "rsAuthRequest - null auth request pointer" );
        return SYS_INVALID_INPUT_PARAM;
    }
    // =-=-=-=-=-=-=-
    // auth scheme should be in the challenge if *_req is not null
    // if it is null then assume native auth scheme
    std::string auth_scheme = eirods::AUTH_NATIVE_SCHEME;
     
    // =-=-=-=-=-=-=-
    // support the old style memory mgmt for legacy
    // behavior - necessary for federation
    if( !(*_req ) ) { 
         (*_req) = (authRequestOut_t*)malloc( sizeof( authRequestOut_t ) );
         (*_req)->challenge = (char*)malloc( CHALLENGE_LEN+2 );
          
    } else {
        // =-=-=-=-=-=-=-
        // if the challenge pointer is valid set the scheme
        // to it - we will try to resolve that plugin
        if( (*_req)->challenge ) {
            auth_scheme = (*_req)->challenge;
        } 

    }

    // =-=-=-=-=-=-=-
    // construct an auth object given the scheme
    eirods::auth_object_ptr auth_obj;
    eirods::error ret = eirods::auth_factory( 
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
              authRequestOut_t** >( 
                  eirods::AUTH_AGENT_AUTH_REQUEST,
                  auth_obj,
                  _req );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // cache the challenge so the below function can
    // access it
    strncpy( 
        buf, 
        (*_req)->challenge, 
        CHALLENGE_LEN+1 );

    // =-=-=-=-=-=-=-
    // win!
    return 0;

} // rsAuthRequest 

// =-=-=-=-=-=-=-
// accessor for static challenge buf variable
char* _rsAuthRequestGetChallenge() {
   return ((char *)&buf);
}



