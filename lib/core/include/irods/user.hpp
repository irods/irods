#ifndef IRODS_USER_ADMINISTRTION_USER_HPP
#define IRODS_USER_ADMINISTRTION_USER_HPP

#include <string>
#include <optional>
#include <ostream>

namespace irods::experimental::administration
{
    inline namespace v1
    {
        struct user
        {
            explicit user(std::string name, std::optional<std::string> zone = std::nullopt);

            auto operator==(const user& other) const noexcept -> bool;
            auto operator!=(const user& other) const noexcept -> bool;
            auto operator< (const user& other) const noexcept -> bool;

            std::string name;
            std::string zone;
        }; // user

        auto operator<<(std::ostream& out, const user& user) -> std::ostream&;
    } // namespace v1
} // namespace irods::experimental::administration

#endif // IRODS_USER_ADMINISTRTION_USER_HPP
