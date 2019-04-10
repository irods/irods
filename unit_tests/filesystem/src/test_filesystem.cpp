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
#include "transport/default_transport.hpp"

#include <functional>
#include <vector>
#include <iostream>
#include <variant>
#include <iterator>
#include <algorithm>

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

    using dstream = irods::experimental::io::dstream;
    using odstream = irods::experimental::io::odstream;
    using default_transport = irods::experimental::io::client::default_transport;

    const fs::path user_home = "/tempZone/home/rods";

    SECTION("copy data objects and collections")
    {
        const auto sandbox = user_home / "sandbox";
        REQUIRE(fs::client::create_collections(*comm, sandbox / "dir/subdir"));

        {
            default_transport tp{*comm};        
            dstream{tp, sandbox / "file1.txt"};
        }

        REQUIRE(fs::client::exists(*comm, sandbox / "file1.txt"));

        REQUIRE_NOTHROW(fs::client::copy(*comm, sandbox / "file1.txt", sandbox / "file2.txt"));
        REQUIRE_NOTHROW(fs::client::copy(*comm, sandbox / "dir", sandbox / "dir2"));

        const auto copy_of_sandbox = user_home / "copy_of_sandbox";
        REQUIRE_NOTHROW(fs::client::copy(*comm, sandbox, copy_of_sandbox, fs::copy_options::recursive));

        REQUIRE(fs::client::remove_all(*comm, sandbox, fs::remove_options::no_trash));
        REQUIRE(fs::client::remove_all(*comm, copy_of_sandbox, fs::remove_options::no_trash));
    }

    SECTION("renaming data objects and collections")
    {
        const auto sandbox = user_home / "sandbox";
        const auto from = sandbox / "from";
        REQUIRE(fs::client::create_collections(*comm, from));
        REQUIRE(fs::client::exists(*comm, from));

        const auto to = sandbox / "to";
        REQUIRE(fs::client::create_collections(*comm, to));
        REQUIRE(fs::client::exists(*comm, to));

        const auto d1 = from / "d1.txt";

        {
            default_transport tp{*comm};        
            dstream{tp, d1};
        }

        REQUIRE(fs::client::exists(*comm, d1));

        REQUIRE_THROWS(fs::client::rename(*comm, d1, fs::path{to} += '/'));
        REQUIRE_NOTHROW(fs::client::rename(*comm, d1, to / "d2.txt"));
        REQUIRE_THROWS(fs::client::rename(*comm, from, to));
        REQUIRE_NOTHROW(fs::client::rename(*comm, from, to / "sub_collection"));

        REQUIRE(fs::client::remove_all(*comm, sandbox, fs::remove_options::no_trash));
    }

    SECTION("create and remove collections")
    {
        const fs::path col1 = user_home / "col1";
        REQUIRE(fs::client::create_collection(*comm, col1));
        REQUIRE(fs::client::create_collection(*comm, "/tempZone/home/rods/col2", col1));
        REQUIRE(fs::client::create_collections(*comm, "/tempZone/home/rods/col2/col3/col4/col5"));
        REQUIRE(fs::client::remove(*comm, col1));
        REQUIRE(fs::client::remove_all(*comm, user_home / "col2/col3/col4"));
        REQUIRE(fs::client::remove_all(*comm, user_home / "col2", fs::remove_options::no_trash));
    }

    SECTION("existence checking")
    {
        REQUIRE(fs::client::exists(*comm, user_home));
        REQUIRE(fs::client::exists(fs::client::status(*comm, user_home)));

        REQUIRE_FALSE(fs::client::exists(*comm, user_home / "bogus"));
        REQUIRE_FALSE(fs::client::exists(fs::client::status(*comm, user_home / "bogus")));
    }

    SECTION("equivalence checking")
    {
        const fs::path p = user_home / "../rods";
        REQUIRE_THROWS(fs::client::equivalent(*comm, user_home, p));
        REQUIRE(fs::client::equivalent(*comm, user_home, p.lexically_normal()));
    }

    SECTION("data object size and checksum")
    {
        const fs::path p = user_home / "data_object";

        {
            default_transport tp{*comm};
            odstream{tp, p} << "hello world!";
        }

        REQUIRE(fs::client::exists(*comm, p));
        REQUIRE(fs::client::data_object_size(*comm, p) == 12);
        REQUIRE(fs::client::data_object_checksum(*comm, p, fs::replica_number::all).size() == 1);
        REQUIRE(fs::client::remove(*comm, p, fs::remove_options::no_trash));
    }

    SECTION("collection iterators")
    {
        // Creates three data objects under at the path "_collection".
        const auto create_data_objects_under_collection = [comm](const fs::path& _collection)
        {
            // Create new data objects.
            for (auto&& e : {"f1.txt", "f2.txt", "f3.txt"}) {
                default_transport tp{*comm};
                odstream{tp, _collection / e} << "test file";
            }
        };

        create_data_objects_under_collection(user_home);

        // Create two collections.
        const auto col1 = user_home / "col1.d";
        REQUIRE(fs::client::create_collection(*comm, col1));

        const auto col2 = user_home / "col2.d";
        REQUIRE(fs::client::create_collection(*comm, col2));

        create_data_objects_under_collection(col1);

        SECTION("non-recursive collection iterator")
        {
            // Capture the results of the iterator in a vector.
            std::vector<std::string> entries;

            for (auto&& e : fs::client::collection_iterator{*comm, user_home}) {
                entries.push_back(e.path().string());
            }

            std::sort(std::begin(entries), std::end(entries));

            // The sorted list of paths that the "entries" vector must match.
            const std::vector expected_entries{
                col1.string(),
                col2.string(),
                (user_home / "f1.txt").string(),
                (user_home / "f2.txt").string(),
                (user_home / "f3.txt").string()
            };

            REQUIRE(expected_entries == entries);
        }

        SECTION("recursive collection iterator")
        {
            // Capture the results of the iterator in a vector.
            std::vector<std::string> entries;

            for (auto&& e : fs::client::recursive_collection_iterator{*comm, user_home}) {
                entries.push_back(e.path().string());
            }

            std::sort(std::begin(entries), std::end(entries));

            // The sorted list of paths that the "entries" vector must match.
            const std::vector expected_entries{
                col1.string(),
                (col1 / "f1.txt").string(),
                (col1 / "f2.txt").string(),
                (col1 / "f3.txt").string(),
                col2.string(),
                (user_home / "f1.txt").string(),
                (user_home / "f2.txt").string(),
                (user_home / "f3.txt").string()
            };

            REQUIRE(expected_entries == entries);
        }

        // Clean-up.
        REQUIRE(fs::client::remove(*comm, user_home / "f1.txt", fs::remove_options::no_trash));
        REQUIRE(fs::client::remove(*comm, user_home / "f2.txt", fs::remove_options::no_trash));
        REQUIRE(fs::client::remove(*comm, user_home / "f3.txt", fs::remove_options::no_trash));
        REQUIRE(fs::client::remove_all(*comm, col1, fs::remove_options::no_trash));
        REQUIRE(fs::client::remove_all(*comm, col2, fs::remove_options::no_trash));
    }

    SECTION("object type checking")
    {
        REQUIRE(fs::client::is_collection(*comm, user_home));
        REQUIRE_FALSE(fs::client::is_data_object(*comm, user_home));

        const fs::path p = user_home / "data_object";

        {
            default_transport tp{*comm};
            dstream{tp, p};
        }

        REQUIRE(fs::client::is_data_object(*comm, p));
        REQUIRE_FALSE(fs::client::is_collection(*comm, p));
        REQUIRE(fs::client::remove(*comm, p, fs::remove_options::no_trash));
    }

    SECTION("metadata management")
    {
        const fs::path p = user_home / "data_object";

        {
            default_transport tp{*comm};
            dstream{tp, p};
        }

        REQUIRE(fs::client::exists(*comm, p));

        REQUIRE(fs::client::set_metadata(*comm, p, {"n1", "v1", "u1"}));

        const auto results = fs::client::get_metadata(*comm, p);
        REQUIRE_FALSE(results.empty());
        REQUIRE(results[0].attribute == "n1");
        REQUIRE(results[0].value == "v1");
        REQUIRE(results[0].units == "u1");

        REQUIRE(fs::client::remove_metadata(*comm, p, {"n1", "v1", "u1"}));

        REQUIRE(fs::client::remove(*comm, p, fs::remove_options::no_trash));
    }
}

