/// \file

#include "msi_preconditions.hpp"

#include "irods_error.hpp"
#include "irods_ms_plugin.hpp"
#include "irods_re_structs.hpp"
#include "msParam.h"
#include "process_stash.hpp"
#include "rodsErrorTable.h"
#include "rodsLog.h"

#include <functional>
#include <string>

namespace
{
    auto msi_impl(MsParam* _json_handle, ruleExecInfo_t* _rei) -> int
    {
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle);
        IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

        IRODS_MSI_REQUIRE_TYPE(_json_handle->type, STR_MS_T);

        try {
            const auto* handle = static_cast<const char*>(_json_handle->inOutStruct);
            rodsLog(LOG_DEBUG, "JSON handle [%s]", handle);
            irods::process_stash::erase(handle);
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, "Caught exception while releasing resources for JSON handle. [%s]", e.what());
            return SYS_LIBRARY_ERROR;
        }

        return 0;
    } // msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& name, Function func) -> irods::ms_table_entry*
    {
        auto* msi = new irods::ms_table_entry{sizeof...(Args)}; // NOLINT(cppcoreguidelines-owning-memory)
        msi->add_operation(name, std::function<int(Args..., ruleExecInfo_t*)>(func));
        return msi;
    } // make_msi
} // anonymous namespace

extern "C" auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<MsParam*>("msi_json_free", msi_impl);
} // plugin_factory

#ifdef IRODS_FOR_DOXYGEN
/// \brief Releases all memory associated with a JSON handle.
///
/// This microservice must be called to avoid leaking memory in long running agents.
///
/// \param[in]     _json_handle The handle of a JSON structure obtained via msi_json_parse().
/// \param[in,out] _rei         A ::RuleExecInfo object that is automatically handled by the
///                             rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.12
int msi_json_free(MsParam* _json_handle, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
