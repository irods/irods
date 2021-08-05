#include "catch.hpp"

#include "connection_pool.hpp"
#include "dstream.hpp"
#include "filesystem.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_query.hpp"
#include "replica.hpp"
#include "rodsClient.h"
#include "transport/default_transport.hpp"

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include <chrono>
#include <string_view>

#include <unistd.h>

namespace fs = irods::experimental::filesystem;
namespace io = irods::experimental::io;

auto get_hostname() noexcept -> std::string;
auto create_resource_vault(const std::string& _vault_name) -> boost::filesystem::path;

TEST_CASE("dstream", "[iostreams]")
{
    load_client_api_plugins();

    auto conn_pool = irods::make_connection_pool(2);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";
    auto conn = conn_pool->get_connection();

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
        REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
    }};

    SECTION("default constructed stream does not cause segfault on destruction")
    {
        io::dstream stream;
    }

    SECTION("supports move semantics")
    {
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
        const auto path = sandbox / "data_object.txt";
        std::string_view message = "Hello, iRODS!";

        // Show that (in | out) openmode creates a new data object if
        // it does not exist. The (out) flag is implicit due to the use of odstream.
        {
            io::client::default_transport xport{conn};
            io::odstream{xport, path, std::ios_base::in | std::ios_base::app} << message;
        }

        REQUIRE(fs::client::exists(conn, path));
        REQUIRE(irods::experimental::replica::replica_size<rcComm_t>(conn, path, 0) == message.size());

        // Verify that the data object contains the expected message.
        {
            io::client::default_transport xport{conn};
            io::idstream in{xport, path};

            std::string line;
            std::getline(in, line);

            REQUIRE(message == line);
        }

        SECTION("out+in+app openmode appends to data object without truncating")
        {
            io::client::default_transport xport{conn};
            io::odstream out{xport, path, std::ios_base::out | std::ios_base::in | std::ios_base::app};
            out.close();

            REQUIRE(irods::experimental::replica::replica_size<rcComm_t>(conn, path, 0) > 0);

            out.open(xport, path, std::ios_base::in | std::ios_base::out | std::ios_base::app);
            out << message;
        }

        SECTION("in+app openmode appends to data object without truncating")
        {
            io::client::default_transport xport{conn};
            io::odstream out{xport, path, std::ios_base::out | std::ios_base::in | std::ios_base::app};
            out.close();

            REQUIRE(irods::experimental::replica::replica_size<rcComm_t>(conn, path, 0) > 0);

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

    SECTION("dstream supports intermediate replicas and replica tokens")
    {
        const auto path = sandbox / "data_object.txt";

        io::client::default_transport tp0{conn};
        io::odstream ds0{tp0, path};
        REQUIRE(ds0);

        auto other_conn = conn_pool->get_connection();

        SECTION("opening replica in intermediate state without replica token generates an error")
        {
            io::client::default_transport tp1{other_conn};
            io::odstream ds1{tp1, path, io::replica_number{ds0.replica_number()}};
            REQUIRE_FALSE(ds1);
        }

        SECTION("opening replica in intermediate state with replica token")
        {
            io::client::default_transport tp1{other_conn};
            io::odstream ds1{tp1, ds0.replica_token(), path, io::replica_number{ds0.replica_number()}};
            REQUIRE(ds1);
        }
    }

    SECTION("create replica using root resource")
    {
        const auto hostname = get_hostname();
        const auto vault = create_resource_vault("irods_unit_testing_vault");

        {
            const auto cmd = fmt::format("iadmin mkresc ut_ufs_resc unixfilesystem {}:{}", hostname, vault.c_str());
            REQUIRE(std::system(cmd.data()) == 0);
        }

        // A new connection pool must be used so that the resource manager within
        // the agents can see the new resources.
        auto conn_pool = irods::make_connection_pool();
        auto conn = conn_pool->get_connection();
        const auto path = sandbox / "data_object.txt";

        irods::at_scope_exit remove_resources{[&conn, &path] {
            REQUIRE(fs::client::remove(conn, path, fs::remove_options::no_trash));
            REQUIRE(std::system("iadmin rmresc ut_ufs_resc") == 0);
        }};

        io::client::native_transport tp{conn};
        io::odstream out{tp, path, io::root_resource_name{"ut_ufs_resc"}};
        REQUIRE(out);
        out << "Hello, iRODS!";
        out.close();

        REQUIRE(fs::client::is_data_object(conn, path));
    }

    SECTION("create replica using leaf resource")
    {
        const auto hostname = get_hostname();
        const auto vault = create_resource_vault("irods_unit_testing_vault_1");

        {
            // Create the resource hierarchy (ut_pt_resc;ut_ufs_resc_1)
            //                               (passthru  ;unixfilesystem)
            const auto cmd = fmt::format("iadmin mkresc ut_ufs_resc_1 unixfilesystem {}:{}", hostname, vault.c_str());
            REQUIRE(std::system("iadmin mkresc ut_pt_resc passthru") == 0);
            REQUIRE(std::system(cmd.data()) == 0);
            REQUIRE(std::system("iadmin addchildtoresc ut_pt_resc ut_ufs_resc_1") == 0);
        }

        // A new connection pool must be used so that the resource manager within
        // the agents can see the new resources.
        auto conn_pool = irods::make_connection_pool();
        auto conn = conn_pool->get_connection();
        const auto path = sandbox / "data_object.txt";

        irods::at_scope_exit remove_resources{[&conn, &path] {
            if (fs::client::exists(conn, path)) {
                REQUIRE(fs::client::remove(conn, path, fs::remove_options::no_trash));
            }

            REQUIRE(std::system("iadmin rmchildfromresc ut_pt_resc ut_ufs_resc_1") == 0);
            REQUIRE(std::system("iadmin rmresc ut_ufs_resc_1") == 0);
            REQUIRE(std::system("iadmin rmresc ut_pt_resc") == 0);
        }};

        io::client::native_transport tp{conn};
        io::odstream out{tp, path, io::leaf_resource_name{"ut_ufs_resc_1"}};
        REQUIRE(out);
        out << "Hello, iRODS!";
        out.close();

        REQUIRE(fs::client::is_data_object(conn, path));

        SECTION("create a new replica")
        {
            const auto vault = create_resource_vault("irods_unit_testing_vault_2");

            {
                const auto cmd = fmt::format("iadmin mkresc ut_ufs_resc_2 unixfilesystem {}:{}", hostname, vault.c_str());
                REQUIRE(std::system(cmd.data()) == 0);
            }

            // A new connection pool must be used so that the resource manager within
            // the agents can see the new resources.
            auto conn_pool = irods::make_connection_pool();
            auto conn = conn_pool->get_connection();
            const auto path = sandbox / "data_object.txt";

            irods::at_scope_exit remove_resources{[&conn, &path] {
                REQUIRE(fs::client::remove(conn, path, fs::remove_options::no_trash));
                REQUIRE(std::system("iadmin rmresc ut_ufs_resc_2") == 0);
            }};

            // Create the second replica.
            io::client::native_transport tp{conn};
            io::odstream out{tp, path, io::root_resource_name{"ut_ufs_resc_2"}};
            REQUIRE(out);
            out << "Hello, iRODS!";
            out.close();

            // Show that two replicas exist.
            const auto gql = fmt::format("select COUNT(DATA_ID) where COLL_NAME = '{}' and DATA_NAME = '{}'",
                                         path.parent_path().c_str(),
                                         path.object_name().c_str());

            for (auto&& row : irods::query{static_cast<rcComm_t*>(conn), gql}) {
                REQUIRE(row[0] == "2");
            }
        }
    }

    SECTION("non-existent root resource causes open to fail")
    {
        io::client::native_transport tp{conn};
        io::odstream out{tp, sandbox / "data_object.txt", io::root_resource_name{"bad_root_resc"}};
        REQUIRE_FALSE(out);
    }

    SECTION("non-existent leaf resource causes open to fail")
    {
        io::client::native_transport tp{conn};
        io::odstream out{tp, sandbox / "data_object.txt", io::leaf_resource_name{"bad_leaf_resc"}};
        REQUIRE_FALSE(out);
    }

    SECTION("replica number cannot be used to create new replicas")
    {
        io::client::native_transport tp{conn};
        io::odstream out{tp, sandbox / "should_not_exist", io::replica_number{1}};
        REQUIRE_FALSE(out);
    }

    SECTION("read previously written bytes using the same stream")
    {
        io::client::native_transport tp{conn};
        io::dstream ds{tp, sandbox / "data_object.txt", std::ios::in | std::ios::out | std::ios::trunc};

        ds.write("abcd", 4);
        ds.seekp(-2, std::ios::cur);

        char buf[2]{};
        ds.read(buf, 2);
        REQUIRE(std::string_view(buf, 2) == "cd");
    }
}

auto get_hostname() noexcept -> std::string
{
    char hostname[250];
    gethostname(hostname, sizeof(hostname));
    return hostname;
}

auto create_resource_vault(const std::string& _vault_name) -> boost::filesystem::path
{
    namespace fs = boost::filesystem;

    const auto suffix = "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    const auto vault = boost::filesystem::temp_directory_path() / (_vault_name + suffix).data();

    // Create the vault for the resource and allow the iRODS server to
    // read and write to the vault.

    if (!exists(vault)) {
        REQUIRE(fs::create_directory(vault));
    }

    fs::permissions(vault, fs::perms::add_perms | fs::perms::others_read | fs::perms::others_write);

    return vault;
}
