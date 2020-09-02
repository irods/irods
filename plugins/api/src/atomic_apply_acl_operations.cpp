#include "api_plugin_number.h"
#include "irods_configuration_keywords.hpp"
#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsErrorTable.h"
#include "rodsPackInstruct.h"
#include "client_api_whitelist.hpp"

#include "apiHandler.hpp"

#include <functional>
#include <stdexcept>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "atomic_apply_acl_operations.h"

#include "rodsConnect.h"
#include "objDesc.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_api_call.hpp"
#include "irods_re_serialization.hpp"
#include "irods_rs_comm_query.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_logger.hpp"
#include "miscServerFunct.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods_query.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#include "json.hpp"
#include "fmt/format.h"
#include "nanodbc/nanodbc.h"

#include <cstdlib>
#include <string>
#include <string_view>
#include <tuple>
#include <chrono>
#include <system_error>

namespace
{
    // clang-format off
    namespace fs    = irods::experimental::filesystem;

    using log       = irods::experimental::log;
    using json      = nlohmann::json;
    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    // clang-format on

    //
    // Function Prototypes
    //

    auto call_atomic_apply_acl_operations(irods::api_entry*, rsComm_t*, bytesBuf_t*, bytesBuf_t**) -> int;

    auto is_input_valid(const bytesBuf_t*) -> std::tuple<bool, std::string>;

    auto to_bytes_buffer(const std::string& _s) -> bytesBuf_t*;

    auto get_file_descriptor(const bytesBuf_t& _buf) -> int;

    auto make_error_object(const json& _op, int _op_index, const std::string& _error_msg) -> json;

    auto get_object_id(rsComm_t& _comm, std::string_view _logical_path) -> int;

    auto user_has_permission_to_modify_acls(rsComm_t& _comm,
                                            nanodbc::connection& _db_conn,
                                            int _object_id) -> bool;

    auto throw_if_invalid_acl(std::string_view _acl) -> void;

    auto throw_if_invalid_entity_id(int _entity_id) -> void;

    auto to_access_type_id(std::string_view _acl) -> int;

    auto new_database_connection() -> nanodbc::connection;

    auto execute_transaction(nanodbc::connection& _db_conn, std::function<int(nanodbc::transaction&)> _func) -> int;

    auto get_entity_id(nanodbc::connection& _db_conn, std::string_view _entity_name) -> int;

    auto entity_has_acls_set_on_object(nanodbc::connection& _db_conn, int _object_id, int _entity_id) -> bool;

    auto insert_acl(nanodbc::connection& _db_conn, int _object_id, int _entity_id, std::string_view acl) -> void;

    auto update_acl(nanodbc::connection& _db_conn, int _object_id, int _entity_id, std::string_view new_acl) -> void;

    auto remove_acl(nanodbc::connection& _db_conn, int _object_id, int _entity_id) -> void;

    auto execute_acl_operation(nanodbc::connection& _db_conn,
                               int _object_id,
                               const json& _operation,
                               int _op_index) -> std::tuple<int, bytesBuf_t*>;

    auto rs_atomic_apply_acl_operations(rsComm_t*, bytesBuf_t*, bytesBuf_t**) -> int;

    //
    // Function Implementations
    //

    auto call_atomic_apply_acl_operations(irods::api_entry* _api,
                                          rsComm_t* _comm,
                                          bytesBuf_t* _input,
                                          bytesBuf_t** _output) -> int
    {
        return _api->call_handler<bytesBuf_t*, bytesBuf_t**>(_comm, _input, _output);
    }

    auto is_input_valid(const bytesBuf_t* _input) -> std::tuple<bool, std::string>
    {
        if (!_input) {
            return {false, "Missing JSON input"};
        }

        if (_input->len <= 0) {
            return {false, "Length of buffer must be greater than zero"};
        }

        if (!_input->buf) {
            return {false, "Missing input buffer"};
        }

        return {true, ""};
    }

    auto to_bytes_buffer(const std::string& _s) -> bytesBuf_t*
    {
        constexpr auto allocate = [](const auto bytes) noexcept
        {
            return std::memset(std::malloc(bytes), 0, bytes);
        };

        const auto buf_size = _s.length() + 1;

        auto* buf = static_cast<char*>(allocate(sizeof(char) * buf_size));
        std::strncpy(buf, _s.c_str(), _s.length());

        auto* bbp = static_cast<bytesBuf_t*>(allocate(sizeof(bytesBuf_t)));
        bbp->len = buf_size;
        bbp->buf = buf;

        return bbp;
    }

    auto make_error_object(const json& _op, int _op_index, const std::string& _error_msg) -> json
    {
        return json{
            {"operation", _op},
            {"operation_index", _op_index},
            {"error_message", _error_msg}
        };
    }

    auto get_object_id(rsComm_t& _comm, std::string_view _logical_path) -> int
    {
        fs::path p = _logical_path.data();
        std::string gql;

        const auto s = fs::server::status(_comm, p);

        if (!fs::server::exists(s)) {
            log::api::error("Object does not exist at the provided logical path [path={}]", _logical_path);
            return OBJ_PATH_DOES_NOT_EXIST;
        }
        
        if (fs::server::is_collection(s)) {
            gql = fmt::format("select COLL_ID where COLL_NAME = '{}'", _logical_path);
        }
        else if (fs::server::is_data_object(s)) {
            fs::path p = _logical_path.data();
            gql = fmt::format("select DATA_ID where COLL_NAME = '{}' and DATA_NAME = '{}'",
                              p.parent_path().c_str(),
                              p.object_name().c_str());
        }
        else {
            log::api::error("Object is not a data object or collection [path={}]", _logical_path);
            return CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION;
        }

        for (auto&& row : irods::query{&_comm, gql}) {
            return std::stoi(row[0]);
        }

        log::api::error("Failed to resolve path to an ID [path={}]", _logical_path);

        return SYS_UNKNOWN_ERROR;
    }

    auto throw_if_invalid_acl(std::string_view _acl) -> void
    {
        const auto l = {"read", "write", "own", "null"};

        if (std::none_of(std::begin(l), std::end(l), [_acl](std::string_view x) { return x == _acl; })) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid ACL [acl={}]", _acl));
        }
    }

    auto throw_if_invalid_entity_id(int _entity_id) -> void
    {
        if (-1 == _entity_id) {
            THROW(SYS_INVALID_INPUT_PARAM, "Invalid entity");
        }
    }

    auto to_access_type_id(std::string_view _acl) -> int
    {
        // clang-format off
        if (_acl == "read")  { return 1050; }
        if (_acl == "write") { return 1120; }
        if (_acl == "own")   { return 1200; }
        if (_acl == "null")  { return 1000; }
        // clang-format on

        THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid ACL [acl={}]", _acl));
    }

    auto new_database_connection() -> nanodbc::connection
    {
        const std::string dsn = [] {
            if (const char* dsn = std::getenv("irodsOdbcDSN"); dsn) {
                return dsn;
            }

            return "iRODS Catalog";
        }();

        std::string config_path;

        if (const auto error = irods::get_full_path_for_config_file("server_config.json", config_path); !error.ok()) {
            log::api::error("Server configuration not found");
            THROW(CAT_CONNECT_ERR, "Failed to connect to catalog");
        }

        log::api::trace("Reading server configuration ...");

        json config;

        {
            std::ifstream config_file{config_path};
            config_file >> config;
        }

        try {
            const auto& db_plugin_config = config.at(irods::CFG_PLUGIN_CONFIGURATION_KW).at(irods::PLUGIN_TYPE_DATABASE);
            const auto& db_instance = db_plugin_config.front();
            const auto db_username = db_instance.at(irods::CFG_DB_USERNAME_KW).get<std::string>();
            const auto db_password = db_instance.at(irods::CFG_DB_PASSWORD_KW).get<std::string>();

            // Capture the database instance name.
            std::string db_instance_name;
            for (auto& [k, v] : db_plugin_config.items()) {
                db_instance_name = k;
            }

            if (db_instance_name.empty()) {
                THROW(SYS_CONFIG_FILE_ERR, "Database instance name cannot be empty");
            }

            nanodbc::connection db_conn{dsn, db_username, db_password};

            if (db_instance_name == "mysql") {
                // MySQL must be running in ANSI mode (or at least in PIPES_AS_CONCAT mode) to be
                // able to understand Postgres SQL. STRICT_TRANS_TABLES must be set too, otherwise
                // inserting NULL into a "NOT NULL" column does not produce an error.
                nanodbc::just_execute(db_conn, "set SESSION sql_mode = 'ANSI,STRICT_TRANS_TABLES'");
                nanodbc::just_execute(db_conn, "set character_set_client = utf8");
                nanodbc::just_execute(db_conn, "set character_set_results = utf8");
                nanodbc::just_execute(db_conn, "set character_set_connection = utf8");
            }

            return db_conn;
        }
        catch (const std::exception& e) {
            log::api::error(e.what());
            THROW(CAT_CONNECT_ERR, "Failed to connect to catalog");
        }
    }

    auto user_has_permission_to_modify_acls(rsComm_t& _comm,
                                            nanodbc::connection& _db_conn,
                                            int _object_id) -> bool
    {
        const auto query = fmt::format("select t.token_id from R_TOKN_MAIN t"
                                       " inner join R_OBJT_ACCESS a on t.token_id = a.access_type_id "
                                       "where"
                                       " a.user_id = (select user_id from R_USER_MAIN where user_name = '{}') and"
                                       " a.object_id = '{}'", _comm.clientUser.userName, _object_id);

        if (auto row = execute(_db_conn, query); row.next()) {
            constexpr int access_own = 1200;
            return row.get<int>(0) == access_own;
        }

        return false;
    }

    auto execute_transaction(nanodbc::connection& _db_conn, std::function<int(nanodbc::transaction&)> _func) -> int
    {
        nanodbc::transaction trans{_db_conn};
        return _func(trans);
    }

    auto entity_has_acls_set_on_object(nanodbc::connection& _db_conn, int _object_id, int _entity_id) -> bool
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "select count(*) from R_OBJT_ACCESS where object_id = ? and user_id = ?");

        stmt.bind(0, &_object_id);
        stmt.bind(1, &_entity_id);

        if (auto row = execute(stmt); row.next()) {
            return row.get<int>(0) > 0;
        }

        return false;
    }

    auto insert_acl(nanodbc::connection& _db_conn,
                    int _object_id,
                    int _entity_id,
                    std::string_view _new_acl) -> void
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "insert into R_OBJT_ACCESS (object_id, user_id, access_type_id, create_ts, modify_ts) "
                      "values (?, ?, ?, ?, ?)");

        using std::chrono::system_clock;
        using std::chrono::duration_cast;
        using std::chrono::seconds;

        const auto timestamp = fmt::format("{:011}", duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
        const auto access_type_id = to_access_type_id(_new_acl);

        stmt.bind(0, &_object_id);
        stmt.bind(1, &_entity_id);
        stmt.bind(2, &access_type_id);
        stmt.bind(3, timestamp.c_str());
        stmt.bind(4, timestamp.c_str());

        execute(stmt);
    }

    auto update_acl(nanodbc::connection& _db_conn,
                    int _object_id,
                    int _entity_id,
                    std::string_view _new_acl) -> void
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "update R_OBJT_ACCESS set access_type_id = ?, modify_ts = ? where object_id = ? and user_id = ?");

        using std::chrono::system_clock;
        using std::chrono::duration_cast;
        using std::chrono::seconds;

        const auto timestamp = fmt::format("{:011}", duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
        const auto access_type_id = to_access_type_id(_new_acl);

        stmt.bind(0, &access_type_id);
        stmt.bind(1, timestamp.c_str());
        stmt.bind(2, &_object_id);
        stmt.bind(3, &_entity_id);

        execute(stmt);
    }

    auto remove_acl(nanodbc::connection& _db_conn, int _object_id, int _entity_id) -> void
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "delete from R_OBJT_ACCESS where object_id = ? and user_id = ?");

        stmt.bind(0, &_object_id);
        stmt.bind(1, &_entity_id);

        execute(stmt);
    }

    // TODO This function should probably deal with remote zones.
    auto get_entity_id(nanodbc::connection& _db_conn, std::string_view _entity_name) -> int
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "select user_id from R_USER_MAIN where user_name = ?");

        stmt.bind(0, _entity_name.data());

        if (auto row = execute(stmt); row.next()) {
            return row.get<int>(0);
        }

        return -1;
    }

    auto execute_acl_operation(nanodbc::connection& _db_conn,
                               int _object_id,
                               const json& _op,
                               int _op_index) -> std::tuple<int, bytesBuf_t*>
    {
        try {
            log::api::trace("Checking if ACL is valid ...");
            const auto acl = _op.at("acl").get<std::string>();
            throw_if_invalid_acl(acl);

            log::api::trace("Retrieving entity ID ...");
            const auto entity_id = get_entity_id(_db_conn, _op.at("entity_name").get<std::string>());
            throw_if_invalid_entity_id(entity_id);

            if (acl == "null") {
                remove_acl(_db_conn, _object_id, entity_id);
            }
            else if (entity_has_acls_set_on_object(_db_conn, _object_id, entity_id)) {
                update_acl(_db_conn, _object_id, entity_id, acl);
            }
            else {
                insert_acl(_db_conn, _object_id, entity_id, acl);
            }

            return {0, to_bytes_buffer("{}")};
        }
        catch (const irods::exception& e) {
            log::api::error({{"log_message", e.what()}, {"acl_operation", _op.dump()}});
            return {e.code(), to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const fs::filesystem_error& e) {
            log::api::error({{"log_message", e.what()}, {"acl_operation", _op.dump()}});
            return {e.code().value(), to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const json::exception& e) {
            log::api::error({{"log_message", e.what()}, {"acl_operation", _op.dump()}});
            return {SYS_INTERNAL_ERR, to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const std::system_error& e) {
            log::api::error({{"log_message", e.what()}, {"acl_operation", _op.dump()}});
            return {e.code().value(), to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
    }

    auto rs_atomic_apply_acl_operations(rsComm_t* _comm, bytesBuf_t* _input, bytesBuf_t** _output) -> int
    {
        rodsServerHost_t *rodsServerHost;

        if (const auto ec = getAndConnRcatHost(_comm, MASTER_RCAT, nullptr, &rodsServerHost); ec < 0) {
            return ec;
        }

        if (LOCAL_HOST == rodsServerHost->localFlag) {
            std::string svc_role;

            if (const auto err = get_catalog_service_role(svc_role); !err.ok()) {
                const auto* msg = "Failed to retrieve service role";
                log::api::error(msg);
                *_output = to_bytes_buffer(make_error_object(json{}, 0, msg).dump());
                return err.code();
            }

            if (irods::CFG_SERVICE_ROLE_CONSUMER == svc_role) {
                const auto* msg = "Remote catalog provider not found";
                log::api::error(msg);
                *_output = to_bytes_buffer(make_error_object(json{}, 0, msg).dump());
                return SYS_NO_ICAT_SERVER_ERR;
            }

            if (irods::CFG_SERVICE_ROLE_PROVIDER != svc_role) {
                const auto msg = fmt::format("Role not supported [role={}]", svc_role);
                log::api::error(msg);
                *_output = to_bytes_buffer(make_error_object(json{}, 0, msg).dump());
                return SYS_SERVICE_ROLE_NOT_SUPPORTED;
            }
        }
        else {
            log::api::trace("Redirecting request to remote server ...");

            std::string_view json_input(static_cast<const char*>(_input->buf), _input->len);
            char* json_output = nullptr;

            const auto ec = rc_atomic_apply_acl_operations(rodsServerHost->conn, json_input.data(), &json_output);
            *_output = to_bytes_buffer(json_output);

            return ec;
        }

        log::api::trace("Performing basic input validation ...");

        if (const auto [valid, msg] = is_input_valid(_input); !valid) {
            log::api::error(msg);
            *_output = to_bytes_buffer(make_error_object(json{}, 0, "Invalid input").dump());
            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        json input;

        try {
            log::api::trace("Parsing string into JSON ...");
            input = json::parse(std::string(static_cast<const char*>(_input->buf), _input->len));
        }
        catch (const json::exception& e) {
            // clang-format off
            log::api::error({{"log_message", "Failed to parse input into JSON"},
                             {"error_message", e.what()}});
            // clang-format on

            const auto err_info = make_error_object(json{}, 0, e.what());
            *_output = to_bytes_buffer(err_info.dump());

            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        std::string logical_path;

        try {
            logical_path = input.at("logical_path").get<std::string>();
        }
        catch (const json::exception& e) {
            *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
            return SYS_INVALID_INPUT_PARAM;
        }

        const int object_id = get_object_id(*_comm, logical_path);

        if (object_id < 0) {
            const auto msg = fmt::format("Failed to retrieve object id [error_code={}]", object_id);
            *_output = to_bytes_buffer(make_error_object(json{}, 0, msg).dump());
            return object_id;
        }

        nanodbc::connection db_conn;

        try {
            log::api::trace("Connecting to database ...");
            db_conn = new_database_connection();
        }
        catch (const irods::exception& e) {
            *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
            return e.code();
        }
        catch (const std::exception& e) {
            *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
            return SYS_CONFIG_FILE_ERR;
        }

        log::api::trace("Checking if user has permission to modify permissions ...");

        if (!user_has_permission_to_modify_acls(*_comm, db_conn, object_id)) {
            log::api::error("User not allowed to modify ACLs [logical_path={}, object_id={}]", logical_path, object_id);
            *_output = to_bytes_buffer(make_error_object(json{}, 0, "User not allowed to modify ACLs").dump());
            return CAT_NO_ACCESS_PERMISSION;
        }

        log::api::trace("Executing ACL operations ...");

        return execute_transaction(db_conn, [&](auto& _trans) -> int
        {
            try {
                const auto& operations = input.at("operations");

                for (json::size_type i = 0; i < operations.size(); ++i) {
                    const auto [ec, bbuf] = execute_acl_operation(_trans.connection(),
                                                                  object_id,
                                                                  operations[i],
                                                                  i);
                    
                    if (ec != 0) {
                        *_output = bbuf;
                        return ec;
                    }
                }

                _trans.commit();

                *_output = to_bytes_buffer("{}");

                return 0;
            }
            catch (const json::exception& e) {
                *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
                return SYS_INTERNAL_ERR;
            }
        });
    }

    const operation op = rs_atomic_apply_acl_operations;
    #define CALL_ATOMIC_APPLY_ACL_OPERATIONS call_atomic_apply_acl_operations
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    const operation op{};
    #define CALL_ATOMIC_APPLY_ACL_OPERATIONS nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_whitelist::instance().add(ATOMIC_APPLY_ACL_OPERATIONS_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{ATOMIC_APPLY_ACL_OPERATIONS_APN,            // API number
                        RODS_API_VERSION,                           // API version
                        NO_USER_AUTH,                               // Client auth
                        NO_USER_AUTH,                               // Proxy auth
                        "BytesBuf_PI", 0,                           // In PI / bs flag
                        "BytesBuf_PI", 0,                           // Out PI / bs flag
                        op,                                         // Operation
                        "api_atomic_apply_acl_operations",          // Operation name
                        nullptr,                                    // Null clear function
                        (funcPtr) CALL_ATOMIC_APPLY_ACL_OPERATIONS};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    api->out_pack_key = "BytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
}

