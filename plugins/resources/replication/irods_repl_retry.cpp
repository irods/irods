#include "irods_repl_retry.hpp"
#include "irods_repl_types.hpp"
#include "rsDataObjRepl.hpp"

#include <boost/numeric/conversion/cast.hpp>
#include <chrono>
#include <string>
#include <thread>

// Replicates a data object and verifies that the bits are correct
// Retry mechanism triggers based on config in repl context string
int irods::data_obj_repl_with_retry(
        irods::plugin_context& _ctx,
        dataObjInp_t& dataObjInp ) {

    transferStat_t* trans_stat{ nullptr };
    rmKeyVal(&dataObjInp.condInput, ALL_KW);
    auto status{ rsDataObjRepl( _ctx.comm(), &dataObjInp, &trans_stat ) };
    if ( 0 == status ) {
        irods::log(LOG_DEBUG8, fmt::format("[{}:{}] - replication succeeded", __FUNCTION__, __LINE__));
        free( trans_stat );
        return status;
    }
    else if (SYS_NOT_ALLOWED == status) {
        const auto error = irods::pop_error_message(_ctx.comm()->rError);
        irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]",
            __FUNCTION__, __LINE__, error));
        return status;
    }

    // Throw in the event that repl resource retry settings not set
    auto retry_attempts{ irods::DEFAULT_RETRY_ATTEMPTS };
    auto delay_in_seconds{ irods::DEFAULT_RETRY_FIRST_DELAY_IN_SECONDS };
    auto backoff_multiplier{ irods::DEFAULT_RETRY_BACKOFF_MULTIPLIER };
    irods::error err{ _ctx.prop_map().get< decltype( retry_attempts ) >
                          ( irods::RETRY_ATTEMPTS_KW, retry_attempts ) };
    if ( !err.ok() ) {
        THROW( err.code(), err.result() );
    }
    err = _ctx.prop_map().get< decltype( delay_in_seconds ) >
              ( irods::RETRY_FIRST_DELAY_IN_SECONDS_KW, delay_in_seconds );
    if ( !err.ok() ) {
        THROW( err.code(), err.result() );
    }
    err = _ctx.prop_map().get< decltype( backoff_multiplier ) >
              ( irods::RETRY_BACKOFF_MULTIPLIER_KW, backoff_multiplier );
    if ( !err.ok() ) {
        THROW( err.code(), err.result() );
    }

    irods::log(LOG_DEBUG, fmt::format(
        "[{}:{}] - replication failed, retrying...attempts:[{}],delay:[{}],backoff[{}]",
        __FUNCTION__, __LINE__, retry_attempts, delay_in_seconds, backoff_multiplier));

    // Keep retrying until success or there are no more attempts left
    try {
        while ( status < 0 && retry_attempts-- > 0 ) {
            irods::log(LOG_DEBUG, fmt::format("[{}:{}] - retries remaining:[{}]", __FUNCTION__, __LINE__, retry_attempts));
            std::this_thread::sleep_for( std::chrono::seconds( delay_in_seconds ) );
            status = rsDataObjRepl( _ctx.comm(), &dataObjInp, &trans_stat );
            if ( status < 0 && retry_attempts > 0 ) {
                delay_in_seconds = boost::numeric_cast< decltype( delay_in_seconds ) >
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
