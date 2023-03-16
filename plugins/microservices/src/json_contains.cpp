/// \file

#include "json_common.hpp"
#include "msi_preconditions.hpp"

#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/process_stash.hpp"
#include "irods/rodsErrorTable.h"

#include <boost/any.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>

namespace
{
    // clang-format off
    using log_msi = irods::experimental::log::microservice;
    using json    = nlohmann::json;
    // clang-format on

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto msi_impl(MsParam* _json_handle, MsParam* _json_pointer, MsParam* _result, ruleExecInfo_t* _rei) -> int
    {
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer);
        IRODS_MSI_REQUIRE_VALID_POINTER(_result);
        IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

        IRODS_MSI_REQUIRE_TYPE(_json_handle->type, STR_MS_T);
        IRODS_MSI_REQUIRE_TYPE(_json_pointer->type, STR_MS_T);

        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle->inOutStruct);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer->inOutStruct);

        try {
            auto& json_object = irods::resolve_json_handle(*_json_handle);

            const auto* p = static_cast<const char*>(_json_pointer->inOutStruct);
            log_msi::debug("JSON pointer [{}]", p);

            fillIntInMsParam(_result, static_cast<int>(json_object.contains(json::json_pointer{p})));
        }
        catch (const json::parse_error& e) {
            log_msi::error("Caught exception while checking existence of JSON structure. [{}]", e.what());
            return SYS_INVALID_INPUT_PARAM;
        }
        catch (const irods::exception& e) {
            const auto* const msg = e.client_display_what();
            log_msi::error("Caught exception while checking existence of JSON structure. [{}]", msg);
            return static_cast<int>(e.code());
        }
        catch (const boost::bad_any_cast& e) {
            log_msi::error("Caught exception while checking existence of JSON structure. [{}]", e.what());
            return INVALID_ANY_CAST;
        }
        catch (const std::exception& e) {
            log_msi::error("Caught exception while checking existence of JSON structure. [{}]", e.what());
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
    return make_msi<MsParam*, MsParam*, MsParam*>("msi_json_contains", msi_impl);
} // plugin_factory

#ifdef IRODS_FOR_DOXYGEN
/// \brief Returns whether a JSON structure contains a specific element.
///
/// \param[in]     _json_handle  The handle of a JSON structure obtained via msi_json_parse().
/// \param[in]     _json_pointer A JSON pointer to the element of interest.
/// \param[out]    _result       A variable that will contain whether \p _json_pointer points to a
///                              valid location within the JSON structure. The value will be 1 if
///                              \p _json_pointer points to a valid location within the JSON
///                              structure. Otherwise, the value will be 0. The type of \p _result
///                              will be an integer.
/// \param[in,out] _rei          A ::RuleExecInfo object that is automatically handled by the
///                              rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0              On success.
/// \retval Non-zero       On failure.
/// \retval INVALID_HANDLE If the handle does not point to a valid JSON structure.
///
/// \since 4.2.12
int msi_json_contains(MsParam* _json_handle, MsParam* _json_pointer, MsParam* _result, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
