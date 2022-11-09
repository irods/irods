#include <catch2/catch.hpp>

#include "irods/connection_pool.hpp"
#include "irods/rodsClient.h"
#include "irods/ticket_administration.hpp"
#include "irods/query_builder.hpp"
#include "irods/user_administration.hpp"
#include "irods/irods_at_scope_exit.hpp"

#include <fmt/format.h>

#include <string>
#include <cstdio>
#include <stdexcept>

namespace adm = irods::experimental::administration;
namespace tic = irods::experimental::administration::ticket;

TEST_CASE("ticket administration")
{
    const std::string read_ticket_name = "readTicket";
    const std::string write_ticket_name = "writeTicket";
    const std::string read_string = "read";
    const std::string read_coll_string = "/tempZone/home/rods";
    const std::string write_string = "write";
    const std::string write_coll_string = "/tempZone";

    irods::experimental::query_builder builder;

    load_client_api_plugins();
    auto conn_pool = irods::make_connection_pool();
    auto conn = conn_pool->get_connection();

    const std::string set_ticket_name = "setTicket";
    const std::string set_ticket_collection = "/tempZone/home";
    const std::string user_name = "testUser";
    const std::string group_name = "testGroup";

    int set_test_number = 19;
    std::string remove_test_number_string = "0";

    adm::user test_user{user_name};
    adm::client::add_user(conn, test_user);
    irods::at_scope_exit remove_user{[&conn, &test_user] { adm::client::remove_user(conn, test_user); }};

    adm::group test_group{group_name};
    adm::client::add_group(conn, test_group);
    irods::at_scope_exit remove_group{[&conn, &test_group] { adm::client::remove_group(conn, test_group); }};

    SECTION("create read ticket / delete ticket")
    {
        tic::client::create_ticket(conn, tic::ticket_type::read, read_coll_string, read_ticket_name);

        auto general_query =
            builder.build<rcComm_t>(conn,
                                    fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", read_ticket_name));

        bool test_ticket_exists = false;

        for (const auto& row : general_query) {
            if (row[0] == read_ticket_name) {
                test_ticket_exists = true;
                break;
            }
        }

        REQUIRE(test_ticket_exists);

        std::string query =
            fmt::format("select TICKET_COLL_NAME, TICKET_TYPE where TICKET_STRING = '{}'", read_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(read_coll_string == (general_query.begin().capture_results().at(0)));
        REQUIRE(read_string == (general_query.begin().capture_results().at(1)));

        tic::client::delete_ticket(conn, read_ticket_name);

        general_query =
            builder.build<rcComm_t>(conn,
                                    fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", read_ticket_name));

        test_ticket_exists = false;

        for (const auto& row : general_query) {
            if (row[0] == read_ticket_name) {
                test_ticket_exists = true;
                break;
            }
        }

        REQUIRE(!test_ticket_exists);
    }

    SECTION("create write ticket/admin delete ticket")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, write_coll_string, write_ticket_name);

        auto general_query =
            builder.build<rcComm_t>(conn,
                                    fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", write_ticket_name));

        bool test_ticket_exists = false;

        for (const auto& row : general_query) {
            if (row[0] == write_ticket_name) {
                test_ticket_exists = true;
                break;
            }
        }

        REQUIRE(test_ticket_exists);

        std::string query =
            fmt::format("select TICKET_COLL_NAME, TICKET_TYPE where TICKET_STRING = '{}'", write_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(write_coll_string == general_query.begin().capture_results().at(0));
        REQUIRE(write_string == general_query.begin().capture_results().at(1));

        tic::client::delete_ticket(tic::admin, conn, write_ticket_name);

        general_query =
            builder.build<rcComm_t>(conn,
                                    fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", write_ticket_name));

        test_ticket_exists = false;

        for (const auto& row : general_query) {
            if (row[0] == write_ticket_name) {
                test_ticket_exists = true;
                break;
            }
        }

        REQUIRE(!test_ticket_exists);
    }

    SECTION("admin create read ticket")
    {
        tic::client::create_ticket(tic::admin, conn, tic::ticket_type::read, read_coll_string, read_ticket_name);

        auto general_query =
            builder.build<rcComm_t>(conn,
                                    fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", read_ticket_name));

        bool test_ticket_exists = false;

        for (const auto& row : general_query) {
            if (row[0] == read_ticket_name) {
                test_ticket_exists = true;
                break;
            }
        }

        REQUIRE(test_ticket_exists);

        std::string query =
            fmt::format("select TICKET_COLL_NAME, TICKET_TYPE where TICKET_STRING = '{}'", read_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(read_coll_string == (general_query.begin().capture_results().at(0)));
        REQUIRE(read_string == (general_query.begin().capture_results().at(1)));

        tic::client::delete_ticket(conn, read_ticket_name);
    }

    SECTION("admin create write ticket")
    {
        tic::client::create_ticket(tic::admin, conn, tic::ticket_type::write, write_coll_string, write_ticket_name);

        auto general_query =
            builder.build<rcComm_t>(conn,
                                    fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", write_ticket_name));

        bool test_ticket_exists = false;

        for (const auto& row : general_query) {
            if (row[0] == write_ticket_name) {
                test_ticket_exists = true;
                break;
            }
        }

        REQUIRE(test_ticket_exists);

        std::string query =
            fmt::format("select TICKET_COLL_NAME, TICKET_TYPE where TICKET_STRING = '{}'", write_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(write_coll_string == (general_query.begin().capture_results().at(0)));
        REQUIRE(write_string == (general_query.begin().capture_results().at(1)));

        tic::client::delete_ticket(conn, write_ticket_name);
    }

    SECTION("add user/remove user")
    {
        tic::client::create_ticket(conn, tic::ticket_type::read, set_ticket_collection, set_ticket_name);

        tic::client::add_ticket_constraint(conn, set_ticket_name, tic::user_constraint{user_name});

        std::string query = fmt::format("select TICKET_ALLOWED_USER_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        bool user_name_exists = false;

        for (const auto& row : general_query) {
            if (row[0] == user_name) {
                user_name_exists = true;
                break;
            }
        }

        REQUIRE(user_name_exists);

        tic::client::remove_ticket_constraint(conn, set_ticket_name, tic::user_constraint{user_name});

        query = fmt::format("select TICKET_ALLOWED_USER_NAME where TICKET_STRING = '{}'", set_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() == 0);

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("add group/remove group")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::add_ticket_constraint(conn, set_ticket_name, tic::group_constraint{group_name});

        std::string query = fmt::format("select TICKET_ALLOWED_GROUP_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(group_name == (general_query.begin().capture_results().at(0)));

        tic::client::remove_ticket_constraint(conn, set_ticket_name, tic::group_constraint{group_name});

        query = fmt::format("select TICKET_ALLOWED_GROUP_NAME where TICKET_STRING = '{}'", set_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() == 0);

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("set uses_count/remove uses_count")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::set_ticket_constraint(conn, set_ticket_name, tic::use_count_constraint{set_test_number});

        std::string query = fmt::format("select TICKET_USES_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(std::to_string(set_test_number) == (general_query.begin().capture_results().at(0)));

        tic::client::remove_ticket_constraint(conn, set_ticket_name, tic::use_count_constraint{});

        query = fmt::format("select TICKET_USES_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("admin set write_file/remove write_file")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::set_ticket_constraint(tic::admin,
                                           conn,
                                           set_ticket_name,
                                           tic::n_writes_to_data_object_constraint{set_test_number});

        std::string query = fmt::format("select TICKET_WRITE_FILE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(std::to_string(set_test_number) == (general_query.begin().capture_results().at(0)));

        tic::client::remove_ticket_constraint(conn, set_ticket_name, tic::n_writes_to_data_object_constraint{});

        query = fmt::format("select TICKET_WRITE_FILE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("set write_byte/admin remove write_byte")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::set_ticket_constraint(conn, set_ticket_name, tic::n_write_bytes_constraint{set_test_number});

        std::string query = fmt::format("select TICKET_WRITE_BYTE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(std::to_string(set_test_number) == (general_query.begin().capture_results().at(0)));

        tic::client::remove_ticket_constraint(tic::admin, conn, set_ticket_name, tic::n_write_bytes_constraint{});

        query = fmt::format("select TICKET_WRITE_BYTE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }
}
