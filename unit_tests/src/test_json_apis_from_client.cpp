#include "catch.hpp"

#include "atomic_apply_metadata_operations.h"
#include "connection_pool.hpp"
#include "dstream.hpp"
#include "filesystem.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_query.hpp"
#include "replica.hpp"
#include "rodsClient.h"
#include "client_connection.hpp"
#include "transport/default_transport.hpp"

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include <chrono>
#include <string_view>
#include <string>
#include <stdexcept>

#include <unistd.h>
#include <cstdlib>

#include "json.hpp"


namespace ix = irods::experimental;
namespace fs = irods::experimental::filesystem;

using nlohmann::json;
using namespace std::string_literals;

#undef  NON_EXISTENT_ENV_VAR
#define NON_EXISTENT_ENV_VAR "<null-env-var>"

namespace {

    class error_setting_environment : public std::runtime_error {
        public:
        explicit error_setting_environment(const std::string & msg) : std::runtime_error{msg} {
        }
    };

    void change_environment_variable (const char* env_var_name, std::string & tmpStr, const char* new_value) {
        char *tmp = getenv(env_var_name);
        if (nullptr != tmp) { tmpStr = tmp; }
        if (0 != setenv(env_var_name,new_value,1)) {
            throw error_setting_environment {"cannot change environment variable: "s + env_var_name};
        }
    }

    void restore_environment_variable (const char *env_var_name, const std::string & tmp) {
        if (tmp != NON_EXISTENT_ENV_VAR) {
            if (0 != setenv(env_var_name, tmp.c_str(), 1)) {
                throw error_setting_environment {"cannot restore environment variable: "s + env_var_name};
            }
        }
    }
}

TEST_CASE("test atomic metadata with BinBytesBuf", "[xml]")
{
    std::string save_protocol {NON_EXISTENT_ENV_VAR};

    change_environment_variable("irodsProt",save_protocol,"1");

    load_client_api_plugins();

    auto conn_pool = irods::make_connection_pool(2);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

    ix::client_connection conn{env.rodsHost,
                               env.rodsPort,
                               env.rodsUserName,
                               env.rodsZone};

    REQUIRE(fs::client::create_collection(conn, sandbox));

    irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
        namespace fs = ix::filesystem;
        fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash);
    }};

    const auto json_input = json{
        {"entity_name", sandbox.string()},
        {"entity_type", "collection"},
        {"operations", json::array({
            {
                {"operation", "add"},
                {"attribute", "the_attr"},
                {"value", "the_val"},
                {"units", "the_units"}
            },
            {
                {"operation", "add"},
                {"attribute", "name"},
                {"value", "john <&doe&>"},
                {"units", ""}
            }
        })}
    }.dump();

    char* json_error_string{};
    irods::at_scope_exit free_memory{[&json_error_string] { std::free(json_error_string); }};

    auto* conn_ptr = static_cast<rcComm_t*>(conn);
    REQUIRE(rc_atomic_apply_metadata_operations(conn_ptr, json_input.c_str(), &json_error_string) == 0);
    REQUIRE(json_error_string == "{}"s);

    restore_environment_variable("irodsProt",save_protocol);

    const auto gql = fmt::format("select COUNT(COLL_ID) where COLL_NAME = '{}' and META_COLL_ATTR_VALUE like '{}'",
                                 sandbox.c_str(), 
                                 ("%" "<&" "%" "&>" "%"));

    for (auto&& row : irods::query{static_cast<rcComm_t*>(conn), gql}) {
        REQUIRE(row[0] == "1");
    }
}
