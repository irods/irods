#include "irods_ms_plugin.hpp"
#include "irods_re_structs.hpp"
#include "msParam.h"
#include "rodsErrorTable.h"
#include "rs_atomic_apply_metadata_operations.hpp"
#include "irods_error.hpp"
#include "rodsLog.h"

#include <functional>
#include <string>
#include <exception>

namespace
{
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
            rodsLog(LOG_ERROR, "Invalid input argument.");
            return SYS_INVALID_INPUT_PARAM;
        }

        try {
            const auto* json_input = to_string(*_json_input);
            char* json_output{};

            if (const auto ec = rs_atomic_apply_metadata_operations(_rei->rsComm, json_input, &json_output); ec != 0) {
                rodsLog(LOG_ERROR, "Error updating metadata.");
                return ec;
            }

            fillStrInMsParam(_json_output, json_output);

            return 0;
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            return e.code();
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            rodsLog(LOG_ERROR, "An unknown error occurred while processing the request.");
            return SYS_UNKNOWN_ERROR;
        }
    }

    template <typename... Args, typename Function>
    auto make_msi(const std::string& _name, Function _func) -> irods::ms_table_entry*
    {
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation<Args..., ruleExecInfo_t*>(_name, std::function<int(Args..., ruleExecInfo_t*)>(_func));
        return msi;
    }
} // anonymous namespace

extern "C"
auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<msParam_t*, msParam_t*>("msi_atomic_apply_metadata_operations", msi_impl);
}

