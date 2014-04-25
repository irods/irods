

// =-=-=-=-=-=-=-
// irods includes
#include "reGlobalsExtern.hpp"
#include "icatHighLevelRoutines.hpp"
#include "rs_set_round_robin_context.hpp"
#include "irods_resource_manager.hpp"

extern irods::resource_manager resc_mgr;

// =-=-=-=-=-=-=-
// actual implementation of the API plugin
int set_round_robin_context(
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
                     NULL,
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
#ifdef RODS_CAT
        status = chlModResc(
                     _inp->resc_name_,
                     _inp->context_,
                     "context" );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        // =-=-=-=-=-=-=-
        // redirect to the appropriate server
        status = rcSetRoundRobinContex(
                     server_host->conn,
                     _inp );
        if ( status < 0 ) {
            replErrorStack(
                server_host->conn->rError,
                &_comm->rError );
        }
    }

    // =-=-=-=-=-=-=-
    // bad, bad things have happened.
    if ( status < 0 ) {
        rodsLog(
            LOG_NOTICE,
            "rsSetRoundRobinContex: error %d",
            status );
    }

    return status;

} // set_round_robin_context



