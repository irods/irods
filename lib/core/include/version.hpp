#ifndef IRODS_VERSION_HPP
#define IRODS_VERSION_HPP

/// \file

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace irods
{
    /// A simple struct capable of representing a version number.
    ///
    /// \since 4.2.9
    struct version
    {
        // clang-format off
        std::uint16_t major = 0;
        std::uint16_t minor = 0;
        std::uint16_t patch = 0;
        // clang-format on

    private:
        template <typename Operation, typename Integer, typename Function>
        constexpr auto compare_version_unit(Integer _lhs, Integer _rhs, Function _func) const noexcept -> bool
        {
            static_assert(std::is_same_v<Operation, std::greater<std::uint16_t>> ||
                          std::is_same_v<Operation, std::less<std::uint16_t>>);

            if (Operation{}(_lhs, _rhs)) {
                return true;
            }

            if (_lhs == _rhs) {
                return _func();
            }

            return false;
        }

        template <template <typename> class Operation>
        constexpr auto compare_versions(const version& _other) const noexcept -> bool
        {
            using op_type = Operation<std::uint16_t>;

            return compare_version_unit<op_type>(major, _other.major, [this, &_other] {
                return compare_version_unit<op_type>(minor, _other.minor, [this, &_other] {
                    return op_type{}(patch, _other.patch);
                });
            });
        }

    public:
        constexpr auto operator==(const version& _other) const noexcept -> bool
        {
            return major == _other.major &&
                   minor == _other.minor &&
                   patch == _other.patch;
        }

        constexpr auto operator!=(const version& _other) const noexcept -> bool
        {
            return !(*this == _other);
        }

        constexpr auto operator>(const version& _other) const noexcept -> bool
        {
            return compare_versions<std::greater>(_other);
        }

        constexpr auto operator>=(const version& _other) const noexcept -> bool
        {
            return *this == _other || *this > _other;
        }

        constexpr auto operator<(const version& _other) const noexcept -> bool
        {
            return compare_versions<std::less>(_other);
        }

        constexpr auto operator<=(const version& _other) const noexcept -> bool
        {
            return *this == _other || *this < _other;
        }
    }; // struct version

    /// \brief Converts a version string to an irods::version or std::nullopt on failure
    ///
    /// \param[in] _version \parblock
    /// A string formatted like "rodsX.Y.Z" where X, Y, and Z can represent any
    /// integer in the range of std::uint16_t. This is typical of the version string
    /// stored in RcComm.svrVersion->relVersion.
    /// \endparblock
    ///
    /// \retval std::nullopt If _version is empty or not formatted as described above
    ///
    /// \throws std::out_of_range See std::stoi documentation
    ///
    /// \since 4.3.0
    auto to_version(const std::string& _version) -> std::optional<version>;
} // namespace irods

#endif // IRODS_VERSION_HPP

