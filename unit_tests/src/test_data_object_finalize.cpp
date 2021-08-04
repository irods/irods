#include "catch.hpp"

#include "irods_error_enum_matcher.hpp"
#include "rodsClient.h"

#include "client_connection.hpp"
#include "data_object_finalize.h"
#include "data_object_proxy.hpp"
#include "dataObjRepl.h"
#include "dstream.hpp"
#include "filesystem.hpp"
#include "irods_at_scope_exit.hpp"
#include "replica.hpp"
#include "replica_proxy.hpp"
#include "resource_administration.hpp"
#include "transport/default_transport.hpp"
#include "unit_test_utils.hpp"

#include "json.hpp"
#include "fmt/format.h"

#include <cstdlib>
#include <chrono>
#include <iostream>
#include <thread>

namespace fs = irods::experimental::filesystem;
namespace id = irods::experimental::data_object;
namespace io = irods::experimental::io;
namespace ir = irods::experimental::replica;

using json = nlohmann::json;

TEST_CASE("finalize", "[finalize]")
{
    using namespace std::string_literals;
    using namespace std::chrono_literals;
    namespace adm = irods::experimental::administration;

    load_client_api_plugins();

    // create two resources onto which a data object can be written
    const std::string resc_0 = "get_data_obj_info_resc_0";
    const std::string resc_1 = "get_data_obj_info_resc_1";

    irods::at_scope_exit remove_resources{[&resc_0, &resc_1] {
        // reset connection to ensure resource manager is current
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        adm::client::remove_resource(comm, resc_0);
        adm::client::remove_resource(comm, resc_1);
    }};

    const auto mkresc = [](std::string_view _name)
    {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        if (const auto [ec, exists] = adm::client::resource_exists(comm, _name); exists) {
            REQUIRE(adm::client::remove_resource(comm, _name));
        }

        REQUIRE(unit_test_utils::add_ufs_resource(comm, _name, "vault_for_"s + _name.data()));
    };

    mkresc(resc_0);
    mkresc(resc_1);

    // reset connection so resources exist
    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    // create data object on one resource
    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_data_object_finalize";

    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&sandbox] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
    }};

    const auto target_object = sandbox / "target_object";

    {
        io::client::default_transport tp{comm};
        io::odstream{tp, target_object, io::root_resource_name{resc_0}};
    }

    REQUIRE(fs::client::exists(comm, target_object));

    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), resc_1));

    std::this_thread::sleep_for(2s);

    auto [og_op, og_lm] = id::make_data_object_proxy(comm, target_object);

    SECTION("basic operation")
    {
        const auto hash_replica_number = [](const int _rn) { return 2 * _rn + 2; };

        {
            // Get data object info, modify some fields in each replica, and stamp the catalog
            json input;

            input["logical_path"] = og_op.logical_path();

            for (auto& repl : og_op.replicas()) {
                const auto before = ir::to_json(repl);

                repl.replica_number(hash_replica_number(repl.replica_number()));
                repl.comments(fmt::format("replica is on {}", repl.resource().data()));
                repl.mtime(SET_TIME_TO_NOW_KW);

                const auto after = ir::to_json(repl);

                input["replicas"].push_back(json{
                    {"before", before},
                    {"after", after}
                });
            }

            char* error_string{};
            irods::at_scope_exit free_memory{[&error_string] { std::free(error_string); }};

            REQUIRE(0 == rc_data_object_finalize(&comm, input.dump().c_str(), &error_string));

            const auto output = json::parse(error_string);
            CHECK(output.at("database_updated").get<bool>());
            CHECK(output.at("error_message").get<std::string>().empty());
        }

        {
            // Ensure that the catalog was updated correctly for each replica
            auto [op, lm] = id::make_data_object_proxy(comm, target_object);

            REQUIRE(og_op.replica_count() == op.replica_count());

            int i = 0;
            for (auto&& repl : op.replicas()) {
                const std::string comment = fmt::format("replica is on {}", repl.resource().data());

                // check modified fields
                REQUIRE(repl.ctime() != repl.mtime());
                REQUIRE(repl.mtime() != SET_TIME_TO_NOW_KW);
                REQUIRE(repl.comments() == comment);
                REQUIRE(repl.replica_number() == hash_replica_number(i));

                // check data object fields
                REQUIRE(og_op.data_id() == op.data_id());
                REQUIRE(og_op.collection_id() == op.collection_id());
                REQUIRE(og_op.logical_path() == op.logical_path());
                REQUIRE(og_op.owner_user_name() == op.owner_user_name());
                REQUIRE(og_op.owner_zone_name() == op.owner_zone_name());

                // check replica fields
                const auto& og_repl = og_op.replicas()[i];

                REQUIRE(og_repl.resource()      == repl.resource());
                REQUIRE(og_repl.hierarchy()     == repl.hierarchy());
                REQUIRE(og_repl.type()          == repl.type());
                REQUIRE(og_repl.size()          == repl.size());
                REQUIRE(og_repl.checksum()      == repl.checksum());
                REQUIRE(og_repl.version()       == repl.version());
                REQUIRE(og_repl.physical_path() == repl.physical_path());
                REQUIRE(og_repl.resource_id()   == repl.resource_id());
                REQUIRE(og_repl.ctime()         == repl.ctime());
                REQUIRE(og_repl.in_pdmo()       == repl.in_pdmo());
                REQUIRE(og_repl.status()        == repl.status());
                REQUIRE(og_repl.mode()          == repl.mode());
                REQUIRE(og_repl.data_expiry()   == repl.data_expiry());
                REQUIRE(og_repl.map_id()        == repl.map_id());

                i++;
            }
        }
    }

    SECTION("immutable columns")
    {
        constexpr auto new_data_id = 1;
        constexpr auto new_collection_id = 2;
        const auto new_logical_path = fs::path{og_op.logical_path().data()}.parent_path() / "new_data_name";

        // Get data object info, modify some fields in each replica, and stamp the catalog
        json input;

        input["logical_path"] = og_op.logical_path();

        auto [op, op_lm] = id::duplicate_data_object(*og_op.get());

        for (auto& repl : op.replicas()) {
            const auto before = ir::to_json(repl);

            repl.data_id(new_data_id);
            repl.collection_id(new_collection_id);
            repl.logical_path(new_logical_path.c_str());

            const auto after = ir::to_json(repl);

            input["replicas"].push_back(json{
                {"before", before},
                {"after", after}
            });
        }

        char* error_string{};
        irods::at_scope_exit free_memory{[&error_string] { std::free(error_string); }};

        REQUIRE(0 == rc_data_object_finalize(&comm, input.dump().c_str(), &error_string));

        const auto output = json::parse(error_string);
        CHECK(output.at("database_updated").get<bool>());
        CHECK(output.at("error_message").get<std::string>().empty());

        // Ensure that the catalog was updated correctly for each replica
        const auto [new_op, lm] = id::make_data_object_proxy(comm, target_object);

        REQUIRE(og_op.replica_count() == new_op.replica_count());

        for (auto&& repl : new_op.replicas()) {
            // check modified fields (that is, that the fields were NOT modified)
            CHECK(og_op.data_id()       == repl.data_id());
            CHECK(og_op.collection_id() == repl.collection_id());
            CHECK(og_op.logical_path()  == repl.logical_path());
        }
    }

    // TODO: add test for inclusion/exclusion of logical_path
    // TODO: add test for file_modified
    // TODO: add test for bytes_written
}

TEST_CASE("invalid inputs", "[invalid]")
{
    load_client_api_plugins();

    irods::experimental::client_connection comm;

    {
        char* error_string{};
        irods::at_scope_exit free_memory{[&error_string] { std::free(error_string); }};

        REQUIRE_THAT(rc_data_object_finalize(static_cast<RcComm*>(comm), json{}.dump().c_str(), &error_string),
            equals_irods_error(SYS_INVALID_INPUT_PARAM));

        const auto output = json::parse(error_string);
        CHECK(!output.at("database_updated").get<bool>());
        CHECK(!output.at("error_message").get<std::string>().empty());
    }

    {
        char* error_string{};
        irods::at_scope_exit free_memory{[&error_string] { std::free(error_string); }};

        REQUIRE_THAT(rc_data_object_finalize(static_cast<RcComm*>(comm), "nope", &error_string),
            equals_irods_error(INPUT_ARG_NOT_WELL_FORMED_ERR));

        const auto output = json::parse(error_string);
        CHECK(!output.at("database_updated").get<bool>());
        CHECK(!output.at("error_message").get<std::string>().empty());
    }
}
