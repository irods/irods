#ifndef _IRODS_REPL_RETRY_HPP_
#define _IRODS_REPL_RETRY_HPP_

#include "irods/dataObjInpOut.h"
#include "irods/irods_plugin_context.hpp"
#include <string>

namespace irods {
    // throws irods::exception
    int data_obj_repl_with_retry(
        irods::plugin_context& _ctx,
        dataObjInp_t& dataObjInp );

    const std::string RETRY_ATTEMPTS_KW{ "retry_attempts" };
    const std::string RETRY_FIRST_DELAY_IN_SECONDS_KW{ "first_retry_delay_in_seconds" };
    const std::string RETRY_BACKOFF_MULTIPLIER_KW{ "backoff_multiplier" };

    const uint32_t DEFAULT_RETRY_ATTEMPTS{ 1 };
    const uint32_t DEFAULT_RETRY_FIRST_DELAY_IN_SECONDS{ 1 };
    const double DEFAULT_RETRY_BACKOFF_MULTIPLIER{ 1.0f };
}

#endif // _IRODS_REPL_RETRY_HPP_
