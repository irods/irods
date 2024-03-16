/// \file

#include "json_common.hpp"

#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/msi_preconditions.hpp"
#include "irods/process_stash.hpp"
#include "irods/rodsErrorTable.h"

#include <boost/any.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <string>

namespace
{
    // clang-format off
    using log_msi = irods::experimental::log::microservice;
    using json    = nlohmann::json;
    // clang-format on

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters, readability-function-cognitive-complexity)
    auto msi_impl(MsParam* _json_handle, MsParam* _json_pointer, MsParam* _value, ruleExecInfo_t* _rei) -> int
    {
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer);
        IRODS_MSI_REQUIRE_VALID_POINTER(_value);
        IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

        IRODS_MSI_REQUIRE_TYPE(_json_handle->type, STR_MS_T);
        IRODS_MSI_REQUIRE_TYPE(_json_pointer->type, STR_MS_T);

        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle->inOutStruct);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer->inOutStruct);

        try {
            auto& json_object = irods::resolve_json_handle(*_json_handle);
            const auto& json_value = irods::resolve_json_pointer(json_object, *_json_pointer);

            if (json_value.is_string()) {
                fillStrInMsParam(_value, json_value.get_ref<const std::string&>().c_str());
            }
            else if (json_value.is_number_integer()) {
                const auto v = json_value.get<json::number_integer_t>();
                log_msi::debug("JSON value = [{}]", v);

                if (irods::truncated<int>(v)) {
                    log_msi::warn("JSON value (signed) cannot be represented as an int. Value will be truncated.");
                }

                fillStrInMsParam(_value, std::to_string(v).c_str());
            }
            else if (json_value.is_number_unsigned()) {
                const auto v = json_value.get<json::number_unsigned_t>();
                log_msi::debug("JSON value = [{}]", v);

                if (irods::truncated<int>(v)) {
                    log_msi::warn("JSON value (unsigned) cannot be represented as an int. Value will be truncated.");
                }

                fillStrInMsParam(_value, std::to_string(v).c_str());
            }
            else if (json_value.is_number_float()) {
                const auto v = json_value.get<json::number_float_t>();
                log_msi::debug("JSON value = [{}]", v);
                fillStrInMsParam(_value, std::to_string(v).c_str());
            }
            else if (json_value.is_boolean()) {
                const auto v = json_value.get<bool>();
                log_msi::debug("JSON value = [{}]", v);
                fillStrInMsParam(_value, v ? "true" : "false");
            }
            else if (json_value.is_null()) {
                log_msi::debug("JSON value = [null]");
                fillStrInMsParam(_value, "null");
            }
            else if (json_value.is_array() || json_value.is_object() || json_value.is_binary()) {
                fillStrInMsParam(_value, json_value.type_name());
            }
            else {
                // This is only reachable if the JSON standard defines new data types.
                // Returning this error code helps to future-proof the iRODS server by providing signal
                // to the developer or administrator that this microservice or the library needs to updated.
                return TYPE_NOT_SUPPORTED;
            }
        }
        catch (const irods::exception& e) {
            log_msi::error("Caught exception while fetching JSON value. [{}]", e.client_display_what());
            return static_cast<int>(e.code());
        }
        catch (const boost::bad_any_cast& e) {
            log_msi::error("Caught exception while fetching JSON value. [{}]", e.what());
            return INVALID_ANY_CAST;
        }
        catch (const std::exception& e) {
            log_msi::error("Caught exception while fetching JSON value. [{}]", e.what());
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
    return make_msi<MsParam*, MsParam*, MsParam*>("msi_json_value", msi_impl);
} // plugin_factory

#ifdef IRODS_FOR_DOXYGEN
/// \brief Fetches a value within a JSON structure.
///
/// \param[in]     _json_handle  The handle of a JSON structure obtained via msi_json_parse().
/// \param[in]     _json_pointer \parblock
/// A JSON pointer to the element of interest. If the element cannot be represented in the iRODS
/// Rule Language, its type will be written to \p _value instead.
///
/// The following type strings can be returned:
/// - array
/// - binary
/// - object
/// - null
/// \endparblock
/// \param[out]    _value        A variable that will contain the value of the element identified by
///                              the JSON pointer. This microservice only returns strings. Numbers
///                              are converted to strings. Structured objects (i.e. arrays and objects)
///                              cannot be returned by this microservice. Boolean values are returned
///                              as "true" or "false".
/// \param[in,out] _rei          A ::RuleExecInfo object that is automatically handled by the
///                              rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0                  On success.
/// \retval Non-zero           On failure.
/// \retval INVALID_HANDLE     If the handle does not point to a valid JSON structure.
/// \retval DOES_NOT_EXIST     If the JSON pointer does not point to a valid JSON structure.
/// \retval TYPE_NOT_SUPPORTED If the microservice does not recognize the JSON value's type.
///
/// \since 4.2.12
int msi_json_value(MsParam* _json_handle, MsParam* _json_pointer, MsParam* _value, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
