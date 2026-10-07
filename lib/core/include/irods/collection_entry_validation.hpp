#ifndef IRODS_COLLECTION_ENTRY_VALIDATION_HPP
#define IRODS_COLLECTION_ENTRY_VALIDATION_HPP

#include "irods/miscUtil.h"

#include <string_view>

namespace irods
{
    inline auto is_valid_child_name(std::string_view name) -> bool
    {
        // NOLINTNEXTLINE(readability-simplify-boolean-expr)
        return !(name.empty() || name == "." || name == ".." || name.find('/') != std::string_view::npos ||
                 name.find('\0') != std::string_view::npos);
    } // is_valid_child_name

    inline auto is_valid_collection_path(std::string_view path) -> bool
    {
        // COLL_NAME is a full logical path, not a single child name.
        // Allow its leading separator, but validate every component.
        if (!path.empty() && path.front() == '/') {
            path.remove_prefix(1);
        }

        while (true) {
            const auto separator = path.find('/');
            if (!is_valid_child_name(path.substr(0, separator))) {
                return false;
            }

            if (separator == std::string_view::npos) {
                return true;
            }

            path.remove_prefix(separator + 1);
        }
    } // is_valid_collection_path

    inline auto is_valid_collection_entry(const collEnt_t& entry) -> bool
    {
        if (entry.objType != DATA_OBJ_T && entry.objType != COLL_OBJ_T) {
            return true;
        }

        if (!entry.collName || !is_valid_collection_path(entry.collName)) {
            return false;
        }

        if (entry.objType == DATA_OBJ_T) {
            return entry.dataName && is_valid_child_name(entry.dataName);
        }

        return true;
    } // is_valid_collection_entry
} // namespace irods

#endif // IRODS_COLLECTION_ENTRY_VALIDATION_HPP
