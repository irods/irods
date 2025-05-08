#include "utility.hpp"
#include <irods/filesystem.hpp>
#include <irods/get_grid_configuration_value.h>
#include <irods/irods_at_scope_exit.hpp>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_configuration_keywords.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/irods_string_tokenize.hpp>
#include <irods/key_value_proxy.hpp>
#include <irods/parseCommandLine.h>
#include <irods/query_builder.hpp>
#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <irods/rodsErrorTable.h>
#include <irods/rodsLog.h>
#include <irods/set_grid_configuration_value.h>
#include <irods/user.hpp>
#include <irods/user_administration.hpp>
#include <irods/version.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <termios.h>
#include <unistd.h>

#include <boost/lexical_cast.hpp>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#define BIG_STR 3000

int veryVerbose = 0;

char localZone[BIG_STR] = "";

rcComm_t *Conn;
rodsEnv myEnv;

int lastCommandStatus = 0;

void usage( char *subOpt );

namespace {

namespace fs = irods::experimental::filesystem;
using attrs_t = std::vector<std::pair<std::string_view, std::string_view>>;
const attrs_t genquery_attrs = {
    {"COLL_ID", COLL_ID_KW}, // not modifiable with iadmin modrepl
    {"DATA_CREATE_TIME", DATA_CREATE_KW},
    {"DATA_CHECKSUM", CHKSUM_KW},
    {"DATA_EXPIRY", DATA_EXPIRY_KW},
    {"DATA_ID", DATA_ID_KW}, // not modifiable with iadmin modrepl
    {"DATA_REPL_STATUS", REPL_STATUS_KW},
    {"DATA_MAP_ID", DATA_MAP_ID_KW}, // not modifiable with iadmin modrepl
    {"DATA_MODE", DATA_MODE_KW},
    {"DATA_NAME", DATA_NAME_KW}, // not modifiable with iadmin modrepl
    {"DATA_OWNER_NAME", DATA_OWNER_KW},
    {"DATA_OWNER_ZONE", DATA_OWNER_ZONE_KW},
    {"DATA_PATH", FILE_PATH_KW},
    {"DATA_REPL_NUM", REPL_NUM_KW},
    {"DATA_SIZE", DATA_SIZE_KW},
    {"DATA_STATUS", STATUS_STRING_KW},
    {"DATA_TYPE_NAME", DATA_TYPE_KW},
    {"DATA_VERSION", VERSION_KW},
    {"DATA_ACCESS_TIME", DATA_ACCESS_TIME_KW},
    {"DATA_MODIFY_TIME", DATA_MODIFY_KW},
    {"DATA_COMMENTS", DATA_COMMENTS_KW},
    //{"DATA_RESC_GROUP_NAME", DATA_RESC_GROUP_NAME_KW}, // missing from genquery since 4.2
    {"DATA_RESC_HIER", RESC_HIER_STR_KW}, // not modifiable with iadmin modrepl
    {"DATA_RESC_ID", RESC_ID_KW},
    {"DATA_RESC_NAME", RESC_NAME_KW} // not modifiable with iadmin modrepl
};


using args_vector_t = std::vector<std::string_view>;
auto get_args_vector(
    char** argv,
    const std::size_t argc) -> args_vector_t
{
    args_vector_t args;
    int i = 0;
    while (0 != strcmp(argv[i], "")) {
        args.push_back(argv[i++]);
    }

    if (args.size() < argc) {
        throw std::invalid_argument("Input arguments do not match expected values.");
    }
    return args;
} // get_args_vector

using bigint_type = int64_t;
using data_object_option_t = std::variant<bigint_type, fs::path>;
using data_id_t = std::variant_alternative<0, data_object_option_t>::type;
using logical_path_t = std::variant_alternative<1, data_object_option_t>::type;
auto get_data_object_value(
    const std::string_view option,
    const std::string_view input) -> data_object_option_t
{
    data_object_option_t v;
    if ("data_id" == option) {
        try {
            v = boost::lexical_cast<data_id_t>(input.data());
        } catch (const boost::bad_lexical_cast&) {
            std::stringstream msg;
            msg << "Invalid input [" << input << "] for data_id.";
            throw std::invalid_argument(msg.str());
        }
    }
    else if ("logical_path" == option) {
        const auto logical_path = fs::path{input.data()}.lexically_normal();
        if (!logical_path.is_absolute()) {
            throw std::invalid_argument("Provided logical_path must be absolute.");
        }
        v = logical_path;
    }
    return v;
} // get_data_object_value

using int_type = int;
using repl_option_t = std::variant<int_type, std::string_view>;
using replica_number_t = std::variant_alternative<0, repl_option_t>::type;
using resource_hierarchy_t = std::variant_alternative<1, repl_option_t>::type;
auto get_replica_value(
    const std::string_view option,
    const std::string_view input) -> repl_option_t
{
    repl_option_t v;
    if ("replica_number" == option) {
        try {
            v = boost::lexical_cast<replica_number_t>(input.data());
            if (veryVerbose) {
                std::cout << __FUNCTION__ << ": cast [" << std::get<replica_number_t>(v) << "]";
                std::cout << " from [" << input.data() << "]" << std::endl;
            }
        } catch (const boost::bad_lexical_cast&) {
            std::stringstream msg;
            msg << "Invalid input [" << input << "] for replica_number.";
            throw std::invalid_argument(msg.str());
        }
    }
    else if ("resource_hierarchy" == option) {
        v = input.data();
    }
    return v;
} // get_replica_value

auto print_grid_config(const char* _namespace, const char* _option_name) -> int
{
    try {
        namespace adm = irods::experimental::administration;

        const auto user_type =
            adm::client::type(*Conn, adm::user{Conn->clientUser.userName, Conn->clientUser.rodsZone});

        if (!user_type) {
            rodsLogError(
                LOG_ERROR, CAT_INVALID_USER_TYPE, "Could not determine if user has permission to perform operation.");
            return 1;
        }

        if (*user_type != adm::user_type::rodsadmin) {
            rodsLogError(LOG_ERROR, CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Operation requires rodsadmin level privileges.");
            return 1;
        }
    }
    catch (const irods::exception& e) {
        rodsLogError(LOG_ERROR, e.code(), e.client_display_what());
        return 1;
    }
    catch (const std::exception& e) {
        rodsLogError(LOG_ERROR, SYS_UNKNOWN_ERROR, e.what());
        return 1;
    }

    GridConfigurationInput input{};

    // This is the maximum size for the namespace (minus 1 for null terminator).
    constexpr auto namespace_max_size = sizeof(input.name_space) - 1;
    if (const auto len = strnlen(_namespace, namespace_max_size + 1); 0 == len || len > namespace_max_size) {
        std::cerr << fmt::format("Error: namespace must be between 1 and {} characters.\n", namespace_max_size);
        return 1;
    }

    // This is the maximum size for the option name (minus 1 for null terminator).
    constexpr auto option_name_max_size = sizeof(input.option_name) - 1;
    if (const auto len = strnlen(_option_name, option_name_max_size + 1); 0 == len || len > option_name_max_size) {
        std::cerr << fmt::format("Error: option name must be between 1 and {} characters.\n", option_name_max_size);
        return 1;
    }

    std::strncpy(input.name_space, _namespace, namespace_max_size);
    std::strncpy(input.option_name, _option_name, option_name_max_size);

    GridConfigurationOutput* output{};

    irods::at_scope_exit free_output{[&output] { std::free(output); }};

    if (const auto ec = rc_get_grid_configuration_value(Conn, &input, &output); ec != 0) {
        const auto msg = fmt::format("Failed to get grid configuration for namespace [{}] and option [{}] [ec={}]",
                                     _namespace,
                                     _option_name,
                                     ec);

        std::cerr << msg << "\n";

        return 1;
    }

    std::cout << output->option_value << '\n';

    return 0;
} // print_grid_config

auto set_grid_config(const char* _namespace, const char* _option_name, const char* _option_value) -> int
{
    if (!_namespace || !_option_name || !_option_value) {
        std::cerr << "One or more parameters to set_grid_configuration are null.\n";
        return 1;
    }

    try {
        namespace adm = irods::experimental::administration;

        const auto user_type =
            adm::client::type(*Conn, adm::user{Conn->clientUser.userName, Conn->clientUser.rodsZone});

        if (!user_type) {
            rodsLogError(LOG_ERROR,
                         CAT_INVALID_USER_TYPE,
                         "Could not determine if user has permission to set grid configuration values.");
            return 1;
        }

        if (*user_type != adm::user_type::rodsadmin) {
            rodsLogError(LOG_ERROR, CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Operation requires rodsadmin level privileges.");
            return 1;
        }
    }
    catch (const irods::exception& e) {
        rodsLogError(LOG_ERROR, e.code(), e.client_display_what());
        return 1;
    }
    catch (const std::exception& e) {
        rodsLogError(LOG_ERROR, SYS_UNKNOWN_ERROR, e.what());
        return 1;
    }

    GridConfigurationInput input{};

    // This is the maximum size for the namespace (minus 1 for null terminator).
    constexpr auto namespace_max_size = sizeof(input.name_space) - 1;
    if (const auto len = strnlen(_namespace, namespace_max_size + 1); 0 == len || len > namespace_max_size) {
        std::cerr << fmt::format("Error: namespace must be between 1 and {} characters.\n", namespace_max_size);
        return 1;
    }

    // This is the maximum size for the option name (minus 1 for null terminator).
    constexpr auto option_name_max_size = sizeof(input.option_name) - 1;
    if (const auto len = strnlen(_option_name, option_name_max_size + 1); 0 == len || len > option_name_max_size) {
        std::cerr << fmt::format("Error: option name must be between 1 and {} characters.\n", option_name_max_size);
        return 1;
    }

    // This is the maximum size for the option value (minus 1 for null terminator).
    constexpr auto option_value_max_size = sizeof(input.option_value) - 1;
    if (const auto len = strnlen(_option_value, option_value_max_size + 1); 0 == len || len > option_value_max_size) {
        std::cerr << fmt::format("Error: option value must be between 1 and {} characters.\n", option_value_max_size);
        return 1;
    }

    std::strncpy(input.name_space, _namespace, namespace_max_size);
    std::strncpy(input.option_name, _option_name, option_name_max_size);
    std::strncpy(input.option_value, _option_value, option_value_max_size);

    if (const auto ec = rc_set_grid_configuration_value(Conn, &input); ec != 0) {
        const auto msg = fmt::format("Failed to set grid configuration for namespace [{}] and option [{}] [ec={}]",
                                     _namespace,
                                     _option_name,
                                     ec);

        std::cerr << msg << "\n";

        return 1;
    }

    return 0;
} // set_grid_config

auto print_delay_server_info() -> int
{
    try {
        namespace adm = irods::experimental::administration;

        const auto user_type = adm::client::type(*Conn, adm::user{Conn->clientUser.userName, Conn->clientUser.rodsZone});

        if (!user_type) {
            rodsLogError(LOG_ERROR, CAT_INVALID_USER_TYPE, "Could not determine if user has permission to view information.");
            return 1;
        }

        if (*user_type != adm::user_type::rodsadmin) {
            rodsLogError(LOG_ERROR, CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Operation requires rodsadmin level privileges.");
            return 1;
        }
    }
    catch (const irods::exception& e) {
        rodsLogError(LOG_ERROR, e.code(), e.client_display_what());
        return 1;
    }
    catch (const std::exception& e) {
        rodsLogError(LOG_ERROR, SYS_UNKNOWN_ERROR, e.what());
        return 1;
    }

    //
    // At this point, we know the user is a rodsadmin.
    //

    nlohmann::json delay_server_info;

    GridConfigurationInput input{};
    std::strcpy(input.name_space, "delay_server");
    std::strcpy(input.option_name, "leader");

    GridConfigurationOutput* output{};

    {
        irods::at_scope_exit free_output{[&output] {
            std::free(output);
        }};

        if (const auto ec = rc_get_grid_configuration_value(Conn, &input, &output); ec != 0) {
            rodsLogError(LOG_ERROR, ec, "Failed to get delay server information.");
            return 1;
        }

        delay_server_info["leader"] = output->option_value;
    }

    std::strcpy(input.option_name, "successor");
    output = nullptr;

    irods::at_scope_exit free_output{[&output] {
        std::free(output);
    }};

    if (const auto ec = rc_get_grid_configuration_value(Conn, &input, &output); ec != 0) {
        rodsLogError(LOG_ERROR, ec, "Failed to get delay server information.");
        return 1;
    }

    delay_server_info["successor"] = output->option_value;

    std::cout << delay_server_info.dump(4) << '\n';

    return 0;
} // print_delay_server_info

auto modify_replica(
    char** tokens) -> int
{
    const std::vector<std::string_view> genquery_attrs_denylist = {
        "COLL_ID",
        "DATA_ID",
        "DATA_MAP_ID",
        "DATA_NAME",
        "DATA_RESC_HIER",
        "DATA_RESC_NAME"
    };

    const auto get_attribute_to_modify_from_input{
        [&genquery_attrs_denylist](const std::string_view attr) -> std::string_view
        {
            const auto attr_pair = std::find_if(
                std::cbegin(genquery_attrs),
                std::cend(genquery_attrs),
                [&attr](const auto& a) {
                    return 0 == attr.compare(a.first);
                });
            const auto attr_in_genquery_attrs_and_not_in_denylist{
                std::cend(genquery_attrs) != attr_pair &&
                std::none_of(
                    std::cbegin(genquery_attrs_denylist),
                    std::cend(genquery_attrs_denylist),
                    [&attr](const auto& a) {
                        return 0 == attr.compare(a);
                    })
            };
            if (!attr_in_genquery_attrs_and_not_in_denylist) {
                throw std::invalid_argument("Invalid attribute specified.");
            }
            return attr_pair->second;
        }
    };

    try {
        dataObjInfo_t info{};
        const auto args = get_args_vector(tokens, 7);
        const auto data_object_option = get_data_object_value(args[1], args[2]);
        if (const auto id = std::get_if<data_id_t>(&data_object_option)) {
            info.dataId = *id;
        }
        else if (const auto path = std::get_if<logical_path_t>(&data_object_option)) {
            rstrcpy(info.objPath, path->string().c_str(), MAX_NAME_LEN);
        }
        else {
            std::cerr << "Invalid data object option specified." << std::endl;
            return -2;
        }

        const auto replica_option = get_replica_value(args[3], args[4]);
        if (const auto num = std::get_if<replica_number_t>(&replica_option)) {
            info.replNum = *num;
        }
        else if (const auto hier = std::get_if<resource_hierarchy_t>(&replica_option)) {
            rstrcpy(info.rescHier, hier->data(), MAX_NAME_LEN);
        }
        else {
            std::cerr << "Invalid replica option specified." << std::endl;
            return -2;
        }

        const auto key = get_attribute_to_modify_from_input(args[5]);
        auto [kvp, lm] = irods::experimental::make_key_value_proxy();
        kvp[key.data()] = args[6].data();
        kvp[ADMIN_KW] = "";

        modDataObjMeta_t inp{};
        inp.regParam = kvp.get();
        inp.dataObjInfo = &info;
        if(const int status = rcModDataObjMeta(Conn, &inp); status) {
            char* sub_error_name{};
            const char* error_name = rodsErrorName(status, &sub_error_name);
            std::cerr << "rcModDataObjMeta failed when modifying replica: [" << error_name << " (" << status << ")]" << std::endl;
            printErrorStack(Conn->rError);
            return -2;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "An error occurred:\n" << e.what() << std::endl;
        return -2;
    }
    return 0;
} // modify_replica

constexpr auto is_timestamp_label(std::string_view _label) -> bool
{
    constexpr std::array timestamp_labels{"create_ts", "modify_ts"};

    return std::any_of(
        std::cbegin(timestamp_labels), std::cend(timestamp_labels), [&_label](const auto& _l) { return _l == _label; });
} // is_timestamp_label

auto print_genquery_result(std::string_view _label, std::string_view _value) -> void
{
    constexpr const char* fmt_str = "{}: {}\n";
    if (!is_timestamp_label(_label)) {
        fmt::print(fmt::runtime(fmt_str), _label, _value);
    }
    else {
        char timestamp[TIME_LEN] = "";
        if (const int ec = getLocalTimeFromRodsTime(_value.data(), timestamp); 0 == ec) {
            fmt::print(fmt::runtime(fmt_str), _label, timestamp);
        }
    }
} // print_genquery_result

auto print_genquery_results(const std::string_view _query_string, const std::vector<const char*>& _labels) -> void
{
    auto q = irods::query(Conn, _query_string.data());
    if (q.empty()) {
        fmt::print("No rows found\n");
        return;
    }

    for (auto&& result : q) {
        if (result.size() != _labels.size()) {
            THROW(SYS_INVALID_INPUT_PARAM, "Selected columns and labels must be a one-to-one mapping");
        }

        for (std::size_t l = 0; l < _labels.size(); ++l) {
            print_genquery_result(_labels.at(l), result.at(l));
        }
    }
} // print_genquery_results
} // anonymous namespace

auto set_local_zone_global() -> int
{
    if (!std::string_view{localZone}.empty()) {
        return 0;
    }

    try {
        const auto q = irods::query(Conn, "select ZONE_NAME where ZONE_TYPE = 'local'");
        if (q.empty()) {
            fmt::print(stderr, "Error getting local zone\n");
            return 1;
        }

        const auto& local_zone_name = q.front();
        if (local_zone_name.empty()) {
            fmt::print(stderr, "Error getting local zone\n");
            return 2;
        }

        std::strncpy(localZone, q.front().at(0).data(), BIG_STR);
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, "Error getting local zone\n{}\n", e.client_display_what());
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // set_local_zone_global

auto show_token(const char* _token_namespace = nullptr, const char* _token = nullptr) -> int
{
    try {
        constexpr const char* default_namespace = "token_namespace";
        const char* token_namespace =
            !_token_namespace || std::string_view{_token_namespace}.empty() ? default_namespace : _token_namespace;
        if (!_token || std::string_view{_token}.empty()) {
            const auto list_token_namespaces =
                fmt::format(fmt::runtime("select TOKEN_NAME where TOKEN_NAMESPACE = '{}'"), token_namespace);
            const auto query_string = fmt::format(fmt::runtime(list_token_namespaces), _token_namespace);

            auto q = irods::query(Conn, query_string);
            if (q.empty()) {
                fmt::print("No rows found");
                return 0;
            }

            for (auto&& result : q) {
                fmt::print("{}\n", result.at(0));
            }

            return 0;
        }

        constexpr std::array columns{
            "TOKEN_NAMESPACE",
            "TOKEN_ID",
            "TOKEN_NAME",
            "TOKEN_VALUE",
            "TOKEN_VALUE2",
            "TOKEN_VALUE3",
            "TOKEN_COMMENT",
            "TOKEN_CREATE_TIME",
            "TOKEN_MODIFY_TIME"};

        static const std::vector labels{
            "token_namespace",
            "token_id",
            "token_name",
            "token_value",
            "token_value2",
            "token_value3",
            "r_comment",
            "create_ts",
            "modify_ts"};

        const auto query_string_template = fmt::format(
            FMT_COMPILE("select {} where TOKEN_NAMESPACE = '{{}}' and TOKEN_NAME like '{{}}'"),
            fmt::join(columns, ", "));

        print_genquery_results(fmt::format(fmt::runtime(query_string_template), token_namespace, _token), labels);
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_token

auto show_group(const char* _group = nullptr) -> int
{
    try {
        if (_group && !std::string_view{_group}.empty()) {
            fmt::print("Members of group {}:\n", _group);

            constexpr const char* list_group_members =
                "select USER_NAME, USER_ZONE where USER_GROUP_NAME = '{}' and USER_TYPE != 'rodsgroup'";
            const auto query_string = fmt::format(fmt::runtime(list_group_members), _group);

            auto q = irods::query(Conn, query_string);
            if (q.empty()) {
                fmt::print("No rows found\n");
                return 0;
            }

            for (auto&& result : q) {
                const auto& user = result.at(0);
                const auto& zone = result.at(1);
                fmt::print(fmt::runtime("{}#{}\n"), user, zone);
            }
        }
        else {
            constexpr const char* query_string = "select USER_NAME where USER_TYPE = 'rodsgroup'";

            auto q = irods::query(Conn, query_string);
            if (q.empty()) {
                fmt::print("No rows found\n");
                return 0;
            }

            for (auto&& result : q) {
                fmt::print("{}\n", result.at(0));
            }
        }

        return 0;
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_group

auto show_resource(const char* _resc = nullptr) -> int
{
    try {
        if (!_resc || std::string_view{_resc}.empty()) {
            auto q = irods::query(Conn, "select RESC_NAME");
            if (q.empty()) {
                fmt::print("No rows found\n");
                return 0;
            }

            for (auto&& result : q) {
                fmt::print(fmt::runtime("{}\n"), result.at(0));
            }

            return 0;
        }

        constexpr std::array columns{
            "RESC_ID",
            "RESC_NAME",
            "RESC_ZONE_NAME",
            "RESC_TYPE_NAME",
            "RESC_LOC",
            "RESC_VAULT_PATH",
            "RESC_FREE_SPACE",
            "RESC_FREE_SPACE_TIME",
            "RESC_INFO",
            "RESC_COMMENT",
            "RESC_STATUS",
            "RESC_CREATE_TIME",
            "RESC_MODIFY_TIME",
            "RESC_CHILDREN",
            "RESC_CONTEXT",
            "RESC_PARENT",
            "RESC_PARENT_CONTEXT"};

        static const std::vector labels{
            "resc_id",
            "resc_name",
            "zone_name",
            "resc_type_name",
            "resc_net",
            "resc_def_path",
            "free_space",
            "free_space_ts",
            "resc_info",
            "r_comment",
            "resc_status",
            "create_ts",
            "modify_ts",
            "resc_children",
            "resc_context",
            "resc_parent",
            "resc_parent_context"};

        const auto query_string_template =
            fmt::format(FMT_COMPILE("select {} where RESC_NAME = '{{}}'"), fmt::join(columns, ", "));

        print_genquery_results(fmt::format(fmt::runtime(query_string_template), _resc), labels);
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_resource

auto show_zone(const char* _zone = nullptr) -> int
{
    try {
        if (!_zone || std::string_view{_zone}.empty()) {
            auto q = irods::query(Conn, "select ZONE_NAME");
            if (q.empty()) {
                fmt::print("No rows found\n");
                return 0;
            }

            for (auto&& result : q) {
                fmt::print(fmt::runtime("{}\n"), result.at(0));
            }

            return 0;
        }

        constexpr std::array columns{
            "ZONE_ID",
            "ZONE_NAME",
            "ZONE_TYPE",
            "ZONE_CONNECTION",
            "ZONE_COMMENT",
            "ZONE_CREATE_TIME",
            "ZONE_MODIFY_TIME"};

        static const std::vector labels{
            "zone_id", "zone_name", "zone_type_name", "zone_conn_string", "r_comment", "create_ts", "modify_ts"};

        const auto query_string_template =
            fmt::format(FMT_COMPILE("select {} where ZONE_NAME = '{{}}'"), fmt::join(columns, ", "));

        print_genquery_results(fmt::format(fmt::runtime(query_string_template), _zone), labels);
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_zone

auto show_user(const char* _user, const char* _zone = nullptr) -> int
{
    try {
        if (!_user || std::string_view{_user}.empty()) {
            constexpr const char* query_string = "select USER_NAME, USER_ZONE where USER_TYPE != 'rodsgroup'";

            for (auto&& result : irods::query(Conn, query_string)) {
                const auto& user = result.at(0);
                const auto& zone = result.at(1);
                fmt::print(fmt::runtime("{}#{}\n"), user, zone);
            }

            return 0;
        }

        constexpr std::array columns{
            "USER_ID",
            "USER_NAME",
            "USER_TYPE",
            "USER_ZONE",
            "USER_INFO",
            "USER_COMMENT",
            "USER_CREATE_TIME",
            "USER_MODIFY_TIME"};

        static const std::vector labels{
            "user_id", "user_name", "user_type_name", "zone_name", "user_info", "r_comment", "create_ts", "modify_ts"};

        static const auto user_query =
            fmt::format(FMT_COMPILE("select {} where USER_NAME = '{{}}'"), fmt::join(columns, ", "));

        static const auto user_query_with_zone = fmt::format(
            FMT_COMPILE("select {} where USER_NAME = '{{}}' and USER_ZONE = '{{}}'"), fmt::join(columns, ", "));

        const auto query_string = _zone && !std::string_view{_zone}.empty()
                                      ? fmt::format(fmt::runtime(user_query_with_zone), _user, _zone)
                                      : fmt::format(fmt::runtime(user_query), _user);

        print_genquery_results(query_string, labels);
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_user

auto show_user_auth(const char* _user, const char* _zone = nullptr) -> int
{
    try {
        std::string query_string;

        if (!_user || std::string_view{_user}.empty()) {
            query_string = "select USER_NAME, USER_DN";
        }
        else {
            constexpr std::array columns{"USER_NAME", "USER_DN"};

            static const auto user_query =
                fmt::format(FMT_COMPILE("select {} where USER_NAME = '{{}}'"), fmt::join(columns, ", "));

            static const auto user_query_with_zone = fmt::format(
                FMT_COMPILE("select {} where USER_NAME = '{{}}' and USER_ZONE = '{{}}'"), fmt::join(columns, ", "));

            query_string = _zone && !std::string_view{_zone}.empty()
                               ? fmt::format(fmt::runtime(user_query_with_zone), _user, _zone)
                               : fmt::format(fmt::runtime(user_query), _user);
        }

        auto q = irods::query(Conn, query_string);
        if (q.empty()) {
            fmt::print("No rows found\n");
            return 0;
        }

        for (auto&& result : q) {
            fmt::print("{} {}\n", result.at(0), result.at(1));
        }
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_user_auth

auto show_user_auth_name(const char* _auth_name) -> int
{
    if (!_auth_name || std::string_view{_auth_name}.empty()) {
        fmt::print("Provided user auth name is empty.\n");
        return 1;
    }

    try {
        auto q =
            irods::query(Conn, fmt::format(fmt::runtime("select USER_NAME, USER_DN where USER_DN = '{}'"), _auth_name));
        if (q.empty()) {
            fmt::print("No rows found\n");
            return 0;
        }

        for (auto&& result : q) {
            fmt::print("{} {}\n", result.at(0), result.at(1));
        }
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_user_auth_name

auto show_global_quotas(const char* _user_or_group = nullptr) -> int
{
    try {
        constexpr std::array columns{
            "QUOTA_USER_NAME", "QUOTA_USER_ZONE", "QUOTA_LIMIT", "QUOTA_OVER", "QUOTA_MODIFY_TIME"};
        static const std::vector labels{"user_name", "zone_name", "quota_limit", "quota_over", "modify_ts"};

        const auto user_provided = _user_or_group && !std::string_view{_user_or_group}.empty();
        if (!user_provided) {
            fmt::print("\nGlobal (total usage) quotas (if any) for users/groups:\n");

            const auto query_string =
                fmt::format(FMT_COMPILE("select {} where QUOTA_RESC_ID = '0'"), fmt::join(columns, ", "));

            print_genquery_results(query_string, labels);

            return 0;
        }

        if (const int ec = set_local_zone_global(); ec) {
            return ec;
        }

        char user_name[NAME_LEN]{};
        char zone_name[NAME_LEN]{};
        if (const int ec = parseUserName(_user_or_group, user_name, zone_name); ec) {
            return ec;
        }
        if (std::string_view{zone_name}.empty()) {
            std::strncpy(zone_name, localZone, sizeof(zone_name));
        }

        fmt::print("\nGlobal (total usage) quotas (if any) for user/group {}:\n", _user_or_group);

        const auto query_string_template = fmt::format(
            FMT_COMPILE(
                "select {} where QUOTA_USER_NAME = '{{}}' and QUOTA_USER_ZONE = '{{}}' and QUOTA_RESC_ID = '0'"),
            fmt::join(columns, ", "));

        print_genquery_results(fmt::format(fmt::runtime(query_string_template), user_name, zone_name), labels);
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_global_quotas

auto show_resource_quotas(const char* _user_or_group = nullptr) -> int
{
    try {
        constexpr std::array columns{
            "QUOTA_USER_NAME", "QUOTA_USER_ZONE", "QUOTA_RESC_NAME", "QUOTA_LIMIT", "QUOTA_OVER", "QUOTA_MODIFY_TIME"};
        static const std::vector labels{
            "user_name", "zone_name", "resc_name", "quota_limit", "quota_over", "modify_ts"};

        const auto user_provided = _user_or_group && !std::string_view{_user_or_group}.empty();
        if (!user_provided) {
            fmt::print("Per resource quotas (if any) for users/groups:\n");

            const auto query_string =
                fmt::format(FMT_COMPILE("select {} where QUOTA_RESC_ID != '0'"), fmt::join(columns, ", "));

            print_genquery_results(query_string, labels);

            return 0;
        }

        if (const int ec = set_local_zone_global(); ec) {
            return ec;
        }

        char user_name[NAME_LEN]{};
        char zone_name[NAME_LEN]{};
        if (const int ec = parseUserName(_user_or_group, user_name, zone_name); ec) {
            return ec;
        }
        if (std::string_view{zone_name}.empty()) {
            std::strncpy(zone_name, localZone, sizeof(zone_name));
        }

        fmt::print("Per resource quotas (if any) for user/group {}:\n", _user_or_group);

        const auto query_string_template = fmt::format(
            FMT_COMPILE(
                "select {} where QUOTA_USER_NAME = '{{}}' and QUOTA_USER_ZONE = '{{}}' and QUOTA_RESC_NAME != '0'"),
            fmt::join(columns, ", "));

        print_genquery_results(fmt::format(fmt::runtime(query_string_template), user_name, zone_name), labels);
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, fmt::runtime(e.client_display_what()));
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
} // show_resource_quotas

int
generalAdmin( int userOption, char *arg0, char *arg1, char *arg2, char *arg3,
              char *arg4, char *arg5, char *arg6, char *arg7, char* arg8, char* arg9,
              rodsArguments_t* _rodsArgs = 0 ) {
    /* If userOption is 1, try userAdmin if generalAdmin gets a permission
     * failure */

    generalAdminInp_t generalAdminInp;
    userAdminInp_t userAdminInp;
    int status;
    char *funcName;

    if ( _rodsArgs && _rodsArgs->dryrun ) {
        arg3 = "--dryrun";
    }

    generalAdminInp.arg0 = arg0;
    generalAdminInp.arg1 = arg1;
    generalAdminInp.arg2 = arg2;
    generalAdminInp.arg3 = arg3;
    generalAdminInp.arg4 = arg4;
    generalAdminInp.arg5 = arg5;
    generalAdminInp.arg6 = arg6;
    generalAdminInp.arg7 = arg7;
    generalAdminInp.arg8 = arg8;
    generalAdminInp.arg9 = arg9;

    status = rcGeneralAdmin( Conn, &generalAdminInp );
    lastCommandStatus = status;
    funcName = "rcGeneralAdmin";

    if ( userOption == 1 && status == SYS_NO_API_PRIV ) {
        userAdminInp.arg0 = arg0;
        userAdminInp.arg1 = arg1;
        userAdminInp.arg2 = arg2;
        userAdminInp.arg3 = arg3;
        userAdminInp.arg4 = arg4;
        userAdminInp.arg5 = arg5;
        userAdminInp.arg6 = arg6;
        userAdminInp.arg7 = arg7;
        userAdminInp.arg8 = arg8;
        userAdminInp.arg9 = arg9;
        status = rcUserAdmin( Conn, &userAdminInp );
        funcName = "rcGeneralAdmin and rcUserAdmin";
    }

    // =-=-=-=-=-=-=-
    // JMC :: for 'dryrun' option on rmresc we need to capture the
    //     :: return value and simply output either SUCCESS or FAILURE
    // rm resource dryrun BOOYA
    if ( _rodsArgs &&
            _rodsArgs->dryrun == true &&
            0 == strcmp( arg0, "rm" ) &&
            0 == strcmp( arg1, "resource" ) ) {
        if ( 0 == status ) {
            printf( "DRYRUN REMOVING RESOURCE [%s - %d] :: SUCCESS\n", arg2, status );
        }
        else {
            printf( "DRYRUN REMOVING RESOURCE [%s - %d] :: FAILURE\n", arg2, status );
        } // else
    }
    else if ( status == USER_INVALID_USERNAME_FORMAT ) {
        fprintf( stderr, "Invalid username format." );
    }
    else if ( status < 0 && status != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        char *mySubName = NULL;
        const char *myName = rodsErrorName( status, &mySubName );
        rodsLog( LOG_ERROR, "%s failed with error %d %s %s", funcName, status, myName, mySubName );
        if ( status == CAT_INVALID_USER_TYPE ) {
            fprintf( stderr, "See 'lt user_type' for a list of valid user types.\n" );
        }
        free( mySubName );
    } // else if status < 0

    printErrorStack( Conn->rError );
    freeRErrorContent( Conn->rError );

    return status;
}

/*
   Prompt for input and parse into tokens
*/
int
getInput( char *cmdToken[], int maxTokens ) {
    int lenstr, i;
    static char ttybuf[BIG_STR];
    int nTokens;
    int tokenFlag; /* 1: start reg, 2: start ", 3: start ' */
    char *cpTokenStart;
    char *cpStat;

    memset( ttybuf, 0, BIG_STR );
    fputs( "iadmin>", stdout );
    cpStat = fgets( ttybuf, BIG_STR, stdin );
    if ( cpStat == NULL ) {
        ttybuf[0] = 'q';
        ttybuf[1] = '\n';
    }
    lenstr = strlen( ttybuf );
    for ( i = 0; i < maxTokens; i++ ) {
        cmdToken[i] = "";
    }
    cpTokenStart = ttybuf;
    nTokens = 0;
    tokenFlag = 0;
    for ( i = 0; i < lenstr; i++ ) {
        if ( ttybuf[i] == '\n' ) {
            ttybuf[i] = '\0';
            cmdToken[nTokens++] = cpTokenStart;
            return 0;
        }
        if ( tokenFlag == 0 ) {
            if ( ttybuf[i] == '\'' ) {
                tokenFlag = 3;
                cpTokenStart++;
            }
            else if ( ttybuf[i] == '"' ) {
                tokenFlag = 2;
                cpTokenStart++;
            }
            else if ( ttybuf[i] == ' ' ) {
                cpTokenStart++;
            }
            else {
                tokenFlag = 1;
            }
        }
        else if ( tokenFlag == 1 ) {
            if ( ttybuf[i] == ' ' ) {
                ttybuf[i] = '\0';
                cmdToken[nTokens++] = cpTokenStart;
                cpTokenStart = &ttybuf[i + 1];
                tokenFlag = 0;
            }
        }
        else if ( tokenFlag == 2 ) {
            if ( ttybuf[i] == '"' ) {
                ttybuf[i] = '\0';
                cmdToken[nTokens++] = cpTokenStart;
                cpTokenStart = &ttybuf[i + 1];
                tokenFlag = 0;
            }
        }
        else if ( tokenFlag == 3 ) {
            if ( ttybuf[i] == '\'' ) {
                ttybuf[i] = '\0';
                cmdToken[nTokens++] = cpTokenStart;
                cpTokenStart = &ttybuf[i + 1];
                tokenFlag = 0;
            }
        }
    }
    return 0;
}

/* handle a command,
   return code is 0 if the command was (at least partially) valid,
   -1 for quitting,
   -2 for if invalid
   -3 if empty.
*/
int
doCommand( char *cmdToken[], rodsArguments_t* _rodsArgs = 0 ) {
    char buf0[MAX_PASSWORD_LEN + 10];
    char buf1[MAX_PASSWORD_LEN + 10];
    char buf2[MAX_PASSWORD_LEN + 100];
    if ( veryVerbose ) {
        int i;
        printf( "executing command:" );
        for ( i = 0; i < 20 && strlen( cmdToken[i] ) > 0; i++ ) {
            printf( " %s", cmdToken[i] );
        }
        printf( "\n" );
        fflush( stderr );
        fflush( stdout );
    }

    if ( strcmp( cmdToken[0], "help" ) == 0 ||
            strcmp( cmdToken[0], "h" ) == 0 ) {
        usage( cmdToken[1] );
        return 0;
    }
    if ( strcmp( cmdToken[0], "quit" ) == 0 ||
            strcmp( cmdToken[0], "q" ) == 0 ) {
        return -1;
    }
    if ( strcmp( cmdToken[0], "lu" ) == 0 ) {
        char userName[NAME_LEN] = "";
        char zoneName[NAME_LEN] = "";
        if (int ec = parseUserName(cmdToken[1], userName, zoneName); ec < 0) {
            return ec;
        }

        show_user(userName, zoneName);
        return 0;
    }
    if ( strcmp( cmdToken[0], "luz" ) == 0 ) {
        show_user(cmdToken[2], cmdToken[1]);
        return 0;
    }
    if ( strcmp( cmdToken[0], "lt" ) == 0 ) {
        if ( strcmp( cmdToken[1], "resc_type" ) == 0 ) {
            generalAdmin( 0, "lt", "resc_type", "", "", "", "", "", "", "", "" );
        }
        else {
            show_token(cmdToken[1], cmdToken[2]);
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "lr" ) == 0 ) {
        show_resource(cmdToken[1]);
        return 0;
    }
    if ( strcmp( cmdToken[0], "lz" ) == 0 ) {
        show_zone(cmdToken[1]);
        return 0;
    }
    if ( strcmp( cmdToken[0], "lg" ) == 0 ) {
        show_group(cmdToken[1]);
        return 0;
    }
    if ( strcmp( cmdToken[0], "lgd" ) == 0 ) {
        if ( *cmdToken[1] == '\0' ) {
            fprintf( stderr, "You must specify a group with the lgd command\n" );
        }
        else {
            show_user(cmdToken[1]);
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "lrg" ) == 0 ) {
        fprintf( stderr, "Resource groups are deprecated.\n"
                 "Please investigate the available coordinating resource plugins.\n"
                 "(e.g. random, replication, etc.)\n" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "mkuser" ) == 0 ) {
        generalAdmin(0, "add", "user", cmdToken[1], cmdToken[2], "",
                      cmdToken[3], cmdToken[4], cmdToken[5], "", "");
        return 0;
    }
    if ( strcmp( cmdToken[0], "moduser" ) == 0 ) {
        if ( strcmp( cmdToken[2], "password" ) == 0 ) {
            int i, len, lcopy;
            /* this is a random string used to pad, arbitrary, but must match
               the server side: */
            char rand[] = "1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs";

            strncpy( buf0, cmdToken[3], MAX_PASSWORD_LEN );
            len = strlen( cmdToken[3] );
            lcopy = MAX_PASSWORD_LEN - 10 - len;
            if ( lcopy > 15 ) { /* server will look for 15 characters of random string */
                strncat( buf0, rand, lcopy );
            }
            i = obfGetPw( buf1 );
            if ( i != 0 ) {
#ifdef WIN32
                HANDLE hStdin = GetStdHandle( STD_INPUT_HANDLE );
                DWORD mode;
                GetConsoleMode( hStdin, &mode );
                DWORD lastMode = mode;
                mode &= ~ENABLE_ECHO_INPUT;
                BOOL error = !SetConsoleMode( hStdin, mode );
                int errsv = -1;
#else
                struct termios tty;
                tcgetattr( STDIN_FILENO, &tty );
                tcflag_t oldflag = tty.c_lflag;
                tty.c_lflag &= ~ECHO;
                int error = tcsetattr( STDIN_FILENO, TCSANOW, &tty );
                int errsv = errno;
#endif
                if ( error ) {
                    fprintf( stderr, "WARNING: Error %d disabling echo mode. Password will be displayed in plain text.", errsv );
                }
                printf( "Enter your current iRODS password:" );
                std::string password = "";
                getline( std::cin, password );
                strncpy( buf1, password.c_str(), MAX_PASSWORD_LEN );
#ifdef WIN32
                if ( !SetConsoleMode( hStdin, lastMode ) ) {
                    fprintf( stderr, "Error reinstating echo mode." );
                }
#else
                tty.c_lflag = oldflag;
                if ( tcsetattr( STDIN_FILENO, TCSANOW, &tty ) ) {
                    fprintf( stderr, "Error reinstating echo mode." );
                }
#endif

            }
            obfEncodeByKey( buf0, buf1, buf2 );
            cmdToken[3] = buf2;
        }
        generalAdmin( 0, "modify", "user", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "aua" ) == 0 ) {
        generalAdmin( 0, "modify", "user", cmdToken[1], "addAuth",
                      cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rua" ) == 0 ) {
        generalAdmin( 0, "modify", "user", cmdToken[1], "rmAuth",
                      cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rpp" ) == 0 ) {
        generalAdmin( 0, "modify", "user", cmdToken[1], "rmPamPw",
                      cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lua" ) == 0 ) {
        char userName[NAME_LEN];
        char zoneName[NAME_LEN];
        int status;
        status = parseUserName( cmdToken[1], userName, zoneName );
        if ( status < 0 ) {
            return status;
        }

        if ( zoneName[0] != '\0' ) {
            show_user_auth(userName, zoneName);
        }
        else {
            show_user_auth(cmdToken[1]);
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "luan" ) == 0 ) {
        show_user_auth_name(cmdToken[1]);
        return 0;
    }
    if ( strcmp( cmdToken[0], "cu" ) == 0 ) {
        generalAdmin( 0, "calculate-usage", "", "", "",
                      "", "", "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "suq" ) == 0 ) {
        if(!strlen(cmdToken[1])) {
            fprintf( stderr, "ERROR: missing username parameter\n" );
        }

        if(!strlen(cmdToken[2])) {
            fprintf( stderr, "ERROR: missing resource name parameter\n" );
        }

        if(!strlen(cmdToken[3])) {
            fprintf( stderr, "ERROR: missing value parameter\n" );
        }

        generalAdmin( 0, "set-quota", "user",
                      cmdToken[1], cmdToken[2], cmdToken[3],
                      "", "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "sgq" ) == 0 ) {
        if(!strlen(cmdToken[1])) {
            fprintf( stderr, "ERROR: missing group name parameter\n" );
        }

        if(!strlen(cmdToken[2])) {
            fprintf( stderr, "ERROR: missing resource name parameter\n" );
        }

        if(!strlen(cmdToken[3])) {
            fprintf( stderr, "ERROR: missing value parameter\n" );
        }

        generalAdmin( 0, "set-quota", "group",
                      cmdToken[1], cmdToken[2], cmdToken[3],
                      "", "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "lq" ) == 0 ) {
        show_resource_quotas(cmdToken[1]);
        show_global_quotas(cmdToken[1]);
        return 0;
    }

    if ( strcmp( cmdToken[0], "mkresc" ) == 0 ) {
        // trim spaces in resource type string
        std::string resc_type( cmdToken[2] );
        resc_type.erase( std::remove_if( resc_type.begin(), resc_type.end(), ::isspace ), resc_type.end() );

        // tell the user what they are doing
        std::cout << "Creating resource:" << std::endl;
        std::cout << "Name:\t\t\"" << cmdToken[1] << "\"" << std::endl;
        std::cout << "Type:\t\t\"" << cmdToken[2] << "\"" << std::endl;
        if ( cmdToken[3] != NULL && strlen( cmdToken[3] ) > 0 ) {
            std::string host_path( cmdToken[3] );
            std::size_t colon_pos = host_path.find( ":" );
            std::string host = host_path.substr( 0, colon_pos );
            std::cout << "Host:\t\t\"" << host << "\"" << std::endl;
            if ( colon_pos != std::string::npos ) {
                std::string path = host_path.substr( colon_pos + 1, host_path.length() - colon_pos );
                std::cout << "Path:\t\t\"" << path << "\"" << std::endl;
            }
            else {
                std::cout << "Path:\t\t\"\"" << std::endl;
            }
        }
        else {
            std::cout << "Host:\t\t\"\"" << std::endl;
            std::cout << "Path:\t\t\"\"" << std::endl;
        }
        if ( cmdToken[4] != NULL && strlen( cmdToken[4] ) > 0 ) {
            std::cout << "Context:\t\"" << cmdToken[4] << "\"" << std::endl;;
        }
        else {
            std::cout << "Context:\t\"\"" << std::endl;
        }

        generalAdmin( 0, "add", "resource", cmdToken[1], ( char * )resc_type.c_str(),
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], cmdToken[7], cmdToken[8] );
        /* (add resource name type host:path contextstring) */
        return 0;
    }
    if ( strcmp( cmdToken[0], "addchildtoresc" ) == 0 ) {

        generalAdmin( 0, "add", "childtoresc", cmdToken[1], cmdToken[2],
                      cmdToken[3], "", "", "", "", "" );
        /* (add childtoresc parent child context) */
        return 0;
    }
    if ( strcmp( cmdToken[0], "rmchildfromresc" ) == 0 ) {

        generalAdmin( 0, "rm", "childfromresc", cmdToken[1], cmdToken[2],
                      "", "", "", "", "", "" );
        /* (rm childfromresc parent child) */
        return 0;
    }

    if ( strcmp( cmdToken[0], "modrescdatapaths" ) == 0 ) {
        printf(
            "Warning, this command, more than others, is relying on your direct\n"
            "input to modify iCAT tables (potentially, many rows).  It will do a\n"
            "string pattern find and replace operation on the main data-object\n"
            "table for the paths at which the physical files are stored. If you\n"
            "are not sure what you are doing, do not run this command.  You may\n"
            "want to backup the iCAT database before running this.  See the help\n"
            "text for more information.\n"
            "\n"
            "Are you sure you want to run this command? [y/N]:" );
        std::string response = "";
        getline( std::cin, response );
        if ( response == "y" || response == "yes" ) {
            printf( "OK, performing the resource data paths update\n" );

            generalAdmin( 0, "modify", "resourcedatapaths", cmdToken[1],
                          cmdToken[2], cmdToken[3], cmdToken[4], "", "", "", "" );
        }
        return 0;
    }

    if ( strcmp( cmdToken[0], "modresc" ) == 0 ) {
        if ( strcmp( cmdToken[2], "name" ) == 0 )       {
            printf(
                "If you modify a resource name, you and other users will need to\n"
                "change your irods_environment.json files to use it, you may need to update\n"
                "server_config.json and, if rules use the resource name, you'll need to\n"
                "update the core rules (core.re).  This command will update various\n"
                "tables with the new name.\n"
                "Do you really want to modify the resource name? (enter y or yes to do so):" );
            std::string response = "";
            getline( std::cin, response );
            if ( response == "y" || response == "yes" ) {
                printf( "OK, performing the resource rename\n" );
                int stat;
                stat = generalAdmin( 0, "modify", "resource", cmdToken[1], cmdToken[2],
                                     cmdToken[3], "", "", "", "", "" );
                if ( strcmp( cmdToken[2], "path" ) == 0 ) {
                    if ( stat == 0 ) {
                        printf(
                            "Modify resource path was successful.\n"
                            "If the existing iRODS files have been physically moved,\n"
                            "you may want to run 'iadmin modrescdatapaths' with the old\n"
                            "and new path.  See 'iadmin h modrescdatapaths' for more information.\n" );
                    }
                }

            }
            else {
                printf( "Resource rename aborted\n" );
            }
        }
        else {
            if ( strcmp( cmdToken[2], "type" ) == 0 ) { 
                // trim spaces in resource type string
                std::string resc_type( cmdToken[3] );
                resc_type.erase( std::remove_if( resc_type.begin(), resc_type.end(), ::isspace ), resc_type.end() );
                memset( cmdToken[3], 0, strlen( cmdToken[3] ) );
                snprintf( cmdToken[3], resc_type.length()+1, "%s", resc_type.c_str() );
            }

            generalAdmin( 0, "modify", "resource", cmdToken[1], cmdToken[2],
                          cmdToken[3], "", "", "", "", "" );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "mkzone" ) == 0 ) {
        generalAdmin( 0, "add", "zone", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], "", "", "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "modzone" ) == 0 ) {
        if ( strcmp( myEnv.rodsZone, cmdToken[1] ) == 0 &&
                strcmp( cmdToken[2], "name" ) == 0 )       {
            printf(
                "If you modify the local zone name, you and other users will need to\n"
                "change your irods_environment.json files to use it, you may need to update\n"
                "server_config.json and, if rules use the zone name, you'll need to update\n"
                "core.re.  This command will update various tables with the new name\n"
                "and rename the top-level collection.\n"
                "Do you really want to modify the local zone name? (enter y or yes to do so):" );
            std::string response = "";
            getline( std::cin, response );
            if ( response == "y" || response == "yes" ) {
                printf( "OK, performing the local zone rename\n" );
                generalAdmin( 0, "modify", "localzonename", cmdToken[1], cmdToken[3],
                              "", "", "", "", "", "" );
            }
            else {
                printf( "Local zone rename aborted\n" );
            }
        }
        else {
            generalAdmin( 0, "modify", "zone", cmdToken[1], cmdToken[2],
                          cmdToken[3], "", "", "", "", "" );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "modzonecollacl" ) == 0 ) {
        generalAdmin( 0, "modify", "zonecollacl", cmdToken[1], cmdToken[2],
                      cmdToken[3], "", "", "", "", "" );
        return 0;
    }


    if ( strcmp( cmdToken[0], "rmzone" ) == 0 ) {
        generalAdmin( 0, "rm", "zone", cmdToken[1], "",
                      "", "", "", "", "", "" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "mkgroup" ) == 0 ) {
        const auto version = irods::to_version(Conn->svrVersion->relVersion);
        if (version && version.value() > irods::version{4, 3, 3}) {
            // If we are adding a group, we don't need to specify a user type - it's a group.
            generalAdmin(0, "add", "group", cmdToken[1], "", "", "", "", "", "", "");
        }
        else {
            // Versions 4.3.3 and earlier only know how to "add" a "user" with a "rodsgroup" type.
            generalAdmin(0, "add", "user", cmdToken[1], "rodsgroup", "", "", "", "", "", "");
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "rmgroup" ) == 0 ) {
        const auto version = irods::to_version(Conn->svrVersion->relVersion);
        const char* entity_type = (version && version.value() > irods::version{4, 3, 1}) ? "group" : "user";
        generalAdmin(0, "rm", const_cast<char*>(entity_type), cmdToken[1], myEnv.rodsZone, "", "", "", "", "", "");
        return 0;
    }

    if ( strcmp( cmdToken[0], "atg" ) == 0 ) {
        generalAdmin( 1, "modify", "group", cmdToken[1], "add", cmdToken[2],
                      cmdToken[3], "", "", "", "" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "rfg" ) == 0 ) {
        generalAdmin( 1, "modify", "group", cmdToken[1], "remove", cmdToken[2],
                      cmdToken[3], "", "", "", "" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "atrg" ) == 0 ) {
        fprintf( stderr,
                 "Resource groups are deprecated.\n"
                 "Please investigate the available coordinating resource plugins.\n"
                 "(e.g. random, replication, etc.)\n" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "rfrg" ) == 0 ) {
        fprintf( stderr,
                 "Resource groups are deprecated.\n"
                 "Please investigate the available coordinating resource plugins.\n"
                 "(e.g. random, replication, etc.)\n" );
        return 0;
    }

    if ( strcmp( cmdToken[0], "rmresc" ) == 0 ) {
        generalAdmin( 0, "rm", "resource", cmdToken[1], cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "", _rodsArgs );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rmuser" ) == 0 ) {
        generalAdmin( 0, "rm", "user", cmdToken[1],
                      cmdToken[2], cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "at" ) == 0 ) {
        generalAdmin( 0, "add", "token", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "rt" ) == 0 ) {
        generalAdmin( 0, "rm", "token", cmdToken[1], cmdToken[2],
                      cmdToken[3], cmdToken[4], cmdToken[5], cmdToken[6], "", "" );
        return 0;
    }
    if ( strcmp( cmdToken[0], "spass" ) == 0 ) {
        char scrambled[MAX_PASSWORD_LEN + 100];
        if ( strlen( cmdToken[1] ) > MAX_PASSWORD_LEN - 2 ) {
            fprintf( stderr, "Password exceeds maximum length\n" );
        }
        else {
            if ( strlen( cmdToken[2] ) == 0 ) {
                fprintf( stderr, "Warning, scramble key is null\n" );
            }
            obfEncodeByKey( cmdToken[1], cmdToken[2], scrambled );
            printf( "Scrambled form is:%s\n", scrambled );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "dspass" ) == 0 ) {
        char unscrambled[MAX_PASSWORD_LEN + 100];
        if ( strlen( cmdToken[1] ) > MAX_PASSWORD_LEN - 2 ) {
            fprintf( stderr, "Scrambled password exceeds maximum length\n" );
        }
        else {
            if ( strlen( cmdToken[2] ) == 0 ) {
                fprintf( stderr, "Warning, scramble key is null\n" );
            }
            obfDecodeByKey( cmdToken[1], cmdToken[2], unscrambled );
            printf( "Unscrambled form is:%s\n", unscrambled );
        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "rum" ) == 0 ) {
        int status;
        status = generalAdmin( 0, "rm", "unusedAVUs", "", "",
                               "", "", "", "", "", "" );
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
            printf(
                "The return of CAT_SUCCESS_BUT_WITH_NO_INFO in this case means that the\n"
                "SQL succeeded but there were no rows removed; there were no unused\n"
                "AVUs to remove.\n" );
            lastCommandStatus = 0;

        }
        return 0;
    }
    if ( strcmp( cmdToken[0], "asq" ) == 0 ) {
        int status;
        status = generalAdmin( 0, "add", "specificQuery", cmdToken[1],
                               cmdToken[2], "", "", "", "", "", "" );
        return status;
    }
    if ( strcmp( cmdToken[0], "rsq" ) == 0 ) {
        int status;
        status = generalAdmin( 0, "rm", "specificQuery", cmdToken[1],
                               "", "", "", "", "", "", "" );
        return status;
    }

    if (0 == strcmp(cmdToken[0], "modrepl")) {
        return modify_replica(cmdToken);
    }

    if (0 == strcmp(cmdToken[0], "get_grid_configuration")) {
        return print_grid_config(cmdToken[1], cmdToken[2]);
    }

    if (0 == strcmp(cmdToken[0], "set_grid_configuration")) {
        return set_grid_config(cmdToken[1], cmdToken[2], cmdToken[3]);
    }

    if (0 == strcmp(cmdToken[0], "get_delay_server_info")) {
        return print_delay_server_info();
    }

    if (0 == strcmp(cmdToken[0], "set_delay_server")) {
        return generalAdmin( 0, "set_delay_server", cmdToken[1],
                               "", "", "", "", "", "", "", "" );
    }

    /* test is only used for testing so is not included in the help */
    if ( strcmp( cmdToken[0], "test" ) == 0 ) {
        char* result;
        result = obfGetMD5Hash( cmdToken[1] );
        printf( "md5:%s\n", result );
        return 0;
    }
    /* 2spass is only used for testing so is not included in the help */
    if ( strcmp( cmdToken[0], "2spass" ) == 0 ) {
        char scrambled[MAX_PASSWORD_LEN + 10];
        obfEncodeByKeyV2( cmdToken[1], cmdToken[2], cmdToken[3], scrambled );
        printf( "Version 2 scrambled form is:%s\n", scrambled );
        return 0;
    }
    /* 2dspass is only used for testing so is not included in the help */
    if ( strcmp( cmdToken[0], "2dspass" ) == 0 ) {
        char unscrambled[MAX_PASSWORD_LEN + 10];
        obfDecodeByKeyV2( cmdToken[1], cmdToken[2], cmdToken[3], unscrambled );
        printf( "Version 2 unscrambled form is:%s\n", unscrambled );
        return 0;
    }
    if ( *cmdToken[0] != '\0' ) {
        fprintf( stderr, "unrecognized command, try 'help'\n" );
        return -2;
    }
    return -3;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    rodsArguments_t myRodsArgs;
    int status = parseCmdLineOpt( argc, argv, "fvVhZ", 1, &myRodsArgs );

    if ( status ) {
        fprintf( stderr, "Use -h for help.\n" );
        return 2;
    }
    if ( myRodsArgs.help == True ) {
        usage( "" );
        return 0;
    }
    int argOffset = myRodsArgs.optind;


    if ( myRodsArgs.veryVerbose == True ) {
        veryVerbose = 1;
    }

    int i;
    const int maxCmdTokens = 20;
    char *cmdToken[maxCmdTokens];
    for ( i = 0; i < maxCmdTokens; i++ ) {
        cmdToken[i] = "";
    }
    int j = 0;
    for ( i = argOffset; i < argc; i++ ) {
        cmdToken[j++] = argv[i];
    }

    if ( strcmp( cmdToken[0], "help" ) == 0 ||
            strcmp( cmdToken[0], "h" ) == 0 ) {
        usage( cmdToken[1] );
        return 0;
    }

    if ( strcmp( cmdToken[0], "spass" ) == 0 ) {
        char scrambled[MAX_PASSWORD_LEN + 100];
        if ( strlen( cmdToken[1] ) > MAX_PASSWORD_LEN - 2 ) {
            fprintf( stderr, "Password exceeds maximum length\n" );
        }
        else {
            if ( strlen( cmdToken[2] ) == 0 ) {
                fprintf( stderr, "Warning, scramble key is null\n" );
            }
            obfEncodeByKey( cmdToken[1], cmdToken[2], scrambled );
            printf( "Scrambled form is:%s\n", scrambled );
        }
        return 0;
    }

    if ( strcmp( cmdToken[0], "dspass" ) == 0 ) {
        char unscrambled[MAX_PASSWORD_LEN + 100];

        std::string password;
        std::string key;

        if ( strlen( cmdToken[1] ) == 0 ) {
            std::getline( std::cin, password );
            cmdToken[1] = const_cast<char*>( password.c_str() );

            std::getline( std::cin, key );
            cmdToken[2] = const_cast<char*>( key.c_str() );
        }

        if ( strlen( cmdToken[1] ) > MAX_PASSWORD_LEN - 2 ) {
            fprintf( stderr, "Scrambled password exceeds maximum length\n" );
        }
        else {
            if ( strlen( cmdToken[2] ) == 0 ) {
                fprintf( stderr, "Warning, scramble key is null\n" );
            }
            obfDecodeByKey( cmdToken[1], cmdToken[2], unscrambled );
            printf( "Unscrambled form is:%s\n", unscrambled );
        }
        return 0;
    }

    memset( &myEnv, 0, sizeof( myEnv ) );
    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        return 1;
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    rErrMsg_t errMsg;
    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        char *mySubName = NULL;
        const char *myName = rodsErrorName( errMsg.status, &mySubName );
        rodsLog( LOG_ERROR, "rcConnect failure %s (%s) (%d) %s",
                 myName,
                 mySubName,
                 errMsg.status,
                 errMsg.msg );
        free( mySubName );
        return 2;
    }

    const auto disconnect = irods::at_scope_exit{[] { rcDisconnect(Conn); }};

    status = utils::authenticate_client(Conn, myEnv);
    if ( status != 0 ) {
        print_error_stack_to_file(Conn->rError, stderr);
        return 3;
    }

    // If the user is not a rodsadmin, they should not be allowed to execute admin commands.
    try {
        namespace adm = irods::experimental::administration;

        const auto user_type =
            adm::client::type(*Conn, adm::user{Conn->clientUser.userName, Conn->clientUser.rodsZone});

        if (!user_type) {
            rodsLogError(
                LOG_ERROR, CAT_INVALID_USER_TYPE, "Could not determine if user has permission to view information.");
            return 5;
        }

        if (*user_type != adm::user_type::rodsadmin) {
            rodsLogError(LOG_ERROR, CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "Operation requires rodsadmin level privileges.");
            return 6;
        }
    }
    catch (const irods::exception& e) {
        rodsLogError(LOG_ERROR, e.code(), e.client_display_what());
        return 7;
    }
    catch (const std::exception& e) {
        rodsLogError(LOG_ERROR, SYS_UNKNOWN_ERROR, e.what());
        return 8;
    }

    bool keepGoing = argc == 1;
    bool firstTime = true;
    do {
        int status = doCommand( cmdToken, &myRodsArgs );
        // -1 is "quitting"-- will exit with zero 
        if ( status == -1 ) {
            keepGoing = false;
        }
        if ( firstTime ) {
            if (0 == status) {
                keepGoing = false;
            }
            // -2 is "invalid command"-- will exit nonzero
            if (-2 == status) {
                keepGoing = false;
                lastCommandStatus = -2;
            }
            firstTime = false;
        }
        if ( keepGoing ) {
            getInput( cmdToken, maxCmdTokens );
        }
    }
    while ( keepGoing );

    printErrorStack( Conn->rError );

    if ( lastCommandStatus != 0 ) {
        return 4;
    }
    return 0;
}

void
printMsgs( char *msgs[] ) {
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            return;
        }
        printf( "%s\n", msgs[i] );
    }
}

void usageMain() {
    char *Msgs[] = {
        "Usage: iadmin [-hvV] [command]",
        "A blank execute line invokes the interactive mode, where it",
        "prompts and executes commands until 'quit' or 'q' is entered.",
        "Single or double quotes can be used to enter items with blanks.",
        "Commands are:",
        " lu [name[#Zone]] (list user info; details if name entered)",
        " lua [name[#Zone]] (list user authentication (GSI/Kerberos Names, if any))",
        " luan Name (list users associated with auth name (GSI/Kerberos)",
        " lt [name] [subname] (list token info)",
        " lr [name] (list resource info)",
        " lz [name] (list zone info)",
        " lg [name] (list group info (user member list))",
        " lgd name  (list group details)",
        " mkuser Name[#Zone] Type (make user)",
        " moduser Name[#Zone] [ type | comment | info | password ] newValue",
        " aua Name[#Zone] Auth-Name (add user authentication-name (GSI/Kerberos)",
        " rua Name[#Zone] Auth-Name (remove user authentication name (GSI/Kerberos)",
        " rpp Name  (remove PAM-derived Password for user Name)",
        " rmuser Name[#Zone] (remove user, where userName: name[@department][#zone])",
        " mkresc Name Type [Host:Path] [ContextString] (make Resource)",
        " modresc Name [name, type, host, path, status, comment, info, free_space, context, rebalance] Value (mod Resc)",
        " modrescdatapaths Name oldpath newpath [user] (update data-object paths,",
        "      sometimes needed after modresc path)",
        " rmresc Name (remove resource)",
        " addchildtoresc Parent Child [ContextString]",
        " rmchildfromresc Parent Child",
        " mkzone Name Type(remote) [Connection-info] [Comment] (make zone)",
        " modzone Name [ name | conn | comment ] newValue  (modify zone)",
        " modzonecollacl null|read userOrGroup /remotezone (set strict-mode root ACLs)",
        " rmzone Name (remove zone)",
        " mkgroup Name (make group)",
        " rmgroup Name (remove group)",
        " atg groupName userName[#Zone] (add to group - add a user to a group)",
        " rfg groupName userName[#Zone] (remove from group - remove a user from a group)",
        " at tokenNamespace Name [Value1] [Value2] [Value3] (add token) ",
        " rt tokenNamespace Name [Value1] (remove token) ",
        " spass Password Key (print a scrambled form of a password for DB)",
        " dspass Password Key (descramble a password and print it)",
        " suq User Target Value (set user quota)",
        " sgq Group Target Value (set group quota)",
        " lq [Name] List Quotas",
        " cu (calculate usage (for quotas))",
        " rum (remove unused metadata (user-defined AVUs)",
        " asq 'SQL query' [Alias] (add specific query)",
        " rsq 'SQL query' or Alias (remove specific query)",
        " modrepl [logical_path <string>|data_id <int>] [replica_number <int>|resource_hierarchy <string>] ATTR_NAME VALUE",
        " get_delay_server_info",
        " set_delay_server HOSTNAME",
        " get_grid_configuration NAMESPACE OPTION_NAME",
        " set_grid_configuration NAMESPACE OPTION_NAME OPTION_VALUE",
        " help (or h) [command] (this help, or more details on a command)",
        "Also see 'irmtrash -M -u user' for the admin mode of removing trash and",
        "similar admin modes in irepl, iphymv, and itrim.",
        "The admin can also alias as any user via the 'clientUserName' environment",
        "variable.",
        ""
    };
    printMsgs( Msgs );
    printReleaseInfo( "iadmin" );
}

void
usage( char *subOpt ) {
    char *luMsgs[] = {
        "lu [name] (list user info; details if name entered)",
        "list user information.",
        "Just 'lu' will briefly list currently defined users.",
        "If you include a user name, more detailed information is provided.",
        "Usernames can include the zone preceded by #, for example rods#tempZone.",
        "Users are listed in the userName#ZoneName form.",
        "Also see the 'luz', 'lz', and 'iuserinfo' commands.",
        ""
    };
    char *luaMsgs[] = {
        "lua [name[#Zone]] (list user authentication (GSI/Kerberos Names), if any)",
        "list user authentication-names for one or all users",
        "Just 'lua' will list all the GSI/Kerberos names currently defined",
        "for all users along with the associated iRODS user names.",
        "If you include a user name, then the auth-names for that user are listed.",
        "Usernames can include the zone preceded by #, for example rods#tempZone.",
        "Also see the 'luan', 'aua', 'rua', and 'iuserinfo' commands.",
        ""
    };
    char *luanMsgs[] = {
        "luan Name (list users associated with auth name (GSI/Kerberos)",
        "list the user(s) associated with a give Authentication-Name  ",
        "For example:",
        "  luan '/C=US/O=INC/OU=DICE/CN=Wayne Schroeder/UID=schroeder'",
        "will list the iRODS user(s) with the GSI DN, if any.",
        ""
    };
    char *luzMsgs[] = {
        "luz Zone [User] (list user info for a Zone; details if name entered)",
        "list user information for users of a particular Zone.  ",
        "Just 'luz Zonename' will briefly list currently defined users of that Zone.",
        "If you include a user name, more detailed information is provided.",
        "Also see the lu and lz commands.",
        ""
    };
    char *ltMsgs[] = {
        "lt [name] [subname]",
        "list token information.",
        "Just 'lt' lists the types of tokens that are defined",
        "If you include a tokenname, it will list the values that are",
        "allowed for the token type.  For details, lt name subname, ",
        "for example: lt data_type email",
        "The sql wildcard character % can be used on the subname,",
        "for example: lt data_type %DLL",
        ""
    };
    char *lrMsgs[] = {
        "lr [name] (list resource info)",
        "Just 'lr' briefly lists the defined resources.",
        "If you include a resource name, it will list more detailed information.",
        ""
    };
    char *lzMsgs[] = {
        " lz [name] (list zone info)",
        "Just 'lz' briefly lists the defined zone(s).",
        "If you include a zone name, it will list more detailed information.",
        ""
    };
    char *lgMsgs[] = {
        " lg [name] (list group info (user member list))",
        "Just 'lg' briefly lists the defined groups.",
        "If you include a group name, it will list users who are",
        "members of that group.  Users are listed in the user#zone format.",
        "In addition to 'rodsadmin', any user can use this sub-command; this is",
        "of most value to 'groupadmin' users who can also 'atg' and 'rfg'",
        ""
    };

    char *lgdMsgs[] = {
        " lgd name (list group details)",
        "Lists some details about the user group.",
        ""
    };
    char *mkuserMsgs[] = {
        " mkuser Name[#Zone] Type (make user)",
        "Create a new iRODS user in the ICAT database",
        " ",
        "Name is the user name to create",
        "Type is the user type (see 'lt user_type' for a list)",
        "Zone is the user's zone (for remote-zone users)",
        " ",
        "Tip: Use moduser to set a password or other attributes,",
        "     use 'aua' to add a user auth name (GSI DN or Kerberos Principal name)",
        ""
    };

    char *spassMsgs[] = {
        " spass Password Key (print a scrambled form of a password for DB)",
        "Scramble a password, using the input password and key.",
        "This is used during the installation for a little additional security",
        ""
    };

    char *dspassMsgs[] = {
        " dspass Password Key (descramble a password and print it)",
        "Descramble a password, using the input scrambled password and key",
        ""
    };


    char *moduserMsgs[] = {
        " moduser Name[#Zone] [ type | comment | info | password ] newValue",
        "Modifies a field of an existing user definition.",
        "For password authentication, use moduser to set the password.",
        "(The password is transferred in a scrambled form to be more secure.)",
        "Long forms of the field names may also be used:",
        "user_name, user_type_name, zone_name, user_info, or ",
        "r_comment",
        "These are the names listed by 'lu' (and are the database table column names).",
        "Modifying the user's name or zone is not allowed; instead remove the user and",
        "create a new one.  rmuser/mkuser will remove (if empty) and create the needed",
        "collections, too.",
        "For GSI or Kerberos authentication, use 'aua' to add one or more",
        "user auth names (GSI Distinquished Name (DN) or Kerberos principal name).",
        ""
    };

    char *auaMsgs[] = {
        " aua Name[#Zone] Auth-Name (add user authentication-name (GSI/Kerberos)",
        "Add a user authentication name, a GSI  Distinquished Name (DN) or",
        "Kerberos Principal name, to an iRODS user.  Multiple DNs and/or Principal",
        "names can be associated with each user.",
        "This is used with Kerberos and/or GSI authentication, if enabled.",
        "For example:",
        "  aua rods '/C=US/O=INC/OU=DICE/CN=Wayne Schroeder/UID=schroeder'",
        "Also see 'rua', 'lua', and 'luan'.",
        ""
    };

    char *ruaMsgs[] = {
        " rua Name[#Zone] Auth-Name (remove user authentication-name (GSI/Kerberos)",
        "Remove a user authentication name, a GSI Distinquished Name (DN) or",
        "Kerberos Principal name, from being associated with an iRODS user.",
        "These are used with Kerberos and/or GSI authentication, if enabled.",
        "Also see 'aua', 'lua', and 'luan'.",
        ""
    };

    char *rppMsgs[] = {
        " rpp Name (remove PAM-derived Password for user Name)",
        "Remove iRODS short-term (usually 2 weeks) passwords that are created",
        "when users authenticate via the iRODS PAM authentication method.",
        "For additional security, when using PAM (system passwords), 'iinit' will",
        "create a separate iRODS password that is then used (a subsequent 'iinit'",
        "extend its 'life').  If the user's system password is changed, you",
        "may want to use this rpp command to require the user to re-authenticate.",
        ""
    };


    char *rmuserMsgs[] = {
        " rmuser Name[#Zone] (remove user, where userName: name[@department][#zone])",
        " Remove an iRODS user.",
        ""
    };

    char *mkrescMsgs[] = {
        " mkresc Name Type [Host:Path] [ContextString] (make Resource)",
        "Create (register) a new coordinating or storage resource.",
        " ",
        "Name is the name of the new resource.",
        "Type is the resource type.",
        "Host is the DNS host name.",
        "Path is the defaultPath for the vault.",
        "ContextString is any contextual information relevant to this resource.",
        "  (semi-colon separated key=value pairs e.g. \"a=b;c=d\")",
        " ",
        "A ContextString can be added to a coordinating resource (where there is",
        "no hostname or vault path to be set) by explicitly setting the Host:Path",
        "to an empty string ('').",
        " ",
        "A list of available resource types can be shown with:",
        "  iadmin lt resc_type",
        ""
    };

    char *modrescMsgs[] = {
        " modresc Name [name, type, host, path, status, comment, info, free_space, context, rebalance] Value",
        "         (modify Resource)",
        "Change some attribute of a resource.  For example:",
        "    modresc demoResc comment 'test resource'",
        "The 'host' field is the DNS host name, for example 'offsite.example.org',",
        "this is displayed as 'resc_net', the resource network address.",
        " ",
        "Setting the resource status to '" RESC_DOWN "' will cause iRODS to ignore that",
        "resource and bypass communications with that server. Using any other string will",
        "restore use of the resource.",
        " ",
        "The free_space value can be simply specified, or if it starts with + or -",
        "the free_space amount will be incremented or decremented by the value.",
        " ",
        "'context' is any contextual information relevant to this resource.",
        "  (semi-colon separated key=value pairs e.g. \"a=b;c=d\")",
        " ",
        "'rebalance' will trigger the rebalancing operation on a coordinating resource node.",
        " ",
        "Running 'iadmin modresc <rescName> rebalance' will check if",
        "a rebalance is already running for <rescName> by looking for an",
        "AVU on the named resource matching an attribute 'rebalance_operation'.",
        " ",
        "If it finds a match, it will exit early and return",
        "REBALANCE_ALREADY_ACTIVE_ON_RESOURCE.",
        " ",
        "An active (or stale) rebalance will appear in the catalog:",
        " ",
        "  $ imeta ls -R demoResc",
        "  AVUs defined for resource demoResc:",
        "  attribute: rebalance_operation",
        "  value: x.x.x.x:7294",
        "  units: 20180203T140006Z",
        " ",
        "When a rebalance completes successfully, the timestamp AVU is removed.",
        ""
    };

    char *modrescDataPathsMsgs[] = {
        " modrescdatapaths Name oldpath newpath [user] (update data-object paths,",
        "      sometimes needed after modresc path)",

        " ",
        "Modify the paths for existing iRODS files (data-objects) to match a",
        "change of the resource path that had been done via 'iadmin modresc",
        "Resc path'.  This is only needed if the physical files and directories",
        "of existing iRODS files have been moved, via tools outside of iRODS;",
        "i.e the physical resource has been moved.  If you only changed the",
        "path so that new files will be stored under the new path directory,",
        "you do not need to run this.",
        " ",
        "Each data-object has a physical path associated with it.  If the old",
        "physical paths are no longer valid, you can update them via this.  It",
        "will change the beginning part of the path (the Vault) from the old",
        "path to the new.",
        " ",
        "This does a pattern replacement on the paths for files in the named",
        "resource.  The old and new path strings must begin and end with a",
        "slash (/) to make it more likely the correct patterns are replaced",
        "(should the pattern repeat within a path).",
        " ",
        "If you include the optional user, only iRODS files owned by that",
        "user will be updated.",
        " ",
        "When the command runs, it will tell you how many data-object rows",
        "have been updated.",
        " ",
        "The 'iadmin modresc Rescname path' command now returns the previous",
        "path of the resource which can be used as the oldPath for this",
        "modrescdatapaths command.  It also refers the user to this command.",
        " ",
        "To see if you have any files under a given path, use iquest, for example:",
        "iquest \"select count(DATA_ID) where DATA_PATH like '/iRODS/Vault3/%'\" ",
        "And to restrict it to a certain user add:",
        " and USER_NAME = 'name' ",
        ""
    };

    char *rmrescMsgs[] = {
        " rmresc Name (remove resource)",
        "Remove a composable resource.",
        " ",
        "The (coordinating or storage) resource to be removed must be both:",
        " - empty (no data objects may be stored on it, including its trash)",
        " - standalone (it cannot have a parent or a child resource)",
        ""
    };

    char* addchildtorescMsgs[] = {
        " addchildtoresc Parent Child [ContextString] (add child to resource)",
        "Add a child resource to a parent resource.  This creates an 'edge'",
        "between two nodes in a resource tree.",
        " ",
        "Parent is the name of the parent resource.",
        "Child is the name of the child resource.",
        "ContextString is any relevant information that the parent may need in order",
        "  to manage the child.",
        ""
    };

    char* rmchildfromrescMsgs[] = {
        " rmchildfromresc Parent Child (remove child from resource)",
        "Remove a child resource from a parent resource.  This removes an 'edge'",
        "between two nodes in a resource tree.",
        " ",
        "Parent is the name of the parent resource.",
        "Child is the name of the child resource.",
        ""
    };

    char *mkzoneMsgs[] = {
        " mkzone Name Type(remote) [Connection-info] [Comment] (make zone)",
        "Create a new zone definition.  Type must be 'remote' as the local zone",
        "must previously exist and there can be only one local zone definition.",
        "Connection-info (hostname:port) and a Comment field are optional.",
        " ",
        "The connection-info should be the hostname of a Catalog Service Provider",
        "of the zone.  If it is a Catalog Service Consumer, remote users trying to",
        "connect will get a CAT_INVALID_USER error.",
        " ",
        "Also see modzone, rmzone, and lz.",
        ""
    };

    char *modzoneMsgs[] = {
        " modzone Name [ name | conn | comment ] newValue  (modify zone)",
        "Modify values in a zone definition, either the name, conn (connection-info),",
        "or comment.  Connection-info is the DNS host string:port, for example:",
        "irods.example.org:1247",
        "When modifying the conn information, it should be the hostname of a",
        "Catalog Service Provider; see 'h mkzone' for more.",
        " ",
        "The name of the local zone can be changed via some special processing and",
        "since it also requires some manual changes, iadmin will explain those and",
        "prompt for confirmation in this case.",
        ""
    };
    char *modzonecollaclMsgs[] = {
        " modzonecollacl null|read userOrGroup /remotezone (set strict-mode root ACLs)",
        "Modify a remote zone's local collection for strict-mode access.",
        " ",
        "This is only needed if you are running with strict access control",
        "enabled (see acAclPolicy in core.re) and you want users to be able to",
        "see (via 'ils /' or other queries) the existing remote zones in the",
        "root ('/') collection.",
        " ",
        "The problem only occurs at the '/' level because for zones there are",
        "both local and remote collections for the zone. As with any query in",
        "strict mode, when the user asks for information on a collection, the",
        "iCAT-generated SQL adds checks to restrict results to data-objects or",
        "sub-collections in that collection to which the user has read or",
        "better access. The problem is that collections for the remote zones",
        "(/zone) do not have ACLs set, even if ichmod is run try to give it",
        "(e.g. read access to public) because ichmod (like ils, iget, iput,",
        "etc) communicates to the appropriate zone based on the beginning part",
        "of the collection name.",
        " ",
        "The following iquest command returns the local ACLs (tempZone is the",
        "local zone and r3 is a remote zone):",
        "  iquest -z tempZone \"select COLL_ACCESS_TYPE where COLL_NAME = '/r3'\" ",
        "The '-z tempZone' is needed to have it connect locally instead of to the",
        "remote r3 zone.  Normally there will be one row returned for the",
        "owner.  With this command, others can be added.  Note that 'ils -A /r3'",
        "will also check with the remote zone, so use the above iquest",
        "command to see the local information.",
        " ",
        "The command syntax is similar to ichmod:",
        "  null|read userOrGroup /remoteZone",
        "Use null to remove ACLs and read access for another user or group.",
        " ",
        "For example, to allow all users to see the remote zones via 'ils /':",
        "iadmin modzonecollacl read public /r3",
        " ",
        "To remove it:",
        "iadmin modzonecollacl null public /r3",
        " ",
        "Access below this level is controlled at the remote zone.",
        ""
    };


    char *rmzoneMsgs[] = {
        " rmzone Name (remove zone)",
        "Remove a zone definition.",
        "Only remote zones can be removed.",
        ""
    };

    char *mkgroupMsgs[] = {
        " mkgroup Name (make group)",
        "Create a user group.",
        "Also see atg, rfg, and rmgroup.",
        ""
    };

    char *rmgroupMsgs[] = {
        " rmgroup Name (remove group)",
        "Remove a user group.",
        "Also see mkgroup, atg, and rfg.",
        ""
    };

    char *atgMsgs[] = {
        " atg groupName userName[#userZone] (add to group - add a user to a group)",
        "For remote-zone users, include the userZone.",
        "Also see mkgroup, rfg and rmgroup.",
        "In addition to the 'rodsadmin', users of type 'groupadmin' can atg and rfg",
        "for groups they are members of.  They can see group membership via iuserinfo.",
        ""
    };

    char *rfgMsgs[] = {
        " rfg groupName userName[#userZone] (remove from group - remove a user from a group)",
        "For remote-zone users, include the userZone.",
        "Also see mkgroup, afg and rmgroup.",
        "In addition to the 'rodsadmin', users of type 'groupadmin' can atg and rfg",
        "for groups they are members of.  They can see group membership via iuserinfo.",
        ""
    };

    char *atMsgs[] = {
        " at tokenNamespace Name [Value1] [Value2] [Value3] [comment] (add token) ",
        "Add a new token.  The most common use of this is to add",
        "data_type or user_type tokens.  See lt to display currently defined tokens.",
        ""
    };

    char *rtMsgs[] = {
        " rt tokenNamespace Name [Value] (remove token) ",
        "Remove a token.  The most common use of this is to remove",
        "data_type or user_type tokens.  See lt to display currently defined tokens.",
        ""
    };

    char *suqMsgs[] = {
        " suq User Target Value (set user quota)",
        " ",
        "Setting user quotas greater than zero is no longer supported in iRODS.",
        " ",
        "The User parameter is the name of the iRODS user.",
        " ",
        "The Target parameter can be either the name of a particular resource",
        "or the string 'total' to represent a total quota for this user across",
        "all resources.",
        " ",
        "The Value parameter is the quota amount in bytes.",
        "Set this to 0 to remove a user quota.  This is the only valid value.",
        " ",
        "Also see sgq, lq, and cu.",
        ""
    };

    char *sgqMsgs[] = {
        " sgq Group Target Value (set group quota)",
        " ",
        "Set a storage quota for a group.",
        " ",
        "The Group parameter is the name of the iRODS group.",
        " ",
        "The Target parameter can be either the name of a particular resource",
        "or the string 'total' to represent a total quota for this group across",
        "all resources.",
        " ",
        "The Value parameter is the quota amount in bytes.",
        "Set this to 0 to remove a group quota.",
        " ",
        "Also see suq, lq, and cu.",
        ""
    };

    char *lqMsgs[] = {
        " lq [Name] List Quotas",
        "List the quotas that have been set (if any).",
        "If Name is provided, list only that user or group.",
        "Also see suq, sgq, cu, and the 'iquota' command.",
        ""
    };

    char *cuMsgs[] = {
        " cu (calculate usage (for quotas))",
        "Calculate the usage on resources for each user and group and",
        "determine if users are over quota.",
        "Also see suq, sgq, and lq.",
        ""
    };

    char *rumMsgs[] = {
        " rum (remove unused metadata (user-defined AVUs)",
        "When users remove user-defined metadata (Attribute-Value-Unit triples",
        "(AVUs)) on objects (collections, data-objects, etc), or remove the",
        "objects themselves, the associations between those objects and the",
        "AVUs are removed but the actual AVUs (rows in another table) are left",
        "in place.  This is because each AVU can be associated with multiple",
        "objects.  But this only needs to be run if the number of unused AVUs has",
        "gotten large and is slowing down the DBMS.  This command runs SQL",
        "to remove those unused AVU rows.  This is a slower command on MySQL",
        " than on PostgreSQL and Oracle.",
        ""
    };

    char *asqMsgs[] = {
        " asq 'SQL query' [Alias] (add specific query)",
        "Add a specific query to the list of those allowed.",
        "Care must be taken when defining these to prevent users from accessing",
        "or updating information (in the iCAT tables) that needs to be restricted",
        "(passwords, for example) as the normal general-query access controls are",
        "bypassed via this.  This also requires an understanding of the iCAT schema",
        "(see icatSysTables.sql) to properly link tables in your SQL.",
        "If an Alias is provided, clients can use that instead of the full SQL",
        "string to select the SQL.  Aliases are checked to be sure they are unique",
        "but the same SQL can have multiple aliases.",
        "These can be executed via 'iquest --sql'.",
        "Use 'iquest --sql ls' to see the currently defined list.",
        "If 'iquest --sql ls' fails, see icatSysInserts.sql for the definitions of two",
        "built-in specific queries (r_specific_query) that are needed.",
        "To add a specific query with single quotes(') within, use double",
        "quotes(\") around the SQL.",
        "Also see rsq.",
        ""
    };

    char *rsqMsgs[] = {
        " rsq 'SQL query' or Alias (remove specific query)",
        "Remove a specific query from the list of those allowed.",
        "Use 'iquest --sql ls' to see the currently defined list.",
        "Also see asq.",
        ""
    };

    char* modrepl_usage[] = {
        "Usage: modrepl logical_path STRING replica_number INTEGER ATTR_NAME VALUE",
        "Usage: modrepl logical_path STRING resource_hierarchy STRING ATTR_NAME VALUE",
        "Usage: modrepl data_id INTEGER replica_number INTEGER ATTR_NAME VALUE",
        "Usage: modrepl data_id INTEGER resource_hierarchy STRING ATTR_NAME VALUE",
        " ",
        "Change some attribute of a replica, i.e. a row in R_DATA_MAIN.",
        " ",
        "The data object to modify must be specified. There are 2 options for doing so:",
        "    1. logical_path - A STRING holding the absolute path of the target data object",
        "    2. data_id - An INTEGER matching the DATA_ID of the target data object",
        " ",
        "The replica to modify must be specified. There are 2 options for doing so:",
        "    1. replica_number - An INTEGER representing the replica number",
        "    2. resource_hierarchy - A STRING matching the full resource hierarchy hosting",
        "       the target replica",
        " ",
        "ATTR_NAME is the GenQuery attribute to be modified with VALUE.",
        "The following attributes are accepted for modification:",
        "   DATA_CREATE_TIME",
        "   DATA_CHECKSUM",
        "   DATA_EXPIRY",
        "   DATA_REPL_STATUS",
        "   DATA_MODE",
        "   DATA_OWNER_NAME",
        "   DATA_OWNER_ZONE",
        "   DATA_PATH",
        "   DATA_REPL_NUM",
        "   DATA_SIZE",
        "   DATA_STATUS",
        "   DATA_TYPE_NAME",
        "   DATA_VERSION",
        "   DATA_ACCESS_TIME",
        "   DATA_MODIFY_TIME",
        "   DATA_COMMENTS",
        "   DATA_RESC_ID",
        ""
    };

    char* set_grid_configuration_usage[] = {
        "set_grid_configuration NAMESPACE OPTION_NAME OPTION_VALUE",
        " ",
        "Set the specified OPTION_NAME to OPTION_VALUE for the specified",
        "NAMESPACE in the local zone in R_GRID_CONFIGURATION. If NAMESPACE or",
        "OPTION_NAME is not valid, an error is returned.",
        " ",
        "The grid configuration NAMESPACEs and their respective OPTION_NAMEs for",
        "set_grid_configuration are currently:",
        " ",
        "NAMESPACE        OPTION_NAME",
        "authentication   password_extend_lifetime",
        "authentication   password_max_time",
        "authentication   password_min_time",
        ""};

    char* get_grid_configuration_usage[] = {
        "get_grid_configuration NAMESPACE OPTION_NAME",
        " ",
        "Prints local zone grid configuration value for specified NAMESPACE and",
        "OPTION_NAME.",
        " ",
        "This command allows administrators to view grid configuration values",
        "without running queries directly against the database. If NAMESPACE or",
        "OPTION_NAME is not valid, an error is returned.",
        " ",
        "This information is retrieved from the R_GRID_CONFIGURATION database",
        "table.",
        " ",
        "The grid configuration NAMESPACEs and their respective OPTION_NAMEs for",
        "get_grid_configuration are currently:",
        " ",
        "NAMESPACE        OPTION_NAME",
        "authentication   password_extend_lifetime",
        "authentication   password_max_time",
        "authentication   password_min_time",
        "database         schema_version",
        "delay_server     leader",
        "delay_server     successor",
        " ",
        ""};

    char* get_delay_server_info_usage[] = {
        "get_delay_server_info",
        " ",
        "Prints information about the delay server as JSON.",
        " ",
        "This command allows administrators to identify which server is running the",
        "delay server and if the delay server is being migrated.",
        " ",
        "This information is retrieved from the R_GRID_CONFIGURATION database table.",
        " ",
        R"_(Example Output:

    {
        "leader": "consumer-1.irods.org",
        "successor": ""
    })_",
        ""
    };

    char* set_delay_server_usage[] = {
        "set_delay_server HOSTNAME",
        " ",
        "Set the delay server for the local zone in R_GRID_CONFIGURATION.",
        " ",
        "The hostname entered will be saved as the 'successor'.",
        " ",
        "Each iRODS server will periodically check the catalog to determine",
        "if it should promote itself to be the delay server for the local zone.",
        " ",
        "This mechanism allows for graceful delay server migration without downtime.",
        ""
    };

    char *helpMsgs[] = {
        " help (or h) [command] (general help, or more details on a command)",
        " If you specify a command, a brief description of that command",
        " will be displayed.",
        ""
    };

    char* subCmds[] = {"lu",
                       "lua",
                       "luan",
                       "luz",
                       "lt",
                       "lr",
                       "ls",
                       "lz",
                       "lg",
                       "lgd",
                       "mkuser",
                       "moduser",
                       "aua",
                       "rua",
                       "rpp",
                       "rmuser",
                       "mkresc",
                       "modresc",
                       "modrescdatapaths",
                       "rmresc",
                       "addchildtoresc",
                       "rmchildfromresc",
                       "mkzone",
                       "modzone",
                       "modzonecollacl",
                       "rmzone",
                       "mkgroup",
                       "rmgroup",
                       "atg",
                       "rfg",
                       "at",
                       "rt",
                       "spass",
                       "dspass",
                       "suq",
                       "sgq",
                       "lq",
                       "cu",
                       "rum",
                       "asq",
                       "rsq",
                       "modrepl",
                       "get_delay_server_info",
                       "set_delay_server",
                       "get_grid_configuration",
                       "set_grid_configuration",
                       "help",
                       "h"};

    char** pMsgs[] = {luMsgs,
                      luaMsgs,
                      luanMsgs,
                      luzMsgs,
                      ltMsgs,
                      lrMsgs,
                      lzMsgs,
                      lgMsgs,
                      lgdMsgs,
                      mkuserMsgs,
                      moduserMsgs,
                      auaMsgs,
                      ruaMsgs,
                      rppMsgs,
                      rmuserMsgs,
                      mkrescMsgs,
                      modrescMsgs,
                      modrescDataPathsMsgs,
                      rmrescMsgs,
                      addchildtorescMsgs,
                      rmchildfromrescMsgs,
                      mkzoneMsgs,
                      modzoneMsgs,
                      modzonecollaclMsgs,
                      rmzoneMsgs,
                      mkgroupMsgs,
                      rmgroupMsgs,
                      atgMsgs,
                      rfgMsgs,
                      atMsgs,
                      rtMsgs,
                      spassMsgs,
                      dspassMsgs,
                      suqMsgs,
                      sgqMsgs,
                      lqMsgs,
                      cuMsgs,
                      rumMsgs,
                      asqMsgs,
                      rsqMsgs,
                      modrepl_usage,
                      get_delay_server_info_usage,
                      set_delay_server_usage,
                      get_grid_configuration_usage,
                      set_grid_configuration_usage,
                      helpMsgs,
                      helpMsgs};

    if ( *subOpt == '\0' ) {
        usageMain();
    }
    else {
        for ( size_t i = 0; i < sizeof( subCmds ) / sizeof( subCmds[0] ); ++i ) {
            if ( strcmp( subOpt, subCmds[i] ) == 0 ) {
                printMsgs( pMsgs[i] );
            }
        }
    }
}
