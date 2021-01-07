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
#include "dataObjRepl.h"
#include "rcMisc.h"

#include "client_connection.hpp"
#include "connection_pool.hpp"
#include "filesystem.hpp"
#include "resource_administration.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_query.hpp"
#include "replica.hpp"

#include "dstream.hpp"
#include "transport/default_transport.hpp"

#include "boost/filesystem.hpp"
#include "fmt/format.h"

#include <unistd.h>

#include <cstring>
#include <cstdlib>
#include <functional>
#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <thread>
#include <chrono>
#include <array>
#include <string>
#include <string_view>

auto get_hostname() noexcept -> std::string;

auto create_resource_vault(const std::string_view _vault_name) -> boost::filesystem::path;

auto add_ufs_resource(const std::string_view _resc_name, const std::string_view _vault_name) -> void;

auto replicate_data_object(const std::string_view _path, const std::string_view _resc_name) -> void;

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

    irods::at_scope_exit<std::function<void()>> remove_sandbox{[&conn, &sandbox] {
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
        REQUIRE(fs::client::data_object_checksum(conn, p) == "");

        const std::string_view ufs_resc = "unit_test_ufs";

        add_ufs_resource(ufs_resc, "irods_unit_testing_vault");

        namespace adm = irods::experimental::administration;

        irods::at_scope_exit remove_resources{[&ufs_resc] {
            irods::experimental::client_connection conn;
            adm::client::remove_resource(conn, ufs_resc);
        }};

        replicate_data_object(p.c_str(), ufs_resc);

        // Sleep for a few seconds so that the mtime is guaranteed to be different
        // for each replica.
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);

        {
            // Append some new data to the second replica (this also causes the mtime
            // to be updated for the replica).
            namespace io = irods::experimental::io;
            irods::experimental::client_connection conn;
            default_transport tp{conn};
            odstream{tp, p, io::replica_number{1}, std::ios_base::app} << "  This was appended.";
        }

        irods::experimental::client_connection conn;
        REQUIRE(fs::client::data_object_size(conn, p) == 32);

        const auto replica_number = 1;
        const auto checksum = irods::experimental::replica::replica_checksum<rcComm_t>(conn, p, replica_number);
        REQUIRE(fs::client::data_object_checksum(conn, p) == checksum);

        REQUIRE(fs::client::remove(conn, p, fs::remove_options::no_trash));
    }

    SECTION("updating the mtime of a data object is not supported")
    {
        const fs::path p = sandbox / "data_object";

        {
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        REQUIRE_THROWS([&] {
            // clang-format off
            using clock_type    = fs::object_time_type::clock;
            using duration_type = fs::object_time_type::duration;
            // clang-format on

            const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());
            fs::client::last_write_time(conn, p, now);
        }(), "path does not point to a collection");
    }

    SECTION("collection mtime")
    {
        using namespace std::chrono_literals;

        const fs::path col = sandbox / "mtime_col.d";
        REQUIRE(fs::client::create_collection(conn, col));

        const auto old_mtime = fs::client::last_write_time(conn, col);
        std::this_thread::sleep_for(2s);

        // clang-format off
        using clock_type    = fs::object_time_type::clock;
        using duration_type = fs::object_time_type::duration;
        // clang-format on

        const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());
        REQUIRE(old_mtime != now);

        fs::client::last_write_time(conn, sandbox, now);
        const auto updated = fs::client::last_write_time(conn, sandbox);
        REQUIRE(updated == now);

        REQUIRE(fs::client::remove(conn, col, fs::remove_options::no_trash));
    }

    SECTION("fetch data object mtime")
    {
        using namespace std::chrono_literals;

        const fs::path p = sandbox / "data_object";

        {
            irods::experimental::client_connection conn;
            default_transport tp{conn};
            odstream{tp, p} << "hello world!";
        }

        const auto first_replica_mtime = fs::client::last_write_time(conn, p);
        std::this_thread::sleep_for(2s);

        const std::string_view ufs_resc = "unit_test_ufs";

        add_ufs_resource(ufs_resc, "irods_unit_testing_vault");

        namespace adm = irods::experimental::administration;

        irods::at_scope_exit remove_resources{[&ufs_resc] {
            irods::experimental::client_connection conn;
            adm::client::remove_resource(conn, ufs_resc);
        }};

        replicate_data_object(p.c_str(), ufs_resc);

        using duration_type = fs::object_time_type::duration;

        // Get the mtime of the second replica.
        const auto second_replica_mtime = [&p] {
            const auto gql = fmt::format("select DATA_MODIFY_TIME "
                                         "where"
                                         " COLL_NAME = '{}' and"
                                         " DATA_NAME = '{}' and"
                                         " DATA_REPL_NUM = '1'",
                                         p.parent_path().c_str(),
                                         p.object_name().c_str());

            irods::experimental::client_connection conn;
            irods::query query{static_cast<rcComm_t*>(conn), gql};

            return fs::object_time_type{duration_type{std::stoull(query.front()[0])}};
        }();

        REQUIRE(first_replica_mtime < second_replica_mtime);

        irods::experimental::client_connection conn;
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
            const std::vector<std::string> expected_entries{
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
            const std::vector<std::string> expected_entries{
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
        SECTION("basic operations")
        {
            const fs::path p = sandbox / "data_object.a";

            {
                default_transport tp{conn};
                odstream{tp, p};
            }

            fs::metadata md{"n1", "v1", "u1"};
            REQUIRE_NOTHROW(fs::client::set_metadata(conn, p, md));

            irods::at_scope_exit remove_metadata{[&] {
                for (auto&& md : {fs::metadata{"n1", "v1", "u1"},
                                              {"n1", "v2", "u2"},
                                              {"n1", "v2", "u1"},
                                              {"n1", "v1", "u2"}})
                {
                    try { fs::client::remove_metadata(conn, p, md); } catch (...) {}
                }
            }};

            const auto results = fs::client::get_metadata(conn, p);
            REQUIRE(results.size() == 1);
            REQUIRE(results[0].attribute == md.attribute);
            REQUIRE(results[0].value == md.value);
            REQUIRE(results[0].units == md.units);

            SECTION("set operation updates metadata attached to a single object")
            {
                md.value = "v2";
                md.units = "u2";
                REQUIRE_NOTHROW(fs::client::set_metadata(conn, p, md));

                const auto results = fs::client::get_metadata(conn, p);
                REQUIRE(results.size() == 1);
                REQUIRE(results[0].attribute == md.attribute);
                REQUIRE(results[0].value == md.value);
                REQUIRE(results[0].units == md.units);
            }

            SECTION("set operation attaches new metadata and detaches old metadata if existing metadata is attached to multiple objects")
            {
                const fs::path q = sandbox / "data_object.b";

                {
                    default_transport tp{conn};
                    odstream{tp, q};
                }

                REQUIRE_NOTHROW(fs::client::set_metadata(conn, q, md));

                auto results = fs::client::get_metadata(conn, q);
                REQUIRE(results.size() == 1);
                REQUIRE(results[0].attribute == md.attribute);
                REQUIRE(results[0].value == md.value);
                REQUIRE(results[0].units == md.units);

                md.value = "v2";
                md.units = "u2";
                REQUIRE_NOTHROW(fs::client::set_metadata(conn, p, md));
                REQUIRE(fs::client::get_metadata(conn, p).size() == 1);
            }

            SECTION("add operation allows reuse of attribute names when the value or units result in unique metadata")
            {
                md.value = "v2";
                REQUIRE_NOTHROW(fs::client::add_metadata(conn, p, md));
                REQUIRE(fs::client::get_metadata(conn, p).size() == 2);

                md.value = "v1";
                md.units = "u2";
                REQUIRE_NOTHROW(fs::client::add_metadata(conn, p, md));
                REQUIRE(fs::client::get_metadata(conn, p).size() == 3);
            }

            SECTION("remove operation")
            {
                REQUIRE_NOTHROW(fs::client::remove_metadata(conn, p, md));
                REQUIRE(fs::client::get_metadata(conn, p).empty());

                REQUIRE_NOTHROW(fs::client::set_metadata(conn, sandbox, md));
                REQUIRE_NOTHROW(fs::client::remove_metadata(conn, sandbox, md));
                REQUIRE(fs::client::get_metadata(conn, sandbox).empty());
            }
        }

        SECTION("collections")
        {
            const std::array<fs::metadata, 3> metadata{{
                {"n1", "v1", "u1"},
                {"n2", "v2", "u2"},
                {"n3", "v3", "u3"}
            }};

            REQUIRE_NOTHROW(fs::client::set_metadata(conn, sandbox, metadata[0]));
            REQUIRE_NOTHROW(fs::client::set_metadata(conn, sandbox, metadata[1]));
            REQUIRE_NOTHROW(fs::client::set_metadata(conn, sandbox, metadata[2]));

            const auto results = fs::client::get_metadata(conn, sandbox);
            REQUIRE_FALSE(results.empty());
            REQUIRE(std::is_permutation(std::begin(results), std::end(results), std::begin(metadata),
                                        [](const auto& _lhs, const auto& _rhs)
                                        {
                                            return _lhs.attribute == _rhs.attribute &&
                                                   _lhs.value == _rhs.value &&
                                                   _lhs.units == _rhs.units;
                                        }));

            REQUIRE_NOTHROW(fs::client::remove_metadata(conn, sandbox, metadata[0]));
            REQUIRE_NOTHROW(fs::client::remove_metadata(conn, sandbox, metadata[1]));
            REQUIRE_NOTHROW(fs::client::remove_metadata(conn, sandbox, metadata[2]));
            REQUIRE(fs::client::get_metadata(conn, sandbox).empty());
        }

        SECTION("data objects")
        {
            const fs::path p = sandbox / "data_object";

            {
                default_transport tp{conn};
                odstream{tp, p};
            }

            fs::metadata md{"n1", "v1", "u1"};
            REQUIRE_NOTHROW(fs::client::set_metadata(conn, p, md));

            const auto results = fs::client::get_metadata(conn, p);
            REQUIRE_FALSE(results.empty());
            REQUIRE(results[0].attribute == "n1");
            REQUIRE(results[0].value == "v1");
            REQUIRE(results[0].units == "u1");

            REQUIRE_NOTHROW(fs::client::remove_metadata(conn, p, md));
            REQUIRE(fs::client::get_metadata(conn, p).empty());
        }

        SECTION("exceptions")
        {
            std::array<fs::metadata, 3> metadata{{
                {"n1", "v1", "u1"},
                {"n2", "v2", "u2"},
                {"n3", "v3", "u3"}
            }};

            REQUIRE_THROWS(fs::client::set_metadata(conn, "invalid_path", metadata[0]), "cannot set metadata: unknown object type");
            REQUIRE_THROWS(fs::client::add_metadata(conn, "invalid_path", metadata[0]), "cannot add metadata: unknown object type");
            REQUIRE_THROWS(fs::client::remove_metadata(conn, "invalid_path", metadata[0]), "cannot remove metadata: unknown object type");

            // Atomic bulk operations.
            REQUIRE_THROWS(fs::client::add_metadata(conn, "invalid_path", metadata), "cannot apply metadata operations: unknown object type");
            REQUIRE_THROWS(fs::client::remove_metadata(conn, "invalid_path", metadata), "cannot apply metadata operations: unknown object type");
        }

#ifdef IRODS_ENABLE_ALL_UNIT_TESTS
        SECTION("atomic operations")
        {
            // IMPORTANT
            // ~~~~~~~~~
            // This test will fail against databases that have the transaction isolation
            // level set to REPEATABLE-READ or higher (e.g. MySQL by default). This is because
            // the database plugin cannot see changes committed by the nanodbc library.
            //
            // For more details, see: https://github.com/irods/irods/issues/4917

            std::array<fs::metadata, 3> metadata{{
                {"n1", "v1", "u1"},
                {"n2", "v2", "u2"},
                {"n3", "v3", "u3"}
            }};

            REQUIRE_NOTHROW(fs::client::add_metadata(conn, sandbox, metadata));

            auto results = fs::client::get_metadata(conn, sandbox);
            REQUIRE(results.size() == 3);
            REQUIRE(std::is_permutation(std::begin(results), std::end(results), std::begin(metadata),
                                        [](const auto& _lhs, const auto& _rhs)
                                        {
                                            return _lhs.attribute == _rhs.attribute &&
                                                   _lhs.value == _rhs.value &&
                                                   _lhs.units == _rhs.units;
                                        }));

            REQUIRE_NOTHROW(fs::client::remove_metadata(conn, sandbox, metadata));
            REQUIRE(fs::client::get_metadata(conn, sandbox).empty());
        }
#endif // IRODS_ENABLE_ALL_UNIT_TESTS
    }

    SECTION("collection registration")
    {
        REQUIRE(fs::client::is_collection_registered(conn, sandbox));
        REQUIRE_FALSE(fs::client::is_collection_registered(conn, sandbox / "not_registered_in_catalog"));
    }

    SECTION("data object registration")
    {
        const fs::path p = sandbox / "data_object";

        {
            default_transport tp{conn};
            odstream{tp, p};
        }

        REQUIRE(fs::client::is_data_object_registered(conn, p));
        REQUIRE_FALSE(fs::client::is_data_object_registered(conn, sandbox / "not_registered_in_catalog"));
    }

    SECTION("special collections")
    {
        // Create a special collection.
        const auto link_name = fs::path{env.rodsHome} / "col_alias.d";
        const auto cmd = fmt::format("imcoll -m link {} {}", sandbox.c_str(), link_name.c_str());
        REQUIRE(std::system(cmd.data()) == 0);

        irods::at_scope_exit remove_special_collection{[&conn, &link_name] {
            const auto cmd = fmt::format("imcoll -U {}", link_name.c_str());
            REQUIRE(std::system(cmd.data()) == 0);
            REQUIRE(fs::client::remove_all(conn, link_name));
        }};

        // Show that special collections are detected.
        REQUIRE(fs::client::is_special_collection(conn, link_name));

        // Show that normal collections are not considered to be special collections.
        REQUIRE_FALSE(fs::client::is_special_collection(conn, sandbox));
    }
}

auto get_hostname() noexcept -> std::string
{
    char hostname[250];
    gethostname(hostname, sizeof(hostname));
    return hostname;
}

auto create_resource_vault(const std::string_view _vault_name) -> boost::filesystem::path
{
    namespace fs = boost::filesystem;

    const auto suffix = "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    const auto vault = boost::filesystem::temp_directory_path() / (_vault_name.data() + suffix).data();

    // Create the vault for the resource and allow the iRODS server to
    // read and write to the vault.

    if (!exists(vault)) {
        REQUIRE(fs::create_directory(vault));
    }

    fs::permissions(vault, fs::perms::add_perms | fs::perms::others_read | fs::perms::others_write);

    return vault;
}

auto add_ufs_resource(const std::string_view _resc_name, const std::string_view _vault_name) -> void
{
    namespace adm = irods::experimental::administration;

    const auto host_name = get_hostname();
    const auto vault_path = create_resource_vault(_vault_name);

    // The new resource's information.
    adm::resource_registration_info ufs_info;
    ufs_info.resource_name = _resc_name.data();
    ufs_info.resource_type = adm::resource_type::unixfilesystem.data();
    ufs_info.host_name = host_name;
    ufs_info.vault_path = vault_path.c_str();

    irods::experimental::client_connection conn;
    REQUIRE(adm::client::add_resource(conn, ufs_info).value() == 0);
}

auto replicate_data_object(const std::string_view _path, const std::string_view _resc_name) -> void
{
    dataObjInp_t repl_input{};
    irods::at_scope_exit free_memory{[&repl_input] { clearKeyVal(&repl_input.condInput); }};
    std::strncpy(repl_input.objPath, _path.data(), _path.size());
    addKeyVal(&repl_input.condInput, DEST_RESC_NAME_KW, _resc_name.data());

    irods::experimental::client_connection conn;
    REQUIRE(rcDataObjRepl(static_cast<rcComm_t*>(conn), &repl_input) == 0);
}

