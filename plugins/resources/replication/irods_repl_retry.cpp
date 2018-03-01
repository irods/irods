#include "irods_repl_retry.hpp"
#include "irods_repl_types.hpp"
#include "dataObjRepl.h"

#include <boost/numeric/conversion/cast.hpp>
#include <string>

// Replicates a data object and verifies that the bits are correct
// Retry mechanism triggers based on config in repl context string
int irods::data_obj_repl_with_retry(
        irods::resource_plugin_context& _ctx,
        dataObjInp_t& dataObjInp ) {

    transferStat_t* trans_stat = NULL;
    int status = rsDataObjRepl( _ctx.comm(), &dataObjInp, &trans_stat );
    if ( 0 == status ) {
        free( trans_stat );
        return status;
    }

    // Throw in the event that repl resource retry settings not set
    uint32_t retry_attempts = irods::DEFAULT_RETRY_ATTEMPTS;
    uint32_t delay_in_seconds = irods::DEFAULT_RETRY_FIRST_DELAY_IN_SECONDS;
    double backoff_multiplier = irods::DEFAULT_RETRY_BACKOFF_MULTIPLIER;
    irods::error err = _ctx.prop_map().get< uint32_t >
                          ( irods::RETRY_ATTEMPTS_KW, retry_attempts );
    if ( !err.ok() ) {
        THROW( err.code(), err.result() );
    }
    err = _ctx.prop_map().get< uint32_t >
              ( irods::RETRY_FIRST_DELAY_IN_SECONDS_KW, delay_in_seconds );
    if ( !err.ok() ) {
        THROW( err.code(), err.result() );
    }
    err = _ctx.prop_map().get< double >
              ( irods::RETRY_BACKOFF_MULTIPLIER_KW, backoff_multiplier );
    if ( !err.ok() ) {
        THROW( err.code(), err.result() );
    }

    // Keep retrying until success or there are no more attempts left
    try {
        while ( status < 0 && retry_attempts-- > 0 ) {
            sleep( delay_in_seconds );
            status = rsDataObjRepl( _ctx.comm(), &dataObjInp, &trans_stat );
            if ( status < 0 && retry_attempts > 0 ) {
                delay_in_seconds = boost::numeric_cast< uint32_t >
                                    ( delay_in_seconds * backoff_multiplier );
            }
        }
    }
    catch( const boost::bad_numeric_cast& e ) {
        // Indicates that delay value is too large to store,
        // so we should stop retrying (2^32 seconds > 136 years)
        irods::error err = ERROR( USER_INPUT_OPTION_ERR, e.what() );
        irods::log( err );
    }

    free( trans_stat );
    return status;
}
