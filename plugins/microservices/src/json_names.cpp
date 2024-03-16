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

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <tuple>
#include <vector>

namespace
{
    // clang-format off
    using log_msi = irods::experimental::log::microservice;
    using json    = nlohmann::json;
    // clang-format on

    auto gather_json_names(const json& _json) -> std::tuple<std::vector<std::string>, int>
    {
        std::vector<std::string> names;
        names.reserve(_json.size());

        std::size_t max_size = 0;

        for (const auto& [name, _] : _json.items()) {
            names.push_back(name);

            // Add one to "name.size()" to accomodate for the null byte.
            // This allows strings which match "max_length" to be null terminated.
            // If one is not added, iterating over the list of names will not work as expected.
            max_size = std::max(max_size, name.size() + 1);
        }

        log_msi::debug("names = [{}]", fmt::format("{}", fmt::join(names, ", ")));

        return {names, max_size};
    } // gather_json_names

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto allocate_and_initialize_string_array(std::size_t _number_of_strings, int _max_string_size) -> StrArray*
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        auto* list = static_cast<StrArray*>(std::malloc(sizeof(StrArray)));
        std::memset(list, 0, sizeof(StrArray));

        if (irods::truncated<int>(_number_of_strings)) {
            log_msi::warn("Number of strings cannot be represented as an int. Value will be truncated.");
        }

        list->len = static_cast<int>(_number_of_strings);
        list->size = _max_string_size;

        return list;
    } // allocate_and_initialize_string_array

    auto copy_names_into_string_array(StrArray& _list, const std::vector<std::string>& _names) -> void
    {
        const auto list_size = sizeof(char) * _list.size * _list.len;

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        _list.value = static_cast<char*>(std::malloc(list_size));

        // NOLINTNEXTLINE(bugprone-implicit-widening-of-multiplication-result)
        std::memset(_list.value, 0, list_size);

        for (int i = 0; i < _list.len; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::strncpy(_list.value + static_cast<std::ptrdiff_t>(i) * _list.size, _names[i].c_str(), _list.size);
        }
    } // copy_names_into_string_array

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto msi_impl(MsParam* _json_handle, MsParam* _json_pointer, MsParam* _names, ruleExecInfo_t* _rei) -> int
    {
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer);
        IRODS_MSI_REQUIRE_VALID_POINTER(_names);
        IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

        IRODS_MSI_REQUIRE_TYPE(_json_handle->type, STR_MS_T);
        IRODS_MSI_REQUIRE_TYPE(_json_pointer->type, STR_MS_T);

        IRODS_MSI_REQUIRE_VALID_POINTER(_json_handle->inOutStruct);
        IRODS_MSI_REQUIRE_VALID_POINTER(_json_pointer->inOutStruct);

        try {
            auto& json_object = irods::resolve_json_handle(*_json_handle);
            const auto& json_value = irods::resolve_json_pointer(json_object, *_json_pointer);

            const auto [names, max_string_size] = gather_json_names(json_value);
            auto* list = allocate_and_initialize_string_array(names.size(), max_string_size);
            copy_names_into_string_array(*list, names);

            // The "inOutStruct" member must ALWAYS be free'd before the "type" member.
            msp_free_inOutStruct(_names);
            msp_free_type(_names);

            _names->type = strdup(StrArray_MS_T);
            _names->inOutStruct = list;
        }
        catch (const irods::exception& e) {
            log_msi::error("Caught exception while fetching JSON property names. [{}]", e.client_display_what());
            return static_cast<int>(e.code());
        }
        catch (const boost::bad_any_cast& e) {
            log_msi::error("Caught exception while fetching JSON property names. [{}]", e.what());
            return INVALID_ANY_CAST;
        }
        catch (const std::exception& e) {
            log_msi::error("Caught exception while fetching JSON property names. [{}]", e.what());
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
    return make_msi<MsParam*, MsParam*, MsParam*>("msi_json_names", msi_impl);
} // plugin_factory

#ifdef IRODS_FOR_DOXYGEN
/// \brief Fetches the property names within a JSON structure.
///
/// \param[in]     _json_handle  The handle of a JSON structure obtained via msi_json_parse().
/// \param[in]     _json_pointer A JSON pointer to the element of interest.
/// \param[out]    _names        A variable that will contain a list of all names inside the element
///                              identified by the JSON pointer.
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
int msi_json_names(MsParam* _json_handle, MsParam* _json_pointer, MsParam* _names, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
