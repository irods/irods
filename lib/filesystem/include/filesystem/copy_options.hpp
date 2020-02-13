#ifndef IRODS_FILESYSTEM_COPY_OPTIONS_HPP
#define IRODS_FILESYSTEM_COPY_OPTIONS_HPP

#include <cstdint>
#include <type_traits>

namespace irods::experimental::filesystem
{
    enum class copy_options
    {
        none                    = 0,
        skip_existing           = 1 << 0,
        overwrite_existing      = 1 << 1,
        update_existing         = 1 << 2,
        recursive               = 1 << 3,
        collections_only        = 1 << 4,
        in_recursive_copy       = 1 << 5
    };

    inline auto operator|=(copy_options& _lhs, copy_options _rhs) noexcept -> copy_options&
    {
        using T = std::underlying_type_t<copy_options>;
        return _lhs = static_cast<copy_options>(static_cast<T>(_lhs) | static_cast<T>(_rhs));
    }

    inline auto operator|(copy_options _lhs, copy_options _rhs) noexcept -> copy_options
    {
        return _lhs |= _rhs;
    }

    inline auto operator&=(copy_options& _lhs, copy_options _rhs) noexcept -> copy_options&
    {
        using T = std::underlying_type_t<copy_options>;
        return _lhs = static_cast<copy_options>(static_cast<T>(_lhs) & static_cast<T>(_rhs));
    }

    inline auto operator&(copy_options _lhs, copy_options _rhs) noexcept -> copy_options
    {
        return _lhs &= _rhs;
    }

    inline auto operator^=(copy_options& _lhs, copy_options _rhs) noexcept -> copy_options&
    {
        using T = std::underlying_type_t<copy_options>;
        return _lhs = static_cast<copy_options>(static_cast<T>(_lhs) ^ static_cast<T>(_rhs));
    }

    inline auto operator^(copy_options _lhs, copy_options _rhs) noexcept -> copy_options
    {
        return _lhs ^= _rhs;
    }

    inline auto operator~(copy_options& _value) noexcept -> copy_options&
    {
        using T = std::underlying_type_t<copy_options>;
        return _value = static_cast<copy_options>(~static_cast<T>(_value));
    }
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_COPY_OPTIONS_HPP
