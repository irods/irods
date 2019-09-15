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

#include "rodsClient.h"

#include "connection_pool.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "filesystem.hpp"

#include "dstream.hpp"
#include "transport/default_transport.hpp"

#include <functional>
#include <vector>
#include <iostream>
#include <variant>
#include <iterator>
#include <algorithm>
#include <thread>
#include <chrono>

TEST_CASE("filesystem")
{
    auto& api_table = irods::get_client_api_table();
    auto& pck_table = irods::get_pack_table();
    init_api_table(api_table, pck_table);

    rodsEnv env;
    REQUIRE(getRodsEnv(&env) >= 0);

    const auto connection_count = 1;
    const auto refresh_time = 600;

    irods::connection_pool conn_pool{connection_count,
                                     env.rodsHost,
                                     env.rodsPort,
                                     env.rodsUserName,
                                     env.rodsZone,
                                     refresh_time};

    auto* comm = &static_cast<rcComm_t&>(conn_pool.get_connection());

    // clang-format off
    namespace fs = irods::experimental::filesystem;

    using dstream           = irods::experimental::io::dstream;
    using odstream          = irods::experimental::io::odstream;
    using default_transport = irods::experimental::io::client::default_transport;
    // clang-format on

    const fs::path user_home = env.rodsHome;

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
        REQUIRE(fs::client::create_collection(*comm, user_home / "col2", col1));
        REQUIRE(fs::client::create_collections(*comm, user_home / "col2/col3/col4/col5"));
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
        const fs::path p = user_home / ".." / env.rodsUserName;
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

    SECTION("collection modification times")
    {
        using namespace std::chrono_literals;

        const fs::path col = user_home / "mtime_col.d";
        REQUIRE(fs::client::create_collection(*comm, col));

        const auto old_mtime = fs::client::last_write_time(*comm, col);
        std::this_thread::sleep_for(2s);

        using clock_type = fs::object_time_type::clock;
        using duration_type = fs::object_time_type::duration;

        const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());
        REQUIRE(old_mtime != now);

        fs::client::last_write_time(*comm, user_home, now);
        const auto updated = fs::client::last_write_time(*comm, user_home);
        REQUIRE(updated == now);

        REQUIRE(fs::client::remove(*comm, col, fs::remove_options::no_trash));
    }

    SECTION("data object modification times")
    {
        using namespace std::chrono_literals;

        const fs::path p = user_home / "data_object";

        {
            default_transport tp{*comm};
            odstream{tp, p} << "hello world!";
        }

        const auto old_mtime = fs::client::last_write_time(*comm, p);
        std::this_thread::sleep_for(2s);

        using clock_type = fs::object_time_type::clock;
        using duration_type = fs::object_time_type::duration;

        const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());
        REQUIRE(old_mtime != now);

        fs::client::last_write_time(*comm, p, now);
        const auto updated = fs::client::last_write_time(*comm, p);
        REQUIRE(updated == now);

        REQUIRE(fs::client::remove(*comm, p, fs::remove_options::no_trash));
    }

    SECTION("read/modify permissions on a data object")
    {
        const fs::path p = user_home / "data_object";

        {
            default_transport tp{*comm};
            odstream{tp, p} << "hello world!";
        }

        const auto permissions_match = [&env](const auto& _entity_perms, const auto& _expected_perms)
        {
            REQUIRE(_entity_perms.name == env.rodsUserName);
            REQUIRE(_entity_perms.prms == _expected_perms);
        };

        auto status = fs::client::status(*comm, p);

        REQUIRE_FALSE(status.permissions().empty());
        permissions_match(status.permissions()[0], fs::perms::own);

        auto new_perms = fs::perms::read;
        fs::client::permissions(*comm, p, env.rodsUserName, new_perms);
        status = fs::client::status(*comm, p);
        REQUIRE_FALSE(status.permissions().empty());
        permissions_match(status.permissions()[0], new_perms);

        new_perms = fs::perms::own;
        fs::client::permissions(*comm, p, env.rodsUserName, new_perms);
        status = fs::client::status(*comm, p);
        REQUIRE_FALSE(status.permissions().empty());
        permissions_match(status.permissions()[0], new_perms);

        REQUIRE(fs::client::remove(*comm, p, fs::remove_options::no_trash));
    }

    SECTION("collection iterators")
    {
        // Creates three data objects under the path "_collection".
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

