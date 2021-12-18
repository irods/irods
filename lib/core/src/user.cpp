#include "user.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace irods::experimental::administration
{
    inline namespace v1
    {
        user::user(std::string name, std::optional<std::string> zone)
            : name{std::move(name)}
            , zone{zone ? *zone : ""}
        {
        }

        auto user::operator==(const user& other) const noexcept -> bool
        {
            return other.name == name && other.zone == zone;
        }

        auto user::operator!=(const user& other) const noexcept -> bool
        {
            return !(*this == other);
        }

        auto user::operator<(const user& other) const noexcept -> bool
        {
            return name < other.name && zone < other.zone;
        }

        auto operator<<(std::ostream& out, const user& user) -> std::ostream&
        {
            return out << json{{"name", user.name}, {"zone", user.zone}}.dump();
        }
    } // namespace v1
} // namespace irods::experimental::administration

