#include "catch.hpp"

#include "connection_pool.hpp"
#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "filesystem.hpp"
#include "irods_at_scope_exit.hpp"

#include <string_view>

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
        _getRodsEnv(env);

        auto conn_pool = irods::make_connection_pool();
        auto conn = conn_pool->get_connection();
        const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

        if (!fs::client::exists(conn, sandbox)) {
            REQUIRE(fs::client::create_collection(conn, sandbox));
        }

        irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
            REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
        }};

        const auto path = sandbox / "data_object.txt";

        // Guarantees that the stream is closed before clean up.
        {
            io::client::default_transport xport{conn};
            io::odstream stream{xport, path};
            REQUIRE(stream.is_open());

            // Move construct.
            auto other_stream = std::move(stream);
            REQUIRE_FALSE(stream.is_open());
            REQUIRE(other_stream.is_open());
            
            // Construct and then move assign the stream.
            io::odstream another_stream;
            REQUIRE_FALSE(another_stream.is_open());

            another_stream = std::move(other_stream);
            REQUIRE_FALSE(other_stream.is_open());
            REQUIRE(another_stream.is_open());
        }
    }

    SECTION("allows access to underlying stream buffer object")
    {
        io::dstream stream;
        REQUIRE_FALSE(!stream.rdbuf());
    }

    SECTION("in, out, app openmode combinations are correctly translated to POSIX open flags")
    {
        rodsEnv env;
        _getRodsEnv(env);

        auto conn_pool = irods::make_connection_pool();
        auto conn = conn_pool->get_connection();
        const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

        if (!fs::client::exists(conn, sandbox)) {
            REQUIRE(fs::client::create_collection(conn, sandbox));
        }

        irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
            REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
        }};

        const auto path = sandbox / "data_object.txt";
        std::string_view message = "Hello, iRODS!";

        // Show that (in | out) openmode creates a new data object if
        // it does not exist.
        {
            io::client::default_transport xport{conn};
            io::odstream{xport, path, std::ios_base::in | std::ios_base::app} << message;
        }

        REQUIRE(fs::client::exists(conn, path));

        // Verify that the data object contains the expected message.
        {
            io::client::default_transport xport{conn};
            io::idstream in{xport, path};

            std::string line;
            std::getline(in, line);

            REQUIRE(message == line);
        }

        // Open the data object with the openmode flags that were being
        // translated into POSIX flags incorrectly and write the same message
        // to the data object.

        SECTION("out+in+app openmode appends to data object without truncating")
        {
            io::client::default_transport xport{conn};
            io::odstream out{xport, path, std::ios_base::out | std::ios_base::in | std::ios_base::app};
            out.close();

            REQUIRE(fs::client::data_object_size(conn, path) > 0);

            out.open(xport, path, std::ios_base::in | std::ios_base::out | std::ios_base::app);
            out << message;
        }

        SECTION("in+app openmode appends to data object without truncating")
        {
            io::client::default_transport xport{conn};
            io::odstream out{xport, path, std::ios_base::out | std::ios_base::in | std::ios_base::app};
            out.close();

            REQUIRE(fs::client::data_object_size(conn, path) > 0);

            out.open(xport, path, std::ios_base::in | std::ios_base::out | std::ios_base::app);
            out << message;
        }

        // The final data object should contain the message twice.
        io::client::default_transport xport{conn};
        io::idstream in{xport, path};

        std::string line;
        std::getline(in, line);

        REQUIRE(std::string{message} + message.data() == line);
    }
}

