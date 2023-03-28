/// \file

#include "json_common.hpp"
#include "msi_preconditions.hpp"

#include "irods_error.hpp"
#include "irods_ms_plugin.hpp"
#include "irods_re_structs.hpp"
#include "msParam.h"
#include "process_stash.hpp"
#include "rodsErrorTable.h"
#include "rodsLog.h"

#include <boost/any.hpp>
#include <json.hpp>

#include <functional>
#include <string>

namespace
{
    using json = nlohmann::json;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto msi_impl(MsParam* _json_handle, MsParam* _json_pointer, MsParam* _string, ruleExecInfo_t* _rei) -> int
    {
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer);
        IRODS_MSI_REQUIRE_VALID_POINTER(_string);
        IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

        IRODS_MSI_REQUIRE_TYPE(_json_handle->type, STR_MS_T);
        IRODS_MSI_REQUIRE_TYPE(_json_pointer->type, STR_MS_T);

        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle->inOutStruct);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer->inOutStruct);

        try {
            auto& json_object = irods::resolve_json_handle(*_json_handle);
            const auto& json_value = irods::resolve_json_pointer(json_object, *_json_pointer);
            fillStrInMsParam(_string, json_value.dump().c_str());
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, "Caught exception while serializing the JSON structure. [%s]", e.client_display_what());
            return static_cast<int>(e.code());
        }
        catch (const boost::bad_any_cast& e) {
            rodsLog(LOG_ERROR, "Caught exception while serializing the JSON structure. [%s]", e.what());
            return INVALID_ANY_CAST;
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, "Caught exception while serializing the JSON structure. [%s]", e.what());
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
    return make_msi<MsParam*, MsParam*, MsParam*>("msi_json_dump", msi_impl);
} // plugin_factory

#ifdef IRODS_FOR_DOXYGEN
/// \brief Serializes a JSON structure into a string.
///
/// \param[in]     _json_handle  The handle of a JSON structure obtained via msi_json_parse().
/// \param[in]     _json_pointer A JSON pointer to the element of interest.
/// \param[out]    _string       A variable that will contain the serialized version of the JSON
///                              structure.
/// \param[in,out] _rei          A ::RuleExecInfo object that is automatically handled by the
///                              rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0              On success.
/// \retval Non-zero       On failure.
/// \retval INVALID_HANDLE If the handle does not point to a valid JSON structure.
/// \retval DOES_NOT_EXIST If the JSON pointer does not point to a valid JSON structure.
///
/// \since 4.2.12
int msi_json_dump(MsParam* _json_handle, MsParam* _json_pointer, MsParam* _string, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
