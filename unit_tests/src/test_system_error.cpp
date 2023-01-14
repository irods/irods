#include "catch.hpp"

#include "rodsErrorTable.h"
#include "system_error.hpp"

#include "fmt/format.h"

#include <limits>
#include <string>
#include <string_view>

TEST_CASE("irods_category")
{
    using namespace std::string_view_literals;
    constexpr auto ec = USER_FILE_DOES_NOT_EXIST;
    const auto errc = irods::experimental::make_error_code(ec);
    CHECK(errc.value() == ec);
    CHECK(irods::experimental::get_irods_error_code(errc) == USER_FILE_DOES_NOT_EXIST);
    CHECK(irods::experimental::get_errno(errc) == 0);
    CHECK(errc.message() == "USER_FILE_DOES_NOT_EXIST");
    CHECK(errc.category().name() == "iRODS"sv);
}

TEST_CASE("irods_category reports embedded errno value")
{
    using namespace std::string_view_literals;
    constexpr auto ec = USER_FILE_DOES_NOT_EXIST - 120; // 120 = EINTR
    const auto errc = irods::experimental::make_error_code(ec);
    CHECK(errc.value() == ec);
    CHECK(irods::experimental::get_irods_error_code(errc) == USER_FILE_DOES_NOT_EXIST);
    CHECK(irods::experimental::get_errno(errc) == 120);
    CHECK(errc.message() == "USER_FILE_DOES_NOT_EXIST [errno=120]");
    CHECK(errc.category().name() == "iRODS"sv);
}

TEST_CASE("irods_category with positive value")
{
    using namespace std::string_view_literals;
    constexpr auto ec = -USER_FILE_DOES_NOT_EXIST;
    const auto errc = irods::experimental::make_error_code(ec);
    CHECK(errc.value() == ec);
    CHECK(irods::experimental::get_irods_error_code(errc) == -USER_FILE_DOES_NOT_EXIST);
    CHECK(irods::experimental::get_errno(errc) == 0);
    CHECK(errc.message() == fmt::format("Unknown error {}", ec));
    CHECK(errc.category().name() == "iRODS"sv);
}

TEST_CASE("irods_category with positive value with embedded errno value")
{
    using namespace std::string_view_literals;
    constexpr auto ec = -USER_FILE_DOES_NOT_EXIST + 120; // 120 = EINTR
    const auto errc = irods::experimental::make_error_code(ec);
    CHECK(errc.value() == ec);
    CHECK(irods::experimental::get_irods_error_code(errc) == -USER_FILE_DOES_NOT_EXIST);
    CHECK(irods::experimental::get_errno(errc) == 120);
    CHECK(errc.message() == fmt::format("Unknown error {}", ec));
    CHECK(errc.category().name() == "iRODS"sv);
}

TEST_CASE("irods_category handles unknown error codes")
{
    using namespace std::string_view_literals;
    constexpr auto ec = -5;
    const auto errc = irods::experimental::make_error_code(ec);
    CHECK(errc.value() == ec);
    CHECK(irods::experimental::get_irods_error_code(errc) == 0);
    CHECK(irods::experimental::get_errno(errc) == 0);
    CHECK(errc.message() == "Unknown error -5");
    CHECK(errc.category().name() == "iRODS"sv);
}

TEST_CASE("extraction functions detect incompatible error_category")
{
    const auto errc = std::make_error_code(std::errc::already_connected);
    constexpr auto invalid_value = std::numeric_limits<int>::min();
    CHECK(irods::experimental::get_irods_error_code(errc) == invalid_value);
    CHECK(irods::experimental::get_errno(errc) == invalid_value);
}
