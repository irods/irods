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

#include "atomic_apply_metadata_operations.h"

#include "rodsConnect.h"
#include "objDesc.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_api_call.hpp"
#include "irods_re_serialization.hpp"
#include "irods_get_l1desc.hpp"
#include "irods_rs_comm_query.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_query.hpp"
#include "irods_logger.hpp"
#include "miscServerFunct.hpp"

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

    auto call_atomic_apply_metadata_operations(irods::api_entry*, rsComm_t*, bytesBuf_t*, bytesBuf_t**) -> int;
    auto is_input_valid(const bytesBuf_t*) -> std::tuple<bool, std::string>;
    auto to_bytes_buffer(const std::string& _s) -> bytesBuf_t*;
    auto get_file_descriptor(const bytesBuf_t& _buf) -> int;
    auto make_error_object(const json& _op, int _op_index, const std::string& _error_msg) -> json;
    auto get_object_id(rsComm_t& _comm, const std::string& _entity_name, const std::string& _entity_type) -> int;
    auto new_database_connection() -> nanodbc::connection;
    auto user_has_permission_to_modify_metadata(rsComm_t& _comm, nanodbc::connection& _db_conn, int _object_id, const std::string& _entity_type) -> bool;
    auto execute_transaction(nanodbc::connection& _db_conn, std::function<int(nanodbc::transaction&)> _func) -> int;
    auto get_meta_id(nanodbc::connection& _db_conn, const fs::metadata& _metadata) -> int;
    auto is_metadata_attached_to_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> bool;
    auto insert_metadata(nanodbc::connection& _db_conn, const fs::metadata& _metadata) -> int;
    auto attach_metadata_to_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> void;
    auto detach_metadata_from_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> void;
    auto execute_metadata_operation(nanodbc::connection& _db_conn, int _object_id, const json& _operation, int _op_index) -> std::tuple<int, bytesBuf_t*>;
    auto rs_atomic_apply_metadata_operations(rsComm_t*, bytesBuf_t*, bytesBuf_t**) -> int;

    //
    // Function Implementations
    //

    auto call_atomic_apply_metadata_operations(irods::api_entry* _api,
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

    auto get_object_id(rsComm_t& _comm, const std::string& _entity_name, const std::string& _entity_type) -> int
    {
        std::string gql;

        if (_entity_type == "collection") {
            gql = fmt::format("select COLL_ID where COLL_NAME = '{}'", _entity_name);
        }
        else if (_entity_type == "data_object") {
            fs::path p = _entity_name;
            gql = fmt::format("select DATA_ID where COLL_NAME = '{}' and DATA_NAME = '{}'",
                              p.parent_path().c_str(),
                              p.object_name().c_str());
        }
        else if (_entity_type == "user") {
            gql = fmt::format("select USER_ID where USER_NAME = '{}'", _entity_name);
        }
        else if (_entity_type == "resource") {
            gql = fmt::format("select RESC_ID where RESC_NAME = '{}'", _entity_name);
        }
        else {
            throw std::runtime_error{fmt::format("Invalid entity type specified [entity_type => {}]", _entity_type)};
        }

        for (auto&& row : irods::query{&_comm, gql}) {
            return std::stoi(row[0]);
        }

        throw std::runtime_error{fmt::format("Entity does not exist [entity_name => {}]", _entity_name)};
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
            throw std::runtime_error{"Failed to connect to catalog"};
        }

        log::api::trace("Reading server configuration ...");

        json config;

        {
            std::ifstream config_file{config_path};
            config_file >> config;
        }

        try {
            const auto& db_plugin_config = config.at(irods::CFG_PLUGIN_CONFIGURATION_KW).at(irods::PLUGIN_TYPE_DATABASE);
            const auto& db_instance = *std::begin(db_plugin_config);
            const auto db_username = db_instance.at(irods::CFG_DB_USERNAME_KW).get<std::string>();
            const auto db_password = db_instance.at(irods::CFG_DB_PASSWORD_KW).get<std::string>();

            return {dsn, db_username, db_password};
        }
        catch (const std::exception& e) {
            log::api::error(e.what());
            throw std::runtime_error{"Failed to connect to catalog"};
        }
    }

    auto user_has_permission_to_modify_metadata(rsComm_t& _comm,
                                                nanodbc::connection& _db_conn,
                                                int _object_id,
                                                const std::string& _entity_type) -> bool
    {
        if (_entity_type == "data_object" || _entity_type == "collection") {
            const auto query = fmt::format("select t.token_id from R_TOKN_MAIN t"
                                           " inner join R_OBJT_ACCESS a on t.token_id = a.access_type_id "
                                           "where"
                                           " a.user_id = (select user_id from R_USER_MAIN where user_name = '{}') and"
                                           " a.object_id = '{}'", _comm.clientUser.userName, _object_id);

            if (auto row = execute(_db_conn, query); row.next()) {
                constexpr int access_modify_object = 1120;
                return row.get<int>(0) >= access_modify_object;
            }
        }
        else if (_entity_type == "user" || _entity_type == "resource") {
            return irods::is_privileged_client(_comm);
        }
        else {
            log::api::error("Invalid entity type [entity_type => {}]", _entity_type);
        }

        return false;
    }

    auto execute_transaction(nanodbc::connection& _db_conn, std::function<int(nanodbc::transaction&)> _func) -> int
    {
        nanodbc::transaction trans{_db_conn};
        return _func(trans);
    }

    auto get_meta_id(nanodbc::connection& _db_conn, const fs::metadata& _metadata) -> int
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "select meta_id from R_META_MAIN "
                      "where"
                      " meta_attr_name = ? and"
                      " meta_attr_value = ? and"
                      " meta_attr_unit = ?");

        stmt.bind(0, _metadata.attribute.c_str());
        stmt.bind(1, _metadata.value.c_str());
        stmt.bind(2, _metadata.units.c_str());

        if (auto row = execute(stmt); row.next()) {
            return row.get<int>(0);
        }

        return -1;
    }

    auto is_metadata_attached_to_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> bool
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "select count(*) from R_OBJT_METAMAP where object_id = ? and meta_id = ?");

        stmt.bind(0, &_object_id);
        stmt.bind(1, &_meta_id);

        if (auto row = execute(stmt); row.next()) {
            return row.get<int>(0) > 0;
        }

        return false;
    }

    auto insert_metadata(nanodbc::connection& _db_conn, const fs::metadata& _metadata) -> int
    {
        nanodbc::statement stmt{_db_conn};

#ifdef ORA_ICAT
        prepare(stmt, "insert into R_META_MAIN (meta_id, meta_attr_name, meta_attr_value, meta_attr_unit, create_ts, modify_ts) "
                      "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?)");
#else
        prepare(stmt, "insert into R_META_MAIN (meta_id, meta_attr_name, meta_attr_value, meta_attr_unit, create_ts, modify_ts) "
                      "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?)");
#endif // ORA_ICAT

        using std::chrono::system_clock;
        using std::chrono::duration_cast;
        using std::chrono::seconds;

        const auto timestamp = fmt::format("{:011}", duration_cast<seconds>(system_clock::now().time_since_epoch()).count());

        stmt.bind(0, _metadata.attribute.c_str());
        stmt.bind(1, _metadata.value.c_str());
        stmt.bind(2, _metadata.units.c_str());
        stmt.bind(3, timestamp.c_str());
        stmt.bind(4, timestamp.c_str());

        execute(stmt);

        return get_meta_id(_db_conn, _metadata);
    }

    auto attach_metadata_to_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> void
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) "
                      "values (?, ?, ?, ?)");

        using std::chrono::system_clock;
        using std::chrono::duration_cast;
        using std::chrono::seconds;

        const auto timestamp = fmt::format("{:011}", duration_cast<seconds>(system_clock::now().time_since_epoch()).count());

        stmt.bind(0, &_object_id);
        stmt.bind(1, &_meta_id);
        stmt.bind(2, timestamp.c_str());
        stmt.bind(3, timestamp.c_str());

        execute(stmt);
    }

    auto detach_metadata_from_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> void
    {
        nanodbc::statement stmt{_db_conn};

        prepare(stmt, "delete from R_OBJT_METAMAP where object_id = ? and meta_id = ?");

        stmt.bind(0, &_object_id);
        stmt.bind(1, &_meta_id);

        execute(stmt);
    }

    auto execute_metadata_operation(nanodbc::connection& _db_conn,
                                    int _object_id,
                                    const json& _op,
                                    int _op_index) -> std::tuple<int, bytesBuf_t*>
    {
        try {
            fs::metadata md;

            md.attribute = _op.at("attribute").get<std::string>();
            md.value = _op.at("value").get<std::string>();

            // "units" are optional.
            if (_op.count("units")) {
                md.units = _op.at("units").get<std::string>();
            }

            if (const auto op_code = _op.at("operation").get<std::string>(); op_code == "set") {
                if (auto meta_id = get_meta_id(_db_conn, md); meta_id > -1) {
                    if (!is_metadata_attached_to_object(_db_conn, _object_id, meta_id)) {
                        attach_metadata_to_object(_db_conn, _object_id, meta_id);
                    }
                }
                else if (meta_id = insert_metadata(_db_conn, md); meta_id > -1) {
                    attach_metadata_to_object(_db_conn, _object_id, meta_id);
                }
                else {
                    const auto msg = fmt::format("Failed to insert metadata [attribute => {}, value => {}, units => {}]",
                                                 md.attribute, md.value, md.units);
                    return {SYS_INTERNAL_ERR, to_bytes_buffer(make_error_object(_op, _op_index, msg).dump())};
                }
            }
            else if (op_code == "remove") {
                if (const auto meta_id = get_meta_id(_db_conn, md); meta_id > -1) {
                    detach_metadata_from_object(_db_conn, _object_id, meta_id);
                }
            }
            else {
                // clang-format off
                log::api::error({{"log_message", "Invalid metadata operation"},
                                 {"metadata_operation", _op.dump()}});
                // clang-format on

                return {INVALID_OPERATION, to_bytes_buffer(make_error_object(_op, _op_index, "Invalid metadata operation.").dump())};
            }

            return {0, to_bytes_buffer("{}")};
        }
        catch (const fs::filesystem_error& e) {
            // clang-format off
            log::api::error({{"log_message", e.what()},
                             {"metadata_operation", _op.dump()}});
            // clang-format on

            return {e.code().value(), to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const json::out_of_range& e) {
            // clang-format off
            log::api::error({{"log_message", e.what()},
                             {"metadata_operation", _op.dump()}});
            // clang-format on

            return {SYS_INTERNAL_ERR, to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const json::type_error& e) {
            // clang-format off
            log::api::error({{"log_message", e.what()},
                             {"metadata_operation", _op.dump()}});
            // clang-format on

            return {SYS_INTERNAL_ERR, to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const std::system_error& e) {
            // clang-format off
            log::api::error({{"log_message", e.what()},
                             {"metadata_operation", _op.dump()}});
            // clang-format on

            return {e.code().value(), to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
    }

    auto rs_atomic_apply_metadata_operations(rsComm_t* _comm, bytesBuf_t* _input, bytesBuf_t** _output) -> int
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
                const auto msg = fmt::format("Role not supported [role => {}]", svc_role);
                log::api::error(msg);
                *_output = to_bytes_buffer(make_error_object(json{}, 0, msg).dump());
                return SYS_SERVICE_ROLE_NOT_SUPPORTED;
            }
        }
        else {
            std::string_view json_input(static_cast<const char*>(_input->buf), _input->len);
            char* json_output = nullptr;

            const auto ec = rc_atomic_apply_metadata_operations(rodsServerHost->conn, json_input.data(), &json_output);
            *_output = to_bytes_buffer(json_output);

            return ec;
        }

        if (const auto [valid, msg] = is_input_valid(_input); !valid) {
            log::api::error(msg);
            *_output = to_bytes_buffer(make_error_object(json{}, 0, "Invalid input").dump());
            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        json input;

        try {
            input = json::parse(std::string(static_cast<const char*>(_input->buf), _input->len));
        }
        catch (const json::parse_error& e) {
            // clang-format off
            log::api::error({{"log_message", "Failed to parse input into JSON"},
                             {"error_message", e.what()}});
            // clang-format on

            const auto err_info = make_error_object(json{}, 0, e.what());
            *_output = to_bytes_buffer(err_info.dump());

            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        std::string entity_name;
        std::string entity_type;
        int object_id = -1;

        try {
            entity_name = input.at("entity_name").get<std::string>();
            entity_type = input.at("entity_type").get<std::string>();
            object_id = get_object_id(*_comm, entity_name, entity_type);
        }
        catch (const std::exception& e) {
            *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
            return SYS_INVALID_INPUT_PARAM;
        }

        nanodbc::connection db_conn;

        try {
            db_conn = new_database_connection();
        }
        catch (const std::exception& e) {
            *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
            return SYS_CONFIG_FILE_ERR;
        }

        if (!user_has_permission_to_modify_metadata(*_comm, db_conn, object_id, entity_type)) {
            log::api::error("User not allowed to modify metadata [entity_name => {}, entity_type => {}, object_id => {}]",
                            entity_name, entity_type, object_id);
            *_output = to_bytes_buffer(make_error_object(json{}, 0, "User not allowed to modify metadata").dump());
            return CAT_NO_ACCESS_PERMISSION;
        }

        return execute_transaction(db_conn, [&](auto& _trans) -> int
        {
            try {
                const auto& operations = input.at("operations");

                for (json::size_type i = 0; i < operations.size(); ++i) {
                    if (const auto [ec, bbuf] = execute_metadata_operation(_trans.connection(), object_id, operations[i], i); ec != 0) {
                        *_output = bbuf;
                        return ec;
                    }
                }

                _trans.commit();

                *_output = to_bytes_buffer("{}");

                return 0;
            }
            catch (const json::type_error& e) {
                *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
                return SYS_INTERNAL_ERR;
            }
        });
    }

    const operation op = rs_atomic_apply_metadata_operations;
    #define CALL_ATOMIC_APPLY_METADATA_OPERATIONS call_atomic_apply_metadata_operations
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    const operation op{};
    #define CALL_ATOMIC_APPLY_METADATA_OPERATIONS nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_whitelist::instance().add(ATOMIC_APPLY_METADATA_OPERATIONS_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{ATOMIC_APPLY_METADATA_OPERATIONS_APN,       // API number
                        RODS_API_VERSION,                           // API version
                        NO_USER_AUTH,                               // Client auth
                        NO_USER_AUTH,                               // Proxy auth
                        "BytesBuf_PI", 0,                           // In PI / bs flag
                        "BytesBuf_PI", 0,                           // Out PI / bs flag
                        op,                                         // Operation
                        "atomic_apply_metadata_operations",         // Operation name
                        nullptr,                                    // Null clear function
                        (funcPtr) CALL_ATOMIC_APPLY_METADATA_OPERATIONS};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    api->out_pack_key = "BytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
}

