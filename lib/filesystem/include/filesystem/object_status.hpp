#ifndef IRODS_FILESYSTEM_OBJECT_STATUS_HPP
#define IRODS_FILESYSTEM_OBJECT_STATUS_HPP

#include "filesystem/filesystem.hpp"
#include "filesystem/permissions.hpp"

#include <vector>

namespace irods::experimental::filesystem
{
    enum class object_type
    {
        none,
        not_found,
        data_object,
        collection,
        special_collection,
        unknown
    };

    class object_status
    {
    public:
        // Constructors and destructor

        object_status() noexcept
            : object_status{object_type::none}
        {
        }

        object_status(const object_status& _other) = default;

        explicit object_status(object_type _type, const std::vector<entity_permission>& _perms = {})
            : type_{_type}
            , perms_{_perms}
        {
        }

        ~object_status() noexcept = default;

        // Assignment operators

        auto operator=(const object_status& _other) -> object_status& = default;

        // clang-format off

        // Observers

        auto type() const noexcept -> object_type { return type_; }
        auto permissions() const noexcept -> const std::vector<entity_permission>& { return perms_; }

        // Modifiers

        auto type(object_type _ot) noexcept -> void { type_ = _ot; }
        auto permissions(const std::vector<entity_permission>& _perms) -> void { perms_ = _perms; }

        // clang-format on

    private:
        object_type type_;
        std::vector<entity_permission> perms_;
    };
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_OBJECT_STATUS_HPP
