#include <catch2/catch.hpp>

#include "irods/connection_pool.hpp"
#include "irods/rodsClient.h"
#include "irods/ticket_administration.hpp"
#include "irods/query_builder.hpp"
#include "irods/user_administration.hpp"

#include <fmt/format.h>

#include <string>
#include <iostream>
#include <stdio.h>
#include <stdexcept>

TEST_CASE("ticket administration")
{
    namespace tic = irods::experimental::administration::ticket;
    std::string read_ticket_name = "testTicket";
    std::string write_ticket_name = "testTicket2";
    std::string read_string = "read";
    std::string read_coll_string = "/tempZone/home/rods";
    std::string write_string = "write";
    std::string write_coll_string = "/tempZone";

    irods::experimental::query_builder builder;

    load_client_api_plugins();
    auto conn_pool = irods::make_connection_pool();
    auto conn = conn_pool->get_connection();

    std::string set_ticket_name = "setTicket";
    std::string set_ticket_collection = "/tempZone/home";
    std::string user_name = "testUser";
    std::string group_name = "testGroup";

    int set_test_number = 19;
    std::string set_test_number_string = std::to_string(set_test_number);
    std::string remove_test_number_string = std::to_string(0);

    irods::experimental::administration::user test_user{user_name};
    irods::experimental::administration::client::add_user(conn, test_user);

    irods::experimental::administration::group test_group{group_name};
    irods::experimental::administration::client::add_group(conn, test_group);

    SECTION("create read ticket")
    {
        tic::create_ticket(conn, tic::type::read, read_coll_string, read_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, "select TICKET_STRING");

        bool test_ticket_exists = false;

        for (auto&& row : general_query) {
            for (auto values : row) {
                if (values.compare(read_ticket_name) == 0) {
                    test_ticket_exists = true;
                }
            }
        }

        REQUIRE(test_ticket_exists);

        std::string query =
            fmt::format("select TICKET_COLL_NAME, TICKET_TYPE where TICKET_STRING = '{}'", read_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(read_coll_string.compare(general_query.begin().capture_results().at(0)) == 0);
        REQUIRE(read_string.compare(general_query.begin().capture_results().at(1)) == 0);
    }

    SECTION("delete ticket")
    {
        tic::delete_ticket(conn, read_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, "select TICKET_STRING");

        bool test_ticket_exists = false;

        for (auto&& row : general_query) {
            for (auto values : row) {
                if (values.compare(read_ticket_name) == 0) {
                    test_ticket_exists = true;
                }
            }
        }

        REQUIRE(!test_ticket_exists);
    }

    SECTION("create write ticket")
    {
        tic::create_ticket(conn, tic::type::write, write_coll_string, write_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, "select TICKET_STRING");

        bool test_ticket_exists = false;

        for (auto&& row : general_query) {
            for (auto values : row) {
                if (values.compare(write_ticket_name) == 0) {
                    test_ticket_exists = true;
                }
            }
        }

        REQUIRE(test_ticket_exists);

        std::string query =
            fmt::format("select TICKET_COLL_NAME, TICKET_TYPE where TICKET_STRING = '{}'", write_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(write_coll_string.compare(general_query.begin().capture_results().at(0)) == 0);
        REQUIRE(write_string.compare(general_query.begin().capture_results().at(1)) == 0);
    }

    SECTION("admin delete ticket")
    {
        tic::delete_ticket(tic::admin_tag{}, conn, write_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, "select TICKET_STRING");

        bool test_ticket_exists = false;

        for (auto&& row : general_query) {
            for (auto values : row) {
                if (values.compare(write_ticket_name) == 0) {
                    test_ticket_exists = true;
                }
            }
        }

        REQUIRE(!test_ticket_exists);
    }

    SECTION("admin create read ticket")
    {
        tic::create_ticket(tic::admin_tag{}, conn, tic::type::read, read_coll_string, read_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, "select TICKET_STRING");

        bool test_ticket_exists = false;

        for (auto&& row : general_query) {
            for (auto values : row) {
                if (values.compare(read_ticket_name) == 0) {
                    test_ticket_exists = true;
                }
            }
        }

        REQUIRE(test_ticket_exists);

        std::string query =
            fmt::format("select TICKET_COLL_NAME, TICKET_TYPE where TICKET_STRING = '{}'", read_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(read_coll_string.compare(general_query.begin().capture_results().at(0)) == 0);
        REQUIRE(read_string.compare(general_query.begin().capture_results().at(1)) == 0);

        tic::delete_ticket(conn, read_ticket_name);
    }

    SECTION("admin create write ticket")
    {
        tic::create_ticket(tic::admin_tag{}, conn, tic::type::write, write_coll_string, write_ticket_name);

        auto general_query = builder.build<rcComm_t>(conn, "select TICKET_STRING");

        bool test_ticket_exists = false;

        for (auto&& row : general_query) {
            for (auto values : row) {
                if (values.compare(write_ticket_name) == 0) {
                    test_ticket_exists = true;
                }
            }
        }

        REQUIRE(test_ticket_exists);

        std::string query =
            fmt::format("select TICKET_COLL_NAME, TICKET_TYPE where TICKET_STRING = '{}'", write_ticket_name);
        general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(write_coll_string.compare(general_query.begin().capture_results().at(0)) == 0);
        REQUIRE(write_string.compare(general_query.begin().capture_results().at(1)) == 0);

        tic::delete_ticket(conn, write_ticket_name);
    }

    SECTION("add user")
    {
        tic::create_ticket(conn, tic::type::read, set_ticket_collection, set_ticket_name);

        tic::add_ticket_constraint(conn, set_ticket_name, tic::user_constraint{user_name});

        std::string query = fmt::format("select TICKET_ALLOWED_USER_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        bool user_name_exists = false;

        for (auto&& row : general_query) {
            for (auto values : row) {
                if (user_name.compare(values) == 0) {
                    user_name_exists = true;
                }
            }
        }

        REQUIRE(user_name_exists);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("remove user")
    {
        tic::create_ticket(conn, tic::type::read, set_ticket_collection, set_ticket_name);

        tic::add_ticket_constraint(conn, set_ticket_name, tic::user_constraint{user_name});

        tic::remove_ticket_constraint(conn, set_ticket_name, tic::user_constraint{user_name});

        std::string query = fmt::format("select TICKET_ALLOWED_USER_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("add group")
    {
        tic::create_ticket(conn, tic::type::write, set_ticket_collection, set_ticket_name);

        tic::add_ticket_constraint(conn, set_ticket_name, tic::group_constraint{group_name});

        std::string query = fmt::format("select TICKET_ALLOWED_GROUP_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(group_name.compare(general_query.begin().capture_results().at(0)) == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("remove group")
    {
        tic::create_ticket(conn, tic::type::write, set_ticket_collection, set_ticket_name);

        tic::add_ticket_constraint(conn, set_ticket_name, tic::group_constraint{group_name});

        tic::remove_ticket_constraint(conn, set_ticket_name, tic::group_constraint{group_name});

        std::string query = fmt::format("select TICKET_ALLOWED_GROUP_NAME where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("set uses_count")
    {
        tic::create_ticket(conn, tic::type::write, set_ticket_collection, set_ticket_name);

        tic::set_ticket_constraint(conn, set_ticket_name, tic::use_count_constraint{set_test_number});

        std::string query = fmt::format("select TICKET_USES_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(set_test_number_string.compare(general_query.begin().capture_results().at(0)) == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("remove uses_count")
    {
        tic::create_ticket(conn, tic::type::write, set_ticket_collection, set_ticket_name);

        tic::set_ticket_constraint(conn, set_ticket_name, tic::use_count_constraint{set_test_number});

        tic::remove_ticket_constraint(conn,
                                      set_ticket_name,
                                      tic::use_count_constraint{});

        std::string query = fmt::format("select TICKET_USES_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string.compare(general_query.begin().capture_results().at(0)) == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("admin set write_file")
    {
        tic::create_ticket(conn, tic::type::write, set_ticket_collection, set_ticket_name);

        tic::set_ticket_constraint(tic::admin_tag{},
                                   conn,
                                   set_ticket_name,
                                   tic::n_writes_to_data_object_constraint{set_test_number});

        std::string query = fmt::format("select TICKET_WRITE_FILE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(set_test_number_string.compare(general_query.begin().capture_results().at(0)) == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("remove write_file")
    {
        tic::create_ticket(conn, tic::type::write, set_ticket_collection, set_ticket_name);

        tic::set_ticket_constraint(conn, set_ticket_name, tic::n_writes_to_data_object_constraint{set_test_number});

        tic::remove_ticket_constraint(conn,
                                      set_ticket_name,
                                      tic::n_writes_to_data_object_constraint{});

        std::string query = fmt::format("select TICKET_WRITE_FILE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string.compare(general_query.begin().capture_results().at(0)) == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("set write_byte")
    {
        tic::create_ticket(conn, tic::type::write, set_ticket_collection, set_ticket_name);

        tic::set_ticket_constraint(conn, set_ticket_name, tic::n_write_bytes_constraint{set_test_number});

        std::string query = fmt::format("select TICKET_WRITE_BYTE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(set_test_number_string.compare(general_query.begin().capture_results().at(0)) == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }

    SECTION("admin remove write_byte")
    {
        tic::create_ticket(tic::admin_tag{}, conn, tic::type::write, set_ticket_collection, set_ticket_name);

        tic::set_ticket_constraint(conn, set_ticket_name, tic::n_write_bytes_constraint{set_test_number});

        tic::remove_ticket_constraint(tic::admin_tag{},
                                      conn,
                                      set_ticket_name,
                                      tic::n_write_bytes_constraint{});

        std::string query = fmt::format("select TICKET_WRITE_BYTE_LIMIT where TICKET_STRING = '{}'", set_ticket_name);
        auto general_query = builder.build<rcComm_t>(conn, query);

        REQUIRE(general_query.size() > 0);
        REQUIRE(remove_test_number_string.compare(general_query.begin().capture_results().at(0)) == 0);

        tic::delete_ticket(conn, set_ticket_name);
    }
}