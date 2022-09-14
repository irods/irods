#include "irods/user_administration.hpp"

namespace irods::experimental::administration::inline v1
{
    auto to_user_type(const std::string_view user_type_string) -> user_type
    {
        // clang-format off
        if (user_type_string == "rodsuser")   { return user_type::rodsuser; }
        if (user_type_string == "groupadmin") { return user_type::groupadmin; }
        if (user_type_string == "rodsadmin")  { return user_type::rodsadmin; }
        // clang-format on

        throw user_management_error{"undefined user type"};
    }

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

        throw user_management_error{"cannot convert user_type to string"};
    }
} // namespace irods::experimental::administration::inline v1

