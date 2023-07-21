#include <catch2/catch.hpp>

#include "irods/rcConnect.h"
#include "irods/rodsErrorTable.h"

#include <algorithm>
#include <array>
#include <cstring>

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("set_session_signature_client_side")
{
    RcComm comm{};

    constexpr auto sig_size = sizeof(RcComm::session_signature);
    REQUIRE(std::strncmp(comm.session_signature, "", sig_size) == 0);

    SECTION("input buffer satisfies requirements")
    {
        std::array<char, 16> buf{}; // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        std::for_each(std::begin(buf), std::end(buf), [i = 0](auto& _v) mutable { _v = i++; });

        CHECK(set_session_signature_client_side(&comm, buf.data(), buf.size()) == 0);
        CHECK(std::strncmp(comm.session_signature, "000102030405060708090a0b0c0d0e0f", sig_size) == 0);
    }

    SECTION("returns error when input buffer is too small")
    {
        std::array<char, 15> buf{}; // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        CHECK(set_session_signature_client_side(&comm, buf.data(), buf.size()) == SYS_INVALID_INPUT_PARAM);
        CHECK(std::strncmp(comm.session_signature, "", sig_size) == 0);
    }

    SECTION("returns error on null pointers")
    {
        std::array<char, 16> buf{}; // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

        CHECK(set_session_signature_client_side(nullptr, buf.data(), buf.size()) == SYS_INVALID_INPUT_PARAM);
        CHECK(std::strncmp(comm.session_signature, "", sig_size) == 0);

        CHECK(set_session_signature_client_side(&comm, nullptr, buf.size()) == SYS_INVALID_INPUT_PARAM);
        CHECK(std::strncmp(comm.session_signature, "", sig_size) == 0);
    }
}
