#include "rodsConnect.h"
#include "user_validation_utilities.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "query_builder.hpp"

#include "fmt/format.h"

#include <regex>

namespace irods::user
{
    auto get_type(RsComm& _comm, const std::string_view _user_name) -> std::string
    {
        const auto user_and_zone = validate_name(_user_name);
        if (!user_and_zone) {
            THROW(USER_INVALID_USERNAME_FORMAT, "provided username is not properly formatted");
        }

        const auto& [user, zone] = *user_and_zone;

        const std::string qstr = fmt::format(
            "SELECT USER_TYPE where USER_NAME = '{}' and USER_ZONE = '{}'",
            user, zone.empty() ? getLocalZoneName() : zone);

        irods::experimental::query_builder qb;
        auto q = qb.build<RsComm>(_comm, qstr);
        if (q.empty()) {
            THROW(CAT_INVALID_USER, "user does not exist");
        }

        return q.front()[0];
    } // get_type

    auto type_is_valid(const std::string_view _user_type) -> bool
    {
        if (_user_type.empty()) {
            return false;
        }

        static const auto user_types = {"rodsadmin", "rodsuser", "groupadmin", "rodsgroup"};

        return std::any_of(std::cbegin(user_types), std::cend(user_types),
            [&_user_type](const auto& _t)
            {
                return _user_type == _t;
            }
        );
    } // type_is_valid

    auto validate_name(const std::string_view _user_name) -> std::optional<std::pair<std::string, std::string>>
    {
        static const auto user_name_regex = std::regex{"((\\w|[-.@])+)(#([^#]*))?"};

        try {
            std::cmatch matches;
            if (!std::regex_match(_user_name.data(), matches, user_name_regex)) {
                return {};
            }

            const auto& user_name = matches.str(1);

            if (user_name.size() >= NAME_LEN || user_name.empty()) {
                return {};
            }

            if (user_name == "." || user_name == "..") {
                return {};
            }

            const auto& zone_name = matches.str(4);

            if (zone_name.size() >= NAME_LEN) {
                return {};
            }

            return {std::make_pair(user_name, zone_name)};
        }
        catch (...) {
            return {};
        }
    } // validate_name
} // namespace irods::user
