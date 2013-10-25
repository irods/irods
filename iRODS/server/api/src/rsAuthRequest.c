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
// =-=-=-=-=-=-=-
// accessor for static challenge buf variable
char* _rsAuthRequestGetChallenge() {
   return ((char *)&buf);
}

// =-=-=-=-=-=-=-
// mutator for static challenge buf variable
void _rsSetAuthRequestGetChallenge( const char* _c ) {
    if( _c ) {
        strncpy( 
            buf, 
            _c,
            CHALLENGE_LEN+1 );
    }
}


int rsAuthRequest(
    rsComm_t*          _comm, 
    authRequestOut_t** _req ) {
    // =-=-=-=-=-=-=-
    // check our incoming params
    if( !_comm ) {
        rodsLog( LOG_ERROR, "rsAuthRequest - null comm pointer" );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // handle old school memory allocation
    (*_req) = (authRequestOut_t*)malloc( sizeof( authRequestOut_t ) );
    (*_req)->challenge = (char*)malloc( CHALLENGE_LEN+2 );
   
    // =-=-=-=-=-=-=-
    // construct an auth object given the native scheme
    eirods::auth_object_ptr auth_obj;
    eirods::error ret = eirods::auth_factory( 
                            eirods::AUTH_NATIVE_SCHEME, 
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
              rsComm_t* >( 
                  eirods::AUTH_AGENT_AUTH_REQUEST,
                  auth_obj,
                  _comm );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // send back the results
    strncpy(
        (*_req)->challenge,
        auth_obj->request_result().c_str(),
        auth_obj->request_result().size()+1 );

    // =-=-=-=-=-=-=-
    // win!
    return 0;

} // rsAuthRequest 


