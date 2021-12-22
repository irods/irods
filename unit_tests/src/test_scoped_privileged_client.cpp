#include <catch2/catch.hpp>

#include "scoped_privileged_client.hpp"
#include "irods_rs_comm_query.hpp"

TEST_CASE("scoped privileged client")
{
    rsComm_t conn{};

    REQUIRE_FALSE(irods::is_privileged_client(conn));

    {
        irods::experimental::scoped_privileged_client spc{conn};
        REQUIRE(irods::is_privileged_client(conn));
    }

    REQUIRE_FALSE(irods::is_privileged_client(conn));
}

