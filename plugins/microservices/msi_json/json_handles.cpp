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
#include <fmt/format.h>
#include <json.hpp>

#include <functional>
#include <string>
#include <vector>

namespace
{
    auto gather_all_json_handles() -> std::vector<std::string>
    {
        std::vector<std::string> handles;

        for (auto& h : irods::process_stash::handles()) {
            // NOLINTNEXTLINE(readability-implicit-bool-conversion)
            if (boost::any_cast<const nlohmann::json>(irods::process_stash::find(h))) {
                handles.push_back(std::move(h));
            }
        }

        rodsLog(LOG_DEBUG, fmt::format("JSON handles [{}]", fmt::join(handles, ", ")).c_str());

        return handles;
    } // gather_all_json_handles

    auto allocate_and_initialize_string_array(std::size_t _number_of_strings) -> StrArray*
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        auto* list = static_cast<StrArray*>(std::malloc(sizeof(StrArray)));
        std::memset(list, 0, sizeof(StrArray));
        list->len = static_cast<int>(_number_of_strings);
        constexpr auto uuid_size = 37; // Includes space for the null byte.
        list->size = uuid_size;

        return list;
    } // allocate_and_initialize_string_array

    auto copy_handles_into_string_array(StrArray& _list, const std::vector<std::string>& _handles) -> void
    {
        const auto list_size = sizeof(char) * _list.size * _list.len;

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        _list.value = static_cast<char*>(std::malloc(list_size));

        // NOLINTNEXTLINE(bugprone-implicit-widening-of-multiplication-result)
        std::memset(_list.value, 0, list_size);

        for (int i = 0; i < _list.len; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::strncpy(_list.value + static_cast<std::ptrdiff_t>(i) * _list.size, _handles[i].c_str(), _list.size);
        }
    } // copy_handles_into_string_array

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto msi_impl(MsParam* _handles, ruleExecInfo_t* _rei) -> int
    {
        IRODS_MSI_REQUIRE_VALID_POINTER(_handles);
        IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

        try {
            const auto handles = gather_all_json_handles();
            auto* list = allocate_and_initialize_string_array(handles.size());
            copy_handles_into_string_array(*list, handles);

            // The "inOutStruct" member must ALWAYS be free'd before the "type" member.
            msp_free_inOutStruct(_handles);
            msp_free_type(_handles);

            _handles->type = strdup(StrArray_MS_T);
            _handles->inOutStruct = list;
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, "Caught exception while fetching all JSON handles. [%s]", e.client_display_what());
            return static_cast<int>(e.code());
        }
        catch (const boost::bad_any_cast& e) {
            rodsLog(LOG_ERROR, "Caught exception while fetching all JSON handles. [%s]", e.what());
            return INVALID_ANY_CAST;
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, "Caught exception while fetching all JSON handles. [%s]", e.what());
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
    return make_msi<MsParam*>("msi_json_handles", msi_impl);
} // plugin_factory

#ifdef IRODS_FOR_DOXYGEN
/// \brief Fetches all JSON handles in the process.
///
/// \param[out]    _handles A variable that will contain a list of all handles associated with
///                         JSON structures.
/// \param[in,out] _rei     A ::RuleExecInfo object that is automatically handled by the
///                         rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.12
int msi_json_handles(MsParam* _handles, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
