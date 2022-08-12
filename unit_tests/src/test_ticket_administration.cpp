#include <catch2/catch.hpp>

#include "irods/connection_pool.hpp"
#include "irods/rodsClient.h"
#include "irods/ticket_administration.hpp"
#include "irods/query_builder.hpp"
#include "irods/user_administration.hpp"

#include <fmt/format.h>

#include <string>
#include <stdio.h>
#include <stdexcept>

TEST_CASE("ticket administration")
{
    namespace tic = irods::experimental::administration::ticket;
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

    irods::experimental::administration::user test_user{user_name};
    irods::experimental::administration::client::add_user(conn, test_user);

    irods::experimental::administration::group test_group{group_name};
    irods::experimental::administration::client::add_group(conn, test_group);

    SECTION("create read ticket")
    {
        tic::client::create_ticket(conn, tic::ticket_type::read, read_coll_string, read_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", read_ticket_name));

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
    }

    SECTION("delete ticket")
    {
        tic::client::delete_ticket(conn, read_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", read_ticket_name));

        bool test_ticket_exists = false;

        for (const auto& row : general_query) {
            if (row[0] == read_ticket_name) {
                test_ticket_exists = true;
                break;
            }
        }

        REQUIRE(!test_ticket_exists);
    }

    SECTION("create write ticket")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, write_coll_string, write_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", write_ticket_name));

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
    }

    SECTION("admin delete ticket")
    {
        tic::client::delete_ticket(tic::admin, conn, write_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", write_ticket_name));

        bool test_ticket_exists = false;

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

        auto general_query = builder.build<rcComm_t>(conn, fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", read_ticket_name));

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
        tic::client::create_ticket(tic::admin,
                                   conn,
                                   tic::ticket_type::write,
                                   write_coll_string,
                                   write_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, fmt::format("select TICKET_STRING where TICKET_STRING = '{}'", write_ticket_name));

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

    SECTION("add user")
    {
        tic::client::create_ticket(conn, tic::ticket_type::read, set_ticket_collection, set_ticket_name);

        tic::client::add_ticket_constraint(conn, set_ticket_name, tic::user_constraint{user_name});

        std::string query = fmt::format("select TICKET_ALLOWED_USER_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        bool user_name_exists = false;

        for (const auto& row: general_query) {
            if (row[0] == user_name){
                user_name_exists = true;
                break;
            }
        }

        REQUIRE(user_name_exists);

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("remove user")
    {
        tic::client::create_ticket(conn, tic::ticket_type::read, set_ticket_collection, set_ticket_name);

        tic::client::add_ticket_constraint(conn, set_ticket_name, tic::user_constraint{user_name});

        tic::client::remove_ticket_constraint(conn, set_ticket_name, tic::user_constraint{user_name});

        std::string query = fmt::format("select TICKET_ALLOWED_USER_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() == 0);

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("add group")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::add_ticket_constraint(conn, set_ticket_name, tic::group_constraint{group_name});

        std::string query = fmt::format("select TICKET_ALLOWED_GROUP_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(group_name == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("remove group")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::add_ticket_constraint(conn, set_ticket_name, tic::group_constraint{group_name});

        tic::client::remove_ticket_constraint(conn, set_ticket_name, tic::group_constraint{group_name});

        std::string query = fmt::format("select TICKET_ALLOWED_GROUP_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() == 0);

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("set uses_count")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::set_ticket_constraint(conn, set_ticket_name, tic::use_count_constraint{set_test_number});

        std::string query = fmt::format("select TICKET_USES_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(std::to_string(set_test_number) == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("remove uses_count")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::set_ticket_constraint(conn, set_ticket_name, tic::use_count_constraint{set_test_number});

        tic::client::remove_ticket_constraint(conn, set_ticket_name, tic::use_count_constraint{});

        std::string query = fmt::format("select TICKET_USES_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("admin set write_file")
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

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("remove write_file")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::set_ticket_constraint(conn,
                                           set_ticket_name,
                                           tic::n_writes_to_data_object_constraint{set_test_number});

        tic::client::remove_ticket_constraint(conn, set_ticket_name, tic::n_writes_to_data_object_constraint{});

        std::string query = fmt::format("select TICKET_WRITE_FILE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("set write_byte")
    {
        tic::client::create_ticket(conn, tic::ticket_type::write, set_ticket_collection, set_ticket_name);

        tic::client::set_ticket_constraint(conn, set_ticket_name, tic::n_write_bytes_constraint{set_test_number});

        std::string query = fmt::format("select TICKET_WRITE_BYTE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(std::to_string(set_test_number) == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    SECTION("admin remove write_byte")
    {
        tic::client::create_ticket(tic::admin,
                                   conn,
                                   tic::ticket_type::write,
                                   set_ticket_collection,
                                   set_ticket_name);

        tic::client::set_ticket_constraint(conn, set_ticket_name, tic::n_write_bytes_constraint{set_test_number});

        tic::client::remove_ticket_constraint(tic::admin, conn, set_ticket_name, tic::n_write_bytes_constraint{});

        std::string query = fmt::format("select TICKET_WRITE_BYTE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string == (general_query.begin().capture_results().at(0)));

        tic::client::delete_ticket(conn, set_ticket_name);
    }

    irods::experimental::administration::client::remove_user(conn, test_user);
    irods::experimental::administration::client::remove_group(conn, test_group);
}