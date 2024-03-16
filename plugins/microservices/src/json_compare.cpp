/// \file

#include "json_common.hpp"

#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/msi_preconditions.hpp"
#include "irods/objInfo.h" // For StrArray
#include "irods/process_stash.hpp"
#include "irods/rodsErrorTable.h"

#include <boost/any.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace
{
    // clang-format off
    using log_msi = irods::experimental::log::microservice;
    using json    = nlohmann::json;
    // clang-format on

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    auto msi_impl(MsParam* _json_handle_1,
                  MsParam* _json_pointer_1, // NOLINT(bugprone-easily-swappable-parameters)
                  MsParam* _json_handle_2,
                  MsParam* _json_pointer_2, // NOLINT(bugprone-easily-swappable-parameters)
                  MsParam* _result,
                  ruleExecInfo_t* _rei) -> int
    {
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle_1);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle_2);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer_1);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer_2);
        IRODS_MSI_REQUIRE_VALID_POINTER(_result);
        IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

        IRODS_MSI_REQUIRE_TYPE(_json_handle_1->type, STR_MS_T);
        IRODS_MSI_REQUIRE_TYPE(_json_handle_2->type, STR_MS_T);
        IRODS_MSI_REQUIRE_TYPE(_json_pointer_1->type, STR_MS_T);
        IRODS_MSI_REQUIRE_TYPE(_json_pointer_2->type, STR_MS_T);

        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle_1->inOutStruct);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle_2->inOutStruct);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer_1->inOutStruct);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer_2->inOutStruct);

        try {
            auto& json_object_1 = irods::resolve_json_handle(*_json_handle_1);
            const auto& json_value_1 = irods::resolve_json_pointer(json_object_1, *_json_pointer_1);

            // We could detect if the handles match and avoid some of this code, but it's
            // probably not going to have that big of an impact on performance.
            auto& json_object_2 = irods::resolve_json_handle(*_json_handle_2);
            const auto& json_value_2 = irods::resolve_json_pointer(json_object_2, *_json_pointer_2);

            char* result{};

            // clang-format off
            if      (json_value_1 == json_value_2) { result = strdup("equivalent"); }
            else if (json_value_1  < json_value_2) { result = strdup("less"); }
            else if (json_value_1  > json_value_2) { result = strdup("greater"); }
            else                                   { result = strdup("unordered"); }
            // clang-format on

            fillStrInMsParam(_result, result);
        }
        catch (const irods::exception& e) {
            log_msi::error("Caught exception while comparing JSON objects. [{}]", e.client_display_what());
            return static_cast<int>(e.code());
        }
        catch (const boost::bad_any_cast& e) {
            log_msi::error("Caught exception while comparing JSON objects. [{}]", e.what());
            return INVALID_ANY_CAST;
        }
        catch (const std::exception& e) {
            log_msi::error("Caught exception while comparing JSON objects. [{}]", e.what());
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
    return make_msi<MsParam*, MsParam*, MsParam*, MsParam*, MsParam*>("msi_json_compare", msi_impl);
} // plugin_factory

#ifdef IRODS_FOR_DOXYGEN
/// \brief Compares two elements within one or two JSON structures.
///
/// \param[in]     _json_handle_1  The handle of a JSON structure obtained via msi_json_parse(). This
///                                This handle is allowed to match \p _json_handle_2.
/// \param[in]     _json_pointer_1 A JSON pointer to the element of interest in the JSON structure
///                                identified by \p _json_handle_1.
/// \param[in]     _json_handle_2  The handle of a JSON structure obtained via msi_json_parse(). This
///                                This handle is allowed to match \p _json_handle_1.
/// \param[in]     _json_pointer_2 A JSON pointer to the element of interest in the JSON structure
///                                identified by \p _json_handle_2.
/// \param[out]    _result         \parblock
/// A variable that will contain the result of the comparison.
///
/// This variable will hold one of the following:
/// - "equivalent": The elements are equivalent.
/// - "less"      : The first element is sequenced before the second element.
/// - "greater"   : The first element is sequenced after the second element.
/// - "unordered" : The elements cannot be compared.
/// \endparblock
/// \param[in,out] _rei            A ::RuleExecInfo object that is automatically handled by the
///                                rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0              On success.
/// \retval Non-zero       On failure.
/// \retval INVALID_HANDLE If either handle does not point to a valid JSON structure.
/// \retval DOES_NOT_EXIST If either JSON pointer does not point to a valid JSON structure.
///
/// \since 4.2.12
int msi_json_compare(MsParam* _json_handle_1,
                     MsParam* _json_pointer_1,
                     MsParam* _json_handle_2,
                     MsParam* _json_pointer_2,
                     MsParam* _result,
                     ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
