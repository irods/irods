#ifndef IRODS_FILESYSTEM_OBJECT_STATUS_HPP
#define IRODS_FILESYSTEM_OBJECT_STATUS_HPP

#include "filesystem/filesystem.hpp"
#include "filesystem/permissions.hpp"

namespace irods {
namespace experimental {
namespace filesystem {

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

        object_status(const object_status& _other) noexcept = default;

        explicit object_status(object_type _type, perms _permissions = perms::null) noexcept
            : type_{_type}
            , perms_{_permissions}
        {
        }

        ~object_status() noexcept = default;

        // Assignment operators

        auto operator=(const object_status& _other) noexcept -> object_status& = default;

        // clang-format off

        // Observers

        auto type() const noexcept -> object_type  { return type_; }
        auto permissions() const noexcept -> perms { return perms_; }

        // Modifiers

        auto type(object_type _ot) noexcept -> void    { type_ = _ot; }
        auto permissions(perms _prms) noexcept -> void { perms_ = _prms; }

        // clang-format on

    private:
        object_type type_;
        perms perms_;
    };

} // namespace filesystem
} // namespace experimental
} // namespace irods

#endif // IRODS_FILESYSTEM_OBJECT_STATUS_HPP
