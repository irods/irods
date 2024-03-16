#ifndef IRODS_FILESYSTEM_COPY_OPTIONS_HPP
#define IRODS_FILESYSTEM_COPY_OPTIONS_HPP

#include <cstdint>
#include <type_traits>

namespace irods::experimental::filesystem
{
    enum class copy_options : std::uint32_t
    {
        // clang-format off
        none               = 0U,
        skip_existing      = 1U,
        overwrite_existing = 2U,
        update_existing    = 4U,
        recursive          = 8U,
        collections_only   = 16U,
        in_recursive_copy  = 32U
        // clang-format on
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
