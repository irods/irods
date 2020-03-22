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

#include "filesystem/filesystem.hpp"
#include "rodsClient.h"

#include "connection_pool.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "filesystem.hpp"
#include "irods_at_scope_exit.hpp"

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
#include <array>

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

    SECTION("copy data objects and collections")
    {
        REQUIRE(fs::client::create_collections(conn, sandbox / "dir/subdir"));

        {
            default_transport tp{conn};        
            odstream{tp, sandbox / "file1.txt"};
        }

        REQUIRE(fs::client::exists(conn, sandbox / "file1.txt"));

        REQUIRE_NOTHROW(fs::client::copy(conn, sandbox / "file1.txt", sandbox / "file2.txt"));
        REQUIRE_NOTHROW(fs::client::copy(conn, sandbox / "dir", sandbox / "dir2"));

        const auto copy_of_sandbox = fs::path{env.rodsHome} / "copy_of_sandbox";
        REQUIRE_NOTHROW(fs::client::copy(conn, sandbox, copy_of_sandbox, fs::copy_options::recursive));

        REQUIRE(fs::client::remove_all(conn, copy_of_sandbox, fs::remove_options::no_trash));
    }

    SECTION("renaming data objects and collections")
    {
        const auto from = sandbox / "from";
        REQUIRE(fs::client::create_collections(conn, from));
        REQUIRE(fs::client::exists(conn, from));

        const auto to = sandbox / "to";
        REQUIRE(fs::client::create_collections(conn, to));
        REQUIRE(fs::client::exists(conn, to));

        const auto d1 = from / "d1.txt";

        {
            default_transport tp{conn};        
            odstream{tp, d1};
        }

        REQUIRE(fs::client::exists(conn, d1));

        REQUIRE_THROWS(fs::client::rename(conn, d1, fs::path{to} += '/'));
        REQUIRE_NOTHROW(fs::client::rename(conn, d1, to / "d2.txt"));
        REQUIRE_THROWS(fs::client::rename(conn, from, to));
        REQUIRE_NOTHROW(fs::client::rename(conn, from, to / "sub_collection"));
    }

    SECTION("create and remove collections")
    {
        const fs::path col1 = sandbox / "col1";
        REQUIRE(fs::client::create_collection(conn, col1));
        REQUIRE(fs::client::create_collection(conn, sandbox / "col2", col1));
        REQUIRE(fs::client::create_collections(conn, sandbox / "col2/col3/col4/col5"));
        REQUIRE(fs::client::remove(conn, col1));
        REQUIRE(fs::client::remove_all(conn, sandbox / "col2/col3/col4"));
        REQUIRE(fs::client::remove_all(conn, sandbox / "col2", fs::remove_options::no_trash));
    }

    SECTION("existence checking")
    {
        REQUIRE(fs::client::exists(conn, sandbox));
        REQUIRE(fs::client::exists(fs::client::status(conn, sandbox)));

        REQUIRE_FALSE(fs::client::exists(conn, sandbox / "bogus"));
        REQUIRE_FALSE(fs::client::exists(fs::client::status(conn, sandbox / "bogus")));
    }

    SECTION("equivalence checking")
    {
        const auto p = sandbox / ".." / *std::rbegin(sandbox);
        REQUIRE_THROWS(fs::client::equivalent(conn, sandbox, p));
        REQUIRE(fs::client::equivalent(conn, sandbox, p.lexically_normal()));
    }

    SECTION("data object size and checksum")
    {
        const fs::path p = sandbox / "data_object";

        {
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        REQUIRE(fs::client::exists(conn, p));
        REQUIRE(fs::client::data_object_size(conn, p) == 12);
        REQUIRE(fs::client::data_object_checksum(conn, p, fs::replica_number::all).size() == 1);
        REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
    }

    SECTION("collection modification times")
    {
        using namespace std::chrono_literals;

        const fs::path col = sandbox / "mtime_col.d";
        REQUIRE(fs::client::create_collection(conn, col));

        const auto old_mtime = fs::client::last_write_time(conn, col);
        std::this_thread::sleep_for(2s);

        using clock_type = fs::object_time_type::clock;
        using duration_type = fs::object_time_type::duration;

        const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());
        REQUIRE(old_mtime != now);

        fs::client::last_write_time(conn, sandbox, now);
        const auto updated = fs::client::last_write_time(conn, sandbox);
        REQUIRE(updated == now);

        REQUIRE(fs::client::remove(conn, col, fs::remove_options::no_trash));
    }

    SECTION("data object modification times")
    {
        using namespace std::chrono_literals;

        const fs::path p = sandbox / "data_object";

        {
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        const auto old_mtime = fs::client::last_write_time(conn, p);
        std::this_thread::sleep_for(2s);

        using clock_type = fs::object_time_type::clock;
        using duration_type = fs::object_time_type::duration;

        const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());
        REQUIRE(old_mtime != now);

        fs::client::last_write_time(conn, p, now);
        const auto updated = fs::client::last_write_time(conn, p);
        REQUIRE(updated == now);

        REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
    }

    SECTION("read/modify permissions on a data object")
    {
        const fs::path p = sandbox / "data_object";

        {
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        const auto permissions_match = [&env](const auto& _entity_perms, const auto& _expected_perms)
        {
            REQUIRE(_entity_perms.name == env.rodsUserName);
            REQUIRE(_entity_perms.prms == _expected_perms);
        };

        auto status = fs::client::status(conn, p);

        REQUIRE_FALSE(status.permissions().empty());
        permissions_match(status.permissions()[0], fs::perms::own);

        auto new_perms = fs::perms::read;
        fs::client::permissions(conn, p, env.rodsUserName, new_perms);
        status = fs::client::status(conn, p);
        REQUIRE_FALSE(status.permissions().empty());
        permissions_match(status.permissions()[0], new_perms);

        new_perms = fs::perms::own;
        fs::client::permissions(conn, p, env.rodsUserName, new_perms);
        status = fs::client::status(conn, p);
        REQUIRE_FALSE(status.permissions().empty());
        permissions_match(status.permissions()[0], new_perms);

        REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
    }

    SECTION("collection iterators")
    {
        // Creates three data objects under the path "_collection".
        const auto create_data_objects_under_collection = [&conn](const fs::path& _collection)
        {
            // Create new data objects.
            for (auto&& e : {"f1.txt", "f2.txt", "f3.txt"}) {
                default_transport tp{conn};
                odstream{tp, _collection / e} << "test file";
            }
        };

        create_data_objects_under_collection(sandbox);

        // Create two collections.
        const auto col1 = sandbox / "col1.d";
        REQUIRE(fs::client::create_collection(conn, col1));

        const auto col2 = sandbox / "col2.d";
        REQUIRE(fs::client::create_collection(conn, col2));

        create_data_objects_under_collection(col1);

        SECTION("non-recursive collection iterator")
        {
            // Capture the results of the iterator in a vector.
            std::vector<std::string> entries;

            for (auto&& e : fs::client::collection_iterator{conn, sandbox}) {
                entries.push_back(e.path().string());
            }

            std::sort(std::begin(entries), std::end(entries));

            // The sorted list of paths that the "entries" vector must match.
            const std::vector expected_entries{
                col1.string(),
                col2.string(),
                (sandbox / "f1.txt").string(),
                (sandbox / "f2.txt").string(),
                (sandbox / "f3.txt").string()
            };

            REQUIRE(expected_entries == entries);
        }

        SECTION("recursive collection iterator")
        {
            // Capture the results of the iterator in a vector.
            std::vector<std::string> entries;

            for (auto&& e : fs::client::recursive_collection_iterator{conn, sandbox}) {
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
                (sandbox / "f1.txt").string(),
                (sandbox / "f2.txt").string(),
                (sandbox / "f3.txt").string()
            };

            REQUIRE(expected_entries == entries);
        }

        // Clean-up.
        REQUIRE(fs::client::remove(conn, sandbox / "f1.txt", fs::remove_options::no_trash));
        REQUIRE(fs::client::remove(conn, sandbox / "f2.txt", fs::remove_options::no_trash));
        REQUIRE(fs::client::remove(conn, sandbox / "f3.txt", fs::remove_options::no_trash));
        REQUIRE(fs::client::remove_all(conn, col1, fs::remove_options::no_trash));
        REQUIRE(fs::client::remove_all(conn, col2, fs::remove_options::no_trash));

        SECTION("iteration over an empty collection")
        {
            const auto p = sandbox / "empty.d";
            REQUIRE(fs::client::create_collection(conn, p));

            const auto count_entries = [](auto& iter)
            {
                int count = 0;

                for (auto&& e : iter) {
                    static_cast<void>(e);
                    ++count;
                }

                REQUIRE(0 == count);
            };

            fs::client::collection_iterator iter{conn, p};
            REQUIRE(fs::client::collection_iterator{} == iter);
            count_entries(iter);

            fs::client::recursive_collection_iterator recursive_iter{conn, p};
            REQUIRE(fs::client::recursive_collection_iterator{} == recursive_iter);
            count_entries(recursive_iter);

            REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
        }

        SECTION("trailing path separators are ignored when iterating over a collection")
        {
            auto p = sandbox;
            p += fs::path::preferred_separator;
            for (auto&& e : fs::client::collection_iterator{conn, p}) { static_cast<void>(e); };
            for (auto&& e : fs::client::recursive_collection_iterator{conn, p}) { static_cast<void>(e); };
        }

        SECTION("collection iterators throw an exception when passed a data object path")
        {
            const auto p = sandbox / "foo";

            default_transport tp{conn};
            odstream{tp, p} << "test file";
            REQUIRE(fs::client::exists(conn, p));

            const auto* expected_msg = "could not open collection for reading [handle => -834000]";
            REQUIRE_THROWS(fs::client::collection_iterator{conn, p}, expected_msg);
            REQUIRE_THROWS(fs::client::recursive_collection_iterator{conn, p}, expected_msg);
        }
    }

    SECTION("object type checking")
    {
        REQUIRE(fs::client::is_collection(conn, sandbox));
        REQUIRE_FALSE(fs::client::is_data_object(conn, sandbox));

        const fs::path p = sandbox / "data_object";

        {
            default_transport tp{conn};
            odstream{tp, p};
        }

        REQUIRE(fs::client::is_data_object(conn, p));
        REQUIRE_FALSE(fs::client::is_collection(conn, p));
        REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
    }

    SECTION("metadata management")
    {
        SECTION("collections")
        {
            const std::array<fs::metadata, 3> metadata{{
                {"n1", "v1", "u1"},
                {"n2", "v2", "u2"},
                {"n3", "v3", "u3"}
            }};

            REQUIRE(fs::client::set_metadata(conn, sandbox, metadata[0]));
            REQUIRE(fs::client::set_metadata(conn, sandbox, metadata[1]));
            REQUIRE(fs::client::set_metadata(conn, sandbox, metadata[2]));

            const auto results = fs::client::get_metadata(conn, sandbox);
            REQUIRE_FALSE(results.empty());
            REQUIRE(std::is_permutation(std::begin(results), std::end(results), std::begin(metadata),
                                        [](const auto& _lhs, const auto& _rhs)
                                        {
                                            return _lhs.attribute == _rhs.attribute &&
                                                   _lhs.value == _rhs.value &&
                                                   _lhs.units == _rhs.units;
                                        }));

            REQUIRE(fs::client::remove_metadata(conn, sandbox, metadata[0]));
            REQUIRE(fs::client::remove_metadata(conn, sandbox, metadata[1]));
            REQUIRE(fs::client::remove_metadata(conn, sandbox, metadata[2]));
        }

        SECTION("data objects")
        {
            const fs::path p = sandbox / "data_object";

            {
                default_transport tp{conn};
                odstream{tp, p};
            }

            REQUIRE(fs::client::exists(conn, p));

            REQUIRE(fs::client::set_metadata(conn, p, {"n1", "v1", "u1"}));

            const auto results = fs::client::get_metadata(conn, p);
            REQUIRE_FALSE(results.empty());
            REQUIRE(results[0].attribute == "n1");
            REQUIRE(results[0].value == "v1");
            REQUIRE(results[0].units == "u1");

            REQUIRE(fs::client::remove_metadata(conn, p, {"n1", "v1", "u1"}));

            REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
        }
    }
}

