#include "irods/user_administration.hpp"

#include "irods/rodsErrorTable.h"
#include "irods/irods_exception.hpp"

#include <fmt/format.h>

namespace irods::experimental::administration
{
    auto to_user_type(const std::string_view user_type_string) -> user_type
    {
        // clang-format off
        if (user_type_string == "rodsuser")   { return user_type::rodsuser; }
        if (user_type_string == "groupadmin") { return user_type::groupadmin; }
        if (user_type_string == "rodsadmin")  { return user_type::rodsadmin; }
        // clang-format on

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Undefined user type [{}].", user_type_string));
    } // to_user_type

    auto to_c_str(user_type user_type) -> const char*
    {
        // clang-format off
        switch (user_type) {
            case user_type::rodsuser:   return "rodsuser";
            case user_type::groupadmin: return "groupadmin";
            case user_type::rodsadmin:  return "rodsadmin";
            default:                    break;
        }
        // clang-format on

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        THROW(SYS_INVALID_INPUT_PARAM, "Cannot convert user_type to string.");
    } // to_c_str
} // namespace irods::experimental::administration
