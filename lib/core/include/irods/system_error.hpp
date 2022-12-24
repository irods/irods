#ifndef IRODS_SYSTEM_ERROR_HPP
#define IRODS_SYSTEM_ERROR_HPP

/// \file

#include <compare>
#include <system_error>

namespace irods::experimental
{
    /// An error category that contains iRODS specific logic for dealing with
    /// iRODS error codes.
    ///
    /// See https://cppreference.com for additional details about std::error_category.
    ///
    /// \since 4.2.12
    class error_category : public std::error_category
    {
      public:
        [[nodiscard]] auto name() const noexcept -> const char* override
        {
            return "iRODS";
        } // name

        [[nodiscard]] auto message(int _condition) const -> std::string override;

        auto operator==(const error_category& _rhs) const noexcept -> bool
        {
            return this == &_rhs;
        } // operator==

        auto operator<=>(const error_category& _rhs) const noexcept -> std::strong_ordering
        {
            if (this < &_rhs) {
                return std::strong_ordering::less;
            }

            if (this > &_rhs) {
                return std::strong_ordering::greater;
            }

            return std::strong_ordering::equal;
        } // operator<=>
    }; // class error_category

    /// Returns a reference to the static error category for iRODS errors.
    ///
    /// \since 4.2.12
    [[nodiscard]] auto irods_category() noexcept -> const error_category&;

    /// Creates an error code object holding the value \p _ec.
    ///
    /// Equivalent to the following:
    /// \code{.cpp}
    /// std::error_code{_ec, irods::experimental::irods_category()}
    /// \endcode
    ///
    /// \since 4.2.12
    [[nodiscard]] auto make_error_code(int _ec) noexcept -> std::error_code;

    /// Returns the embedded iRODS error code value from \p _errc if available.
    ///
    /// If \p _errc contains a valid iRODS error code, the value returned will match one defined in
    /// rodsErrorTable.h.
    ///
    /// If the \p _errc is not categorized as an iRODS error code (i.e. does not reference the
    /// \p irods_category), then std::numeric_limits<int>::min() is returned.
    ///
    /// \since 4.2.12
    [[nodiscard]] auto get_irods_error_code(const std::error_code& _errc) -> int;

    /// Returns the embedded errno value from the \p _errc if available.
    ///
    /// If the error code is less than 1000 or \p _errc is not categorized as an iRODS error
    /// code (i.e. does not reference the \p irods_category), then std::numeric_limits<int>::min()
    /// is returned.
    ///
    /// \since 4.2.12
    [[nodiscard]] auto get_errno(const std::error_code& _errc) -> int;
} // namespace irods::experimental

#endif // IRODS_SYSTEM_ERROR_HPP
