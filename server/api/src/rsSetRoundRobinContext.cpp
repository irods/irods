// =-=-=-=-=-=-=-
// irods includes
#include "rcMisc.h"
#include "rodsConnect.h"
#include "apiHandler.hpp"
#include "icatHighLevelRoutines.hpp"
#include "irods_resource_manager.hpp"
#include "set_round_robin_context.h"
#include "rsSetRoundRobinContext.hpp"
#include "apiNumber.h"

extern irods::resource_manager resc_mgr;

// =-=-=-=-=-=-=-
// actual implementation of the API plugin
int rsSetRoundRobinContext(
    rsComm_t*                  _comm,
    setRoundRobinContextInp_t* _inp ) {
    rodsLog( LOG_DEBUG, "rsSetRoundRobinContex" );
    // =-=-=-=-=-=-=-
    // error check - incoming parameters
    if ( !_comm ||
         !_inp ) {
        rodsLog(
            LOG_ERROR,
            "rsSetRoundRobinContex - invalid input param" );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // error check - must have a valid context
    //               string otherwise chlModResc
    //               fails.  treat this as a
    //               success
    if ( strlen( _inp->context_ ) <= 0 ) {
        return 0;

    }

    // =-=-=-=-=-=-=-
    // error check - resource name is valid
    //               by getting a handle to it
    irods::resource_ptr resc;
    irods::error ret = resc_mgr.resolve(
        _inp->resc_name_,
        resc );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // error check - resource is in fact a
    //               round robin resource?
    std::string resc_type;
    ret = resc->get_property< std::string >(
        irods::RESOURCE_TYPE,
        resc_type );
    if ( resc_type != "roundrobin" ) {
        rodsLog(
            LOG_ERROR,
            "rsSetRoundRobinContex - resource is not of type roundrobin [%s]",
            resc_type.c_str() );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // error check - next resource is in fact a
    //               child of the resource
    if ( !resc->has_child( _inp->context_ ) ) {
        rodsLog(
            LOG_ERROR,
            "rsSetRoundRobinContex - invalid next child [%s]",
            _inp->context_ );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // get the server host struct for the icat,
    // determine if we are it.  if not redirect.
    rodsServerHost_t* server_host = 0;
    int status = getAndConnRcatHost(
        _comm,
        MASTER_RCAT,
        ( const char* )NULL,
        &server_host );
    if ( status < 0 ) {
        rodsLog(
            LOG_DEBUG,
            "rsSetRoundRobinContex - getAndConnRcatHost failed." );
        return status;
    }

    // =-=-=-=-=-=-=-
    // we are localhost to the iCAT server
    if ( server_host->localFlag == LOCAL_HOST ) {
        // =-=-=-=-=-=-=-
        // lie to the icat api just a little bit...
        int client_user_auth = _comm->clientUser.authInfo.authFlag;
        int proxy_user_auth  = _comm->proxyUser.authInfo.authFlag;
        _comm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
        _comm->proxyUser.authInfo.authFlag  = LOCAL_PRIV_USER_AUTH;
        status = chlModResc(
            _comm,
            _inp->resc_name_,
            "context",
            _inp->context_ );
        _comm->clientUser.authInfo.authFlag = client_user_auth;
        _comm->proxyUser.authInfo.authFlag  = proxy_user_auth;
    }
    else {
        // =-=-=-=-=-=-=-
        // redirect to the appropriate server
        status = procApiRequest(
            server_host->conn,
            SET_RR_CTX_AN,
            _inp,
            NULL,
            ( void** ) NULL,
            NULL );
        if ( status < 0 ) {
            replErrorStack(
                server_host->conn->rError,
                &_comm->rError );
        }

    } // else

    // =-=-=-=-=-=-=-
    // bad, bad things have happened.
    if ( status < 0 ) {
        rodsLog(
            LOG_NOTICE,
            "rsSetRoundRobinContex: error %d",
            status );
    }

    return status;

} // rsSetRoundRobinContext
