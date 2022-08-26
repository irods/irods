/// \file

#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/rodsErrorTable.h"
#include "irods/rs_atomic_apply_metadata_operations.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"

#include <functional>
#include <string>
#include <exception>

namespace
{
    namespace log = irods::experimental::log;

    auto to_string(msParam_t& _p) -> const char*
    {
        const auto* s = parseMspForStr(&_p);

        if (!s) {
            THROW(SYS_INVALID_INPUT_PARAM, "Failed to convert microservice argument to string.");
        }

        return s;
    }

    auto msi_impl(msParam_t* _json_input, msParam_t* _json_output, ruleExecInfo_t* _rei) -> int
    {
        if (!_json_input || !_json_output) {
            log::microservice::error("Invalid input argument.");
            return SYS_INVALID_INPUT_PARAM;
        }

        try {
            const auto* json_input = to_string(*_json_input);
            char* json_output{};

            if (const auto ec = rs_atomic_apply_metadata_operations(_rei->rsComm, json_input, &json_output); ec != 0) {
                // clang-format off
                log::microservice::error({{"log_message", "Error updating metadata."},
                                          {"error_code", std::to_string(ec)}});
                // clang-format on
                return ec;
            }

            fillStrInMsParam(_json_output, json_output);

            return 0;
        }
        catch (const irods::exception& e) {
            // clang-format off
            log::microservice::error({{"log_message", e.what()},
                                      {"error_code", std::to_string(e.code())}});
            // clang-format on
            return e.code();
        }
        catch (const std::exception& e) {
            log::microservice::error(e.what());
            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            log::microservice::error("An unknown error occurred while processing the request.");
            return SYS_UNKNOWN_ERROR;
        }
    }

    template <typename... Args, typename Function>
    auto make_msi(const std::string& _name, Function _func) -> irods::ms_table_entry*
    {
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation(_name, std::function<int(Args..., ruleExecInfo_t*)>(_func));
        return msi;
    }
} // anonymous namespace

extern "C"
auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<msParam_t*, msParam_t*>("msi_atomic_apply_metadata_operations", msi_impl);
}

#ifdef IRODS_FOR_DOXYGEN
/// \brief Executes a list of metadata operations on a single object atomically.
///
/// Sequentially executes all \p operations on \p entity_name as a single transaction. If an
/// error occurs, all updates are rolled back and an error is returned. \p _json_output will
/// contain specific information about the error.
///
/// \p _json_input must have the following JSON structure:
/// \code{.js}
/// {
///   "entity_name": string,
///   "entity_type": string,
///   "operations": [
///     {
///       "operation": string,
///       "attribute": string,
///       "value": string,
///       "units": string
///     }
///   ]
/// }
/// \endcode
///
/// \p entity_name must be one of the following:
/// - A logical path pointing to a data object.
/// - A logical path pointing to a collection.
/// - A user name.
/// - A resource name.
///
/// \p entity_type must be one of the following:
/// - collection
/// - data_object
/// - resource
/// - user
///
/// \p operations is the list of metadata operations to execute atomically. They will be
/// executed in order.
///
/// \p operation must be one of the following:
/// - add
/// - remove
///
/// \p units are optional.
///
/// On error, \p _json_output will have the following JSON structure:
/// \code{.js}
/// {
///   "operation": string,
///   "operation_index": integer,
///   "error_message": string
/// }
/// \endcode
///
/// \param[in]     _json_input  A JSON string containing the batch of metadata operations.
/// \param[in,out] _json_output A JSON string containing the error information on failure.
/// \param[in,out] _rei         A ::RuleExecInfo object that is automatically handled by the
///                             rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
///
/// \since 4.2.8
auto msi_atomic_apply_metadata_operations(msParam_t* _json_input, msParam_t* _json_output, ruleExecInfo_t* _rei) -> int;
#endif // IRODS_FOR_DOXYGEN

