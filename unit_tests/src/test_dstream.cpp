#include "catch.hpp"

#include "rodsClient.h"
#include "connection_pool.hpp"
#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "filesystem.hpp"

TEST_CASE("dstream", "[iostreams]")
{
    namespace fs = irods::experimental::filesystem;
    namespace io = irods::experimental::io;

    SECTION("default constructed stream does not cause segfault on destruction")
    {
        io::dstream stream;
    }

    SECTION("supports move semantics")
    {
        rodsEnv env;
        REQUIRE(getRodsEnv(&env) == 0);

        irods::connection_pool conn_pool{1, env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, 600};

        auto conn = conn_pool.get_connection();
        const auto path = fs::path{env.rodsHome} / "dstream_data_object.txt";

        // Guarantees that the stream is closed before clean up.
        {
            io::client::default_transport xport{conn};
            io::dstream stream{xport, path};
            REQUIRE(stream.is_open());

            // Move construct.
            auto other_stream = std::move(stream);
            REQUIRE_FALSE(stream.is_open());
            REQUIRE(other_stream.is_open());
            
            // Construct and then move assign the stream.
            io::dstream another_stream;
            REQUIRE_FALSE(another_stream.is_open());

            another_stream = std::move(other_stream);
            REQUIRE_FALSE(other_stream.is_open());
            REQUIRE(another_stream.is_open());
        }

        // Clean up.
        fs::client::remove(conn, path, fs::remove_options::no_trash);
    }

    SECTION("allows access to underlying stream buffer object")
    {
        io::dstream stream;
        REQUIRE_FALSE(!stream.rdbuf());
    }
}

