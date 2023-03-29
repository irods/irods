#ifndef IRODS_JSON_COMMON_HPP
#define IRODS_JSON_COMMON_HPP

/// \file

#include "irods_exception.hpp"
#include "msParam.h"
#include "process_stash.hpp"
#include "rodsErrorTable.h"
#include "rodsLog.h"

#include <boost/any.hpp>
#include <fmt/format.h>
#include <json.hpp>

namespace irods
{
    /// Resolves a JSON handle to a JSON structure.
    ///
    /// \param[in] _json_handle A microservice parameter containing a string which represents a JSON handle.
    ///
    /// \throws irods::exception    If the handle does not point to a valid JSON structure.
    /// \throws boost::bad_any_cast If the object pointed to by \p _json_handle does not point to the expected type.
    ///
    /// \returns A reference to a JSON structure.
    ///
    /// \since 4.2.12
    inline auto resolve_json_handle(const MsParam& _json_handle) -> nlohmann::json&
    {
        const auto* handle = static_cast<const char*>(_json_handle.inOutStruct);
        rodsLog(LOG_DEBUG, "JSON handle [%s]", handle);

        auto* object = irods::process_stash::find(handle);

        if (!object) { // NOLINT(readability-implicit-bool-conversion)
            constexpr const char* msg = "Invalid argument: JSON handle [{}] is not associated with any object.";
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(INVALID_HANDLE, fmt::format(msg, handle));
        }

        return boost::any_cast<nlohmann::json&>(*object);
    } // resolve_json_handle

    /// Resolves a JSON pointer to a JSON structure.
    ///
    /// \param[in] _json         The JSON structure to search.
    /// \param[in] _json_pointer A microservice parameter containing a string which represents a JSON pointer.
    ///
    /// \throws irods::exception If the pointer does not point to a valid JSON structure.
    ///
    /// \returns A reference to a JSON structure.
    ///
    /// \since 4.2.12
    inline auto resolve_json_pointer(nlohmann::json& _json, const MsParam& _json_pointer) -> nlohmann::json&
    {
        try {
            const auto* p = static_cast<const char*>(_json_pointer.inOutStruct);
            rodsLog(LOG_DEBUG, "JSON pointer [%s]", p); // Assumes the nullptr check has already happened.
            return _json.at(nlohmann::json::json_pointer{p});
        }
        catch (const nlohmann::json::out_of_range& e) {
            constexpr const char* msg = "JSON pointer does not reference a valid location in JSON structure. [%s]";
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(DOES_NOT_EXIST, fmt::format(msg, e.what()));
        }
    } // resolve_json_pointer
} // namespace irods

#endif // IRODS_JSON_COMMON_HPP
