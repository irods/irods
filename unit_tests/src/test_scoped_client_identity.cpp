#include <catch2/catch.hpp>

#include "scoped_client_identity.hpp"

#include <cstring>
#include <string_view>

TEST_CASE("scoped client identity")
{
    using namespace std::string_view_literals;

    auto username = "rods"sv;

    rsComm_t conn{};
    std::strcpy(conn.clientUser.userName, username.data());

    REQUIRE(username == conn.clientUser.userName);

    {
        auto new_username = "otherrods"sv;
        irods::experimental::scoped_client_identity sci{conn, new_username};
        REQUIRE(new_username == conn.clientUser.userName);
    }

    REQUIRE(username == conn.clientUser.userName);

    // Using a name that exceeds the buffer size of rsComm_t::clientUser::userName
    // will cause an exception to be thrown.
    auto bad_username = "_________THIS_NAME_WILL_CAUSE_AN_EXCEPTION_TO_BE_THROWN_________"sv;
    const auto* expected_msg = "scoped_client_identity: new_username exceeds buffer size";
    REQUIRE_THROWS(irods::experimental::scoped_client_identity{conn, bad_username}, expected_msg);
}

