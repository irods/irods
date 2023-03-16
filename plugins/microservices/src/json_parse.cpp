/// \file

#include "msi_preconditions.hpp"

#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/objInfo.h"
#include "irods/process_stash.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsPackInstruct.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <functional>
#include <span>
#include <string>

namespace
{
    // clang-format off
    using log_msi = irods::experimental::log::microservice;
    using json    = nlohmann::json;
    // clang-format on

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto msi_impl(MsParam* _range, MsParam* _range_size, MsParam* _json_handle, ruleExecInfo_t* _rei) -> int
    {
        IRODS_MSI_REQUIRE_VALID_POINTER(_range);
        IRODS_MSI_REQUIRE_VALID_POINTER(_range_size);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle);
        IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

        IRODS_MSI_REQUIRE_VALID_POINTER(_range->type);
        IRODS_MSI_REQUIRE_VALID_POINTER(_range_size->type);

        IRODS_MSI_REQUIRE_TYPE(_range->type, STR_MS_T, IntArray_MS_T);
        IRODS_MSI_REQUIRE_TYPE(_range_size->type, INT_MS_T);

        IRODS_MSI_REQUIRE_VALID_POINTER(_range->inOutStruct);
        IRODS_MSI_REQUIRE_VALID_POINTER(_range_size->inOutStruct);

        try {
            const auto buf_size = *static_cast<int*>(_range_size->inOutStruct);

            if (buf_size <= 0) {
                log_msi::error("Invalid argument: Buffer size must be greater than zero.");
                return SYS_INVALID_INPUT_PARAM;
            }

            std::string handle;

            if (const std::string_view t = _range->type; t == STR_MS_T) {
                const auto* char_array = static_cast<const char*>(_range->inOutStruct);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                handle = irods::process_stash::insert(json::parse(char_array, char_array + buf_size));
            }
            else if (t == IntArray_MS_T) {
                const auto* int_array = static_cast<IntArray*>(_range->inOutStruct);

                if (buf_size > int_array->len) {
                    log_msi::error("Invalid argument: Buffer size [{}] exceeds size of integer sequence [{}].",
                                   buf_size,
                                   int_array->len);
                    return SYS_INVALID_INPUT_PARAM;
                }

                std::span<const int> s(int_array->value, buf_size);
                log_msi::debug("IntArray => [{}]", fmt::format("{}", fmt::join(s, ", ")));

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                handle = irods::process_stash::insert(json::parse(int_array->value, int_array->value + buf_size));
            }

            log_msi::debug("New JSON handle [{}].", handle);

            fillStrInMsParam(_json_handle, handle.c_str());
        }
        catch (const std::exception& e) {
            log_msi::error("Caught exception while parsing or storing JSON string. [{}]", e.what());
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
    return make_msi<MsParam*, MsParam*, MsParam*>("msi_json_parse", msi_impl);
} // plugin_factory

#ifdef IRODS_FOR_DOXYGEN
/// \brief Parses a null-terminated string or byte/integer sequence into a JSON structure.
///
/// The parsed sequence will be stored in memory for the lifetime of the process. When the
/// JSON structure is not needed any longer, users need to call msi_json_free() to release
/// those resources.
///
/// The JSON structures created via this microservice exist in the memory of the agent, meaning
/// they are only available to the agent who created them. For instance, delay rules MUST parse
/// the byte sequence during execution of the delay rule.
///
/// \param[in]     _range       \parblock
/// A byte sequence that can be parsed into a JSON structure. The value passed must be a
/// null-terminated string or a list of integers that represent a byte sequence.
///
/// <b>Example 1: Parse a null-terminated string.</b>
/// \code{.unparsed}
/// *json_string = '{"first": "john", "last": "doe"}';
/// msiStrlen(*json_string, *length);
/// msi_json_parse(*json_string, int(*length), *handle);
/// msi_json_free(*handle);
/// \endcode
///
/// <b>Example 2: Parse a byte sequence.</b>
/// \code{.unparsed}
/// *bytes = list(123, 34, 120, 34, 58, 49, 125); # => {"x":1}
/// *length = size(*bytes);
/// msi_json_parse(*bytes, *length, *handle);
/// msi_json_free(*handle);
/// \endcode
/// \endparblock
/// \param[in]     _range_size  An integer representing the number of elements in \p _range.
/// \param[out]    _json_handle The handle that will be associated with the JSON structure upon
///                             success.
/// \param[in,out] _rei         A ::RuleExecInfo object that is automatically handled by the
///                             rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.12
int msi_json_parse(MsParam* _range, MsParam* _range_size, MsParam* _json_handle, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
