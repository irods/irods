#ifndef IRODS_USER_ADMINISTRTION_GROUP_HPP
#define IRODS_USER_ADMINISTRTION_GROUP_HPP

#include <string>
#include <ostream>

namespace irods::experimental::administration
{
    inline namespace v1
    {
        struct group
        {
            explicit group(std::string name);

            auto operator==(const group& other) const noexcept -> bool;
            auto operator!=(const group& other) const noexcept -> bool;
            auto operator< (const group& other) const noexcept -> bool;

            std::string name;
        }; // group

        auto operator<<(std::ostream& out, const group& user) -> std::ostream&;
    } // namespace v1
} // namespace irods::experimental::administration

#endif // IRODS_USER_ADMINISTRTION_GROUP_HPP
