#include "irods/group.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace irods::experimental::administration
{
    inline namespace v1
    {
        group::group(std::string name)
            : name{std::move(name)}
        {
        }

        auto group::operator==(const group& other) const noexcept -> bool
        {
            return other.name == name;
        }

        auto group::operator!=(const group& other) const noexcept -> bool
        {
            return !(other == *this);
        }

        auto group::operator<(const group& other) const noexcept -> bool
        {
            return name < other.name;
        }

        auto operator<<(std::ostream& out, const group& group) -> std::ostream&
        {
            return out << json{{"name", group.name}}.dump();
        }
    } // namespace v1
} // namespace irods::experimental::administration

