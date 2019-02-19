//
// The following unit tests were implemented based on code examples from
// "cppreference.com". This code is licensed under the following:
//
//   - Creative Commons Attribution-Sharealike 3.0 Unported License (CC-BY-SA)
//   - GNU Free Documentation License (GFDL)
//
// For more information about these licenses, visit:
//   
//   - https://en.cppreference.com/w/Cppreference:FAQ
//   - https://en.cppreference.com/w/Cppreference:Copyright/CC-BY-SA
//   - https://en.cppreference.com/w/Cppreference:Copyright/GDFL
//

#include "catch.hpp"

#include "filesystem.hpp"
#include "rodsClient.h"
#include "irods_at_scope_exit.hpp"

#include "dstream.hpp"

#include <functional>
#include <vector>
#include <iostream>
#include <variant>

TEST_CASE("filesystem")
{
    rodsEnv env;
    REQUIRE(getRodsEnv(&env) >= 0);

    rErrMsg_t errors;
    auto* comm = rcConnect(env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, 0, &errors);
    REQUIRE(comm);
    irods::at_scope_exit<std::function<void()>> at_scope_exit{[comm] {
        rcDisconnect(comm);
    }};

    char passwd[] = "rods";
    REQUIRE(clientLoginWithPassword(comm, passwd) == 0);

    namespace fs = irods::experimental::filesystem;

    const fs::path user_home = "/tempZone/home/rods";

    SECTION("copy data objects and collections")
    {
        const auto sandbox = user_home / "sandbox";
        REQUIRE(fs::create_collections(*comm, sandbox / "dir/subdir"));

        { irods::experimental::dstream{*comm, sandbox / "file1.txt"}; }
        REQUIRE(fs::exists(*comm, sandbox / "file1.txt"));

        REQUIRE_NOTHROW(fs::copy(*comm, sandbox / "file1.txt", sandbox / "file2.txt"));
        REQUIRE_NOTHROW(fs::copy(*comm, sandbox / "dir", sandbox / "dir2"));

        const auto copy_of_sandbox = user_home / "copy_of_sandbox";
        REQUIRE_NOTHROW(fs::copy(*comm, sandbox, copy_of_sandbox, fs::copy_options::recursive));

        REQUIRE(fs::remove_all(*comm, sandbox, fs::remove_options::no_trash));
        REQUIRE(fs::remove_all(*comm, copy_of_sandbox, fs::remove_options::no_trash));
    }

    SECTION("renaming data objects and collections")
    {
        const auto sandbox = user_home / "sandbox";
        const auto from = sandbox / "from";
        REQUIRE(fs::create_collections(*comm, from));
        REQUIRE(fs::exists(*comm, from));

        const auto to = sandbox / "to";
        REQUIRE(fs::create_collections(*comm, to));
        REQUIRE(fs::exists(*comm, to));

        const auto d1 = from / "d1.txt";
        { irods::experimental::dstream{*comm, d1}; }
        REQUIRE(fs::exists(*comm, d1));

        REQUIRE_THROWS(fs::rename(*comm, d1, fs::path{to} += '/'));
        REQUIRE_NOTHROW(fs::rename(*comm, d1, to / "d2.txt"));
        REQUIRE_THROWS(fs::rename(*comm, from, to));
        REQUIRE_NOTHROW(fs::rename(*comm, from, to / "sub_collection"));

        REQUIRE(fs::remove_all(*comm, sandbox, fs::remove_options::no_trash));
    }

    SECTION("create and remove collections")
    {
        const fs::path col1 = user_home / "col1";
        REQUIRE(fs::create_collection(*comm, col1));
        REQUIRE(fs::create_collection(*comm, "/tempZone/home/rods/col2", col1));
        REQUIRE(fs::create_collections(*comm, "/tempZone/home/rods/col2/col3/col4/col5"));
        REQUIRE(fs::remove(*comm, col1));
        REQUIRE(fs::remove_all(*comm, user_home / "col2/col3/col4"));
        REQUIRE(fs::remove_all(*comm, user_home / "col2", fs::remove_options::no_trash));
    }

    SECTION("existence checking")
    {
        REQUIRE(fs::exists(*comm, user_home));
        REQUIRE(fs::exists(fs::status(*comm, user_home)));

        REQUIRE_FALSE(fs::exists(*comm, user_home / "bogus"));
        REQUIRE_FALSE(fs::exists(fs::status(*comm, user_home / "bogus")));
    }

    SECTION("equivalence checking")
    {
        const fs::path p = user_home / "../rods";
        REQUIRE_THROWS(fs::equivalent(*comm, user_home, p));
        REQUIRE(fs::equivalent(*comm, user_home, p.lexically_normal()));
    }

    SECTION("data object size and checksum")
    {
        const fs::path p = user_home / "data_object";
        { irods::experimental::odstream{*comm, p} << "hello world!"; }
        REQUIRE(fs::exists(*comm, p));
        REQUIRE(fs::data_object_size(*comm, p) == 12);
        REQUIRE(fs::data_object_checksum(*comm, p, fs::replica_number::all).size() == 1);
        REQUIRE(fs::remove(*comm, p, fs::remove_options::no_trash));
    }

    SECTION("object type checking")
    {
        REQUIRE(fs::is_collection(*comm, user_home));
        REQUIRE_FALSE(fs::is_data_object(*comm, user_home));

        const fs::path p = user_home / "data_object";
        { irods::experimental::dstream{*comm, p}; }
        REQUIRE(fs::is_data_object(*comm, p));
        REQUIRE_FALSE(fs::is_collection(*comm, p));
        REQUIRE(fs::remove(*comm, p, fs::remove_options::no_trash));
    }

    SECTION("metadata management")
    {
        const fs::path p = user_home / "data_object";
        { irods::experimental::dstream{*comm, p}; }
        REQUIRE(fs::exists(*comm, p));

        REQUIRE(fs::set_metadata(*comm, p, {"n1", "v1", "u1"}));

        const auto results = fs::get_metadata(*comm, p);
        REQUIRE_FALSE(results.empty());
        REQUIRE(results[0].attribute == "n1");
        REQUIRE(results[0].value == "v1");
        REQUIRE(results[0].units == "u1");

        REQUIRE(fs::remove_metadata(*comm, p, {"n1", "v1", "u1"}));

        REQUIRE(fs::remove(*comm, p, fs::remove_options::no_trash));
    }
}

