#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "irods/connection_pool.hpp"
#include "irods/filesystem.hpp"
#include "irods/fully_qualified_username.hpp"
#include "irods/getRodsEnv.h"
#include "irods/metadata.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsClient.h"

#include "irods/irods_at_scope_exit.hpp"
#include "irods/dstream.hpp"
#include "irods/transport/default_transport.hpp"

#include <vector>
#include <string>

namespace ix  = irods::experimental;
namespace ixm = ix::metadata;
using entity_type = ix::entity::entity_type;

TEST_CASE("metadata")
{
    load_client_api_plugins();

    rodsEnv env;
    REQUIRE(getRodsEnv(&env) == 0);

    const int cp_size = 1;

    irods::experimental::fully_qualified_username user{env.rodsUserName, env.rodsZone};
    irods::connection_pool conn_pool{cp_size, env.rodsHost, env.rodsPort, user};

    auto conn = conn_pool.get_connection();

    // clang-format off
    namespace fs = irods::experimental::filesystem;

    using odstream          = irods::experimental::io::odstream;
    using default_transport = irods::experimental::io::client::default_transport;
    // clang-format on

    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
        REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
    }};

    SECTION("collection remove")
    {
        auto md = ixm::avu{"a", "v", "u"};
        ixm::set(conn, md, entity_type::collection, env.rodsHome);
        auto res = ixm::get(conn, entity_type::collection, env.rodsHome);
        REQUIRE(res.size() == 1);

        ixm::remove(conn, md, entity_type::collection, env.rodsHome);
        res = ixm::get(conn, entity_type::collection, env.rodsHome);
        REQUIRE(res.size() == 0);
    }

    SECTION("collection set")
    {
        auto md = ixm::avu{"a", "v", "u"};
        ixm::set(conn, md, entity_type::collection, env.rodsHome);
        auto res = ixm::get(conn, entity_type::collection, env.rodsHome);
        REQUIRE(res[0] == md);
        ixm::remove(conn, md, entity_type::collection, env.rodsHome);
    }

    SECTION("collection add")
    {
        auto md   = ixm::avu{"a", "v", "u"};
        ixm::add(conn, md, entity_type::collection, env.rodsHome);
        auto res = ixm::get(conn, entity_type::collection, env.rodsHome);
        REQUIRE(res[0] == md);
        ixm::remove(conn, md, entity_type::collection, env.rodsHome);
    }

    SECTION("collection modify")
    {
        auto md  = ixm::avu{"a", "v", "u"};
        auto md2 = ixm::avu{"a2", "v2", "u2"};

        ixm::add(conn, md, entity_type::collection, env.rodsHome);
        auto res = ixm::get(conn, entity_type::collection, env.rodsHome);
        REQUIRE(res[0] == md);

        ixm::modify(conn, md, md2, entity_type::collection, env.rodsHome);
        res = ixm::get(conn, entity_type::collection, env.rodsHome);
        REQUIRE(res[0] == md2);

        ixm::remove(conn, md2, entity_type::collection, env.rodsHome);
    }

    SECTION("object remove")
    {
        const fs::path p = sandbox / "data_object";

        {
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        auto md = ixm::avu{"a", "v", "u"};
        ixm::set(conn, md, entity_type::data_object, p.string());
        auto res = ixm::get(conn, entity_type::data_object, p.string());
        REQUIRE(res.size() == 1);

        ixm::remove(conn, md, entity_type::data_object, p.string());
        res = ixm::get(conn, entity_type::data_object, p.string());
        REQUIRE(res.size() == 0);

        REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
    }

    SECTION("object add")
    {
        const auto p = sandbox / "data_object";

        {
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        auto md = ixm::avu{"a", "v", "u"};
        ixm::add(conn, md, entity_type::data_object, p);
        auto res = ixm::get(conn, entity_type::data_object, p);
        REQUIRE(res.size() == 1);

        ixm::remove(conn, md, entity_type::data_object, p);
        REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
    }

    SECTION("object set")
    {
        const fs::path p = sandbox / "data_object";

        INFO(fmt::format("This is the path: {}", p.string()));

        {
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        REQUIRE(fs::client::exists(conn, p));

        auto md = ixm::avu{"a", "v", "u"};
        ixm::set(conn, md, entity_type::data_object, p);

        auto res = ixm::get(conn, entity_type::data_object, p);
        REQUIRE(res.size() == 1);
        ixm::remove(conn, md, entity_type::data_object, p);

        REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
    }

    SECTION("object modify")
    {
        const fs::path p = sandbox / "data_object";

        INFO(fmt::format("This is the path: {}", p.string()));

        {
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        REQUIRE(fs::client::exists(conn, p));

        auto md  = ixm::avu{"a", "v", "u"};
        auto md2 = ixm::avu{"a2", "v2", "u2"};

        ixm::add(conn, md, entity_type::data_object, p);
        auto res = ixm::get(conn, entity_type::data_object, p);
        REQUIRE(res[0] == md);

        ixm::modify(conn, md, md2, entity_type::data_object, p);
        res = ixm::get(conn, entity_type::data_object, p);
        REQUIRE(res[0] == md2);

        ixm::remove(conn, md2, entity_type::data_object, p);
    }

    SECTION("user remove")
    {
        auto md = ixm::avu{"a", "v", "u"};
        ixm::set(conn, md, entity_type::user, env.rodsUserName);
        auto res = ixm::get(conn, entity_type::user, env.rodsUserName);
        REQUIRE(res.size() == 1);

        ixm::remove(conn, md, entity_type::user, env.rodsUserName);
        res = ixm::get(conn, entity_type::user, env.rodsUserName);
        REQUIRE(res.size() == 0);
    }

    SECTION("user set")
    {
        auto md = ixm::avu{"a", "v", "u"};
        ixm::set(conn, md, entity_type::user, env.rodsUserName);
        auto res = ixm::get(conn, entity_type::user, env.rodsUserName);
        REQUIRE(res[0] == md);
        ixm::remove(conn, md, entity_type::user, env.rodsUserName);
    }

    SECTION("user add")
    {
        auto md   = ixm::avu{"a", "v", "u"};
        ixm::add(conn, md, entity_type::user, env.rodsUserName);
        auto res = ixm::get(conn, entity_type::user, env.rodsUserName);
        REQUIRE(res[0] == md);
        ixm::remove(conn, md, entity_type::user, env.rodsUserName);
    }

    SECTION("user modify")
    {
        auto md  = ixm::avu{"a", "v", "u"};
        auto md2 = ixm::avu{"a2", "v2", "u2"};

        ixm::add(conn, md, entity_type::user, env.rodsUserName);
        auto res = ixm::get(conn, entity_type::user, env.rodsUserName);
        REQUIRE(res[0] == md);

        ixm::modify(conn, md, md2, entity_type::user, env.rodsUserName);
        res = ixm::get(conn, entity_type::user, env.rodsUserName);
        REQUIRE(res[0] == md2);

        ixm::remove(conn, md2, entity_type::user, env.rodsUserName);
    }

    SECTION("resource remove")
    {
        auto md = ixm::avu{"a", "v", "u"};
        ixm::set(conn, md, entity_type::resource, "demoResc");
        auto res = ixm::get(conn, entity_type::resource, "demoResc");
        REQUIRE(res.size() == 1);

        ixm::remove(conn, md, entity_type::resource, "demoResc");
        res = ixm::get(conn, entity_type::resource, "demoResc");
        REQUIRE(res.size() == 0);
    }

    SECTION("resource set")
    {
        auto md = ixm::avu{"a", "v", "u"};
        ixm::set(conn, md, entity_type::resource, "demoResc");
        auto res = ixm::get(conn, entity_type::resource, "demoResc");
        REQUIRE(res[0] == md);
        ixm::remove(conn, md, entity_type::resource, "demoResc");
    }

    SECTION("resource add")
    {
        auto md   = ixm::avu{"a", "v", "u"};
        ixm::add(conn, md, entity_type::resource, "demoResc");
        auto res = ixm::get(conn, entity_type::resource, "demoResc");
        REQUIRE(res[0] == md);
        ixm::remove(conn, md, entity_type::resource, "demoResc");
    }

    SECTION("resource modify")
    {
        auto md  = ixm::avu{"a", "v", "u"};
        auto md2 = ixm::avu{"a2", "v2", "u2"};

        ixm::add(conn, md, entity_type::resource, "demoResc");
        auto res = ixm::get(conn, entity_type::resource, "demoResc");
        REQUIRE(res[0] == md);

        ixm::modify(conn, md, md2, entity_type::resource, "demoResc");
        res = ixm::get(conn, entity_type::resource, "demoResc");
        REQUIRE(res[0] == md2);

        ixm::remove(conn, md2, entity_type::resource, "demoResc");
    }

}

