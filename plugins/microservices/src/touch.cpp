/// \file

#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/rodsErrorTable.h"
#include "irods/rs_touch.hpp"
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

    auto msi_impl(msParam_t* _json_input, ruleExecInfo_t* _rei) -> int
    {
        if (!_json_input) {
            log::microservice::error("Invalid input argument.");
            return SYS_INVALID_INPUT_PARAM;
        }

        try {
            const auto* json_input = to_string(*_json_input);
            const auto ec = rs_touch(_rei->rsComm, json_input);
            
            if (ec != 0) {
                log::microservice::error("msi_touch error [error_code={}].", ec);
            }

            return ec;
        }
        catch (const irods::exception& e) {
            log::microservice::error(e.what());
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
    return make_msi<msParam_t*>("msi_touch", msi_impl);
}

#ifdef IRODS_FOR_DOXYGEN
/// \brief Change the modification time (mtime) of a data object or collection.
///
/// Modeled after UNIX touch.
///
/// \param[in] _json_input \parblock
/// A JSON string containing information for updating the mtime.
///
/// The JSON string must have the following structure:
/// \code{.js}
/// {
///   "logical_path": string,
///   "options": {
///     "no_create": boolean,
///     "replica_number": integer,
///     "leaf_resource_name": string,
///     "seconds_since_epoch": integer,
///     "reference": string
///   }
/// }
/// \endcode
/// \endparblock
/// \param[in,out] _rei A ::RuleExecInfo object that is automatically handled
///                     by the rule engine plugin framework. Users must ignore
///                     this parameter.
///
/// \p logical_path must be an absolute path to a data object or collection.
///
/// \p options is the set of optional values for controlling the behavior
/// of the operation. This field and all fields within are optional.
///
/// \p no_create Instructs the system to not create the target data object
/// when it does not exist.
///
/// \p replica_number Identifies the replica to update. Replica numbers cannot
/// be used to create data objects or additional replicas. This option cannot
/// be used with \p leaf_resource_name.
///
/// \p leaf_resource_name Identifies the location of the replica to update.
/// If the object identified by \p logical_path does not exist and this option
/// is provided, the data object will be created at the specified resource.
/// This option cannot be used with \p replica_number.
///
/// \p seconds_since_epoch The new mtime in seconds. This option cannot be
/// used with \p reference.
///
/// \p reference The absolute path of a data object or collection whose mtime
/// will be used as the new mtime for the target object. This option cannot
/// be used with \p seconds_since_epoch.
///
/// If no object exists at \p logical_path and no options are provided, the
/// server will create a new data object on the resource defined by
/// ::msiSetDefaultResc.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.9
auto msi_touch(msParam_t* _json_input, ruleExecInfo_t* _rei) -> int;
#endif // IRODS_FOR_DOXYGEN

