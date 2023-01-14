#include "system_error.hpp"

#define MAKE_IRODS_ERROR_MAP
#include "rodsErrorTable.h"

#include <fmt/format.h>

#include <cmath>
#include <limits>
#include <string_view>

namespace
{
    // Used to compute the value of an errno value embedded in an iRODS error code.
    constexpr auto errno_divisor = 1000;

    constexpr auto is_irods_error_category(const std::error_code& _errc) -> bool
    {
        return _errc.category().name() == std::string_view{"iRODS"};
    } // is_irods_error_category

    constexpr auto get_irods_error_code(int _ec) -> int
    {
        return _ec / errno_divisor * errno_divisor;
    } // get_irods_error_code_impl
} // anonymous namespace

namespace irods::experimental
{
    auto error_category::message(int _condition) const -> std::string
    {
        // Convert to a pure iRODS error code (i.e. clear the last three digits).
        const auto irods_ec = ::get_irods_error_code(_condition);

        // Capture the errno value.
        const auto embedded_errno_value = std::abs(_condition) - std::abs(irods_ec);

        using irods_error_map_construction::irods_error_map;

        if (const auto iter = irods_error_map.find(irods_ec); iter != std::end(irods_error_map)) {
            if (embedded_errno_value != 0) {
                return fmt::format("{} [errno={}]", iter->second, embedded_errno_value);
            }

            return iter->second;
        }

        return fmt::format("Unknown error {}", _condition);
    } // error_category::message

    auto irods_category() noexcept -> const error_category&
    {
        static const error_category category;
        return category;
    } // irods_category

    auto make_error_code(int _ec) noexcept -> std::error_code
    {
        return {_ec, irods_category()};
    } // make_error_code

    auto get_irods_error_code(const std::error_code& _errc) -> int
    {
        if (!is_irods_error_category(_errc)) {
            return std::numeric_limits<int>::min();
        }

        return ::get_irods_error_code(_errc.value());
    } // get_irods_error_code

    auto get_errno(const std::error_code& _errc) -> int
    {
        if (!is_irods_error_category(_errc)) {
            return std::numeric_limits<int>::min();
        }

        const auto ec = std::abs(_errc.value());

        if (ec < errno_divisor) {
            return 0;
        }

        return std::abs(ec) - ::get_irods_error_code(ec);
    } // get_errno
} // namespace irods::experimental
