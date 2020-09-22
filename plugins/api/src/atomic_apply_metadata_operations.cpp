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
#include "miscServerFunct.hpp"
#include "catalog.hpp"
#include "catalog_utilities.hpp"

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
    namespace ic    = irods::experimental::catalog;

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

    auto get_object_id(rsComm_t& _comm, const std::string& _entity_name, const ic::entity_type _entity_type) -> int;

    auto get_meta_id(nanodbc::connection& _db_conn, const fs::metadata& _metadata) -> int;

    auto is_metadata_attached_to_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> bool;

    auto insert_metadata(nanodbc::connection& _db_conn, std::string_view _db_instance_name, const fs::metadata& _metadata) -> int;

    auto attach_metadata_to_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> void;

    auto detach_metadata_from_object(nanodbc::connection& _db_conn, int _object_id, int _meta_id) -> void;

    auto execute_metadata_operation(nanodbc::connection& _db_conn,
                                    std::string_view _db_instance_name,
                                    int _object_id,
                                    const json& _operation,
                                    int _op_index) -> std::tuple<int, bytesBuf_t*>;

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

    auto get_object_id(rsComm_t& _comm, const std::string& _entity_name, const ic::entity_type _entity_type) -> int
    {
        std::string gql;
        switch (_entity_type) {
            case ic::entity_type::collection:
                gql = fmt::format("select COLL_ID where COLL_NAME = '{}'", _entity_name);
                break;

            case ic::entity_type::data_object:
            {
                fs::path p = _entity_name;
                gql = fmt::format("select DATA_ID where COLL_NAME = '{}' and DATA_NAME = '{}'",
                                  p.parent_path().c_str(),
                                  p.object_name().c_str());
                break;
            }

            case ic::entity_type::user:
                gql = fmt::format("select USER_ID where USER_NAME = '{}'", _entity_name);
                break;

            case ic::entity_type::resource:
                gql = fmt::format("select RESC_ID where RESC_NAME = '{}'", _entity_name);
                break;

            default:
                throw std::runtime_error{fmt::format("Invalid entity type specified [entity_type => {}]", _entity_type)};
        }

        for (auto&& row : irods::query{&_comm, gql}) {
            return std::stoi(row[0]);
        }

        throw std::runtime_error{fmt::format("Entity does not exist [entity_name => {}]", _entity_name)};
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

    auto insert_metadata(nanodbc::connection& _db_conn, std::string_view _db_instance_name, const fs::metadata& _metadata) -> int
    {
        nanodbc::statement stmt{_db_conn};

        if (_db_instance_name == "oracle") {
            prepare(stmt, "insert into R_META_MAIN (meta_id, meta_attr_name, meta_attr_value, meta_attr_unit, create_ts, modify_ts) "
                          "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?)");
        }
        else if (_db_instance_name == "mysql") {
            prepare(stmt, "insert into R_META_MAIN (meta_id, meta_attr_name, meta_attr_value, meta_attr_unit, create_ts, modify_ts) "
                          "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?)");
        }
        else if (_db_instance_name == "postgres") {
            prepare(stmt, "insert into R_META_MAIN (meta_id, meta_attr_name, meta_attr_value, meta_attr_unit, create_ts, modify_ts) "
                          "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?)");
        }
        else {
            throw std::runtime_error{"Invalid database plugin configuration"};
        }

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
                                    std::string_view _db_instance_name,
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

            if (const auto op_code = _op.at("operation").get<std::string>(); op_code == "add") {
                if (auto meta_id = get_meta_id(_db_conn, md); meta_id > -1) {
                    if (!is_metadata_attached_to_object(_db_conn, _object_id, meta_id)) {
                        attach_metadata_to_object(_db_conn, _object_id, meta_id);
                    }
                }
                else if (meta_id = insert_metadata(_db_conn, _db_instance_name, md); meta_id > -1) {
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
                rodsLog(LOG_ERROR, "Invalid metadata operation [metadata_operation => %s]", _op.dump().c_str());
                return {INVALID_OPERATION, to_bytes_buffer(make_error_object(_op, _op_index, "Invalid metadata operation.").dump())};
            }

            return {0, to_bytes_buffer("{}")};
        }
        catch (const fs::filesystem_error& e) {
            rodsLog(LOG_ERROR, "%s [metadata_operation => %s]", e.what(), _op.dump().c_str());
            return {e.code().value(), to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const json::out_of_range& e) {
            rodsLog(LOG_ERROR, "%s [metadata_operation => %s]", e.what(), _op.dump().c_str());
            return {SYS_INTERNAL_ERR, to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const json::type_error& e) {
            rodsLog(LOG_ERROR, "%s [metadata_operation => %s]", e.what(), _op.dump().c_str());
            return {SYS_INTERNAL_ERR, to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
        catch (const std::system_error& e) {
            rodsLog(LOG_ERROR, "%s [metadata_operation => %s]", e.what(), _op.dump().c_str());
            return {e.code().value(), to_bytes_buffer(make_error_object(_op, _op_index, e.what()).dump())};
        }
    }

    auto rs_atomic_apply_metadata_operations(rsComm_t* _comm, bytesBuf_t* _input, bytesBuf_t** _output) -> int
    {
        namespace ic = irods::experimental::catalog;

        try {
            if (!ic::connected_to_catalog_provider(*_comm)) {
                irods::log(LOG_DEBUG8, "Redirecting request to catalog service provider ...");

                auto host_info = ic::redirect_to_catalog_provider(*_comm);

                std::string_view json_input(static_cast<const char*>(_input->buf), _input->len);
                char* json_output = nullptr;

                const auto ec = rc_atomic_apply_metadata_operations(host_info.conn, json_input.data(), &json_output);
                *_output = to_bytes_buffer(json_output);

                return ec;
            }

            ic::throw_if_catalog_provider_service_role_is_invalid();
        }
        catch (const irods::exception& e) {
            std::string_view msg = e.what();
            irods::log(LOG_ERROR, msg.data());
            *_output = to_bytes_buffer(make_error_object(json{}, 0, msg.data()).dump());
            return e.code();
        }

        if (const auto [valid, msg] = is_input_valid(_input); !valid) {
            rodsLog(LOG_ERROR, msg.c_str());
            *_output = to_bytes_buffer(make_error_object(json{}, 0, "Invalid input").dump());
            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        json input;

        try {
            input = json::parse(std::string(static_cast<const char*>(_input->buf), _input->len));
        }
        catch (const json::parse_error& e) {
            rodsLog(LOG_ERROR, "Failed to parse input into JSON [error message => %s]", e.what());

            const auto err_info = make_error_object(json{}, 0, e.what());
            *_output = to_bytes_buffer(err_info.dump());

            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        std::string entity_name;
        ic::entity_type entity_type;
        int object_id = -1;

        try {
            entity_name = input.at("entity_name").get<std::string>();
            entity_type = ic::entity_type_map.at(input.at("entity_type").get<std::string>());
            object_id = get_object_id(*_comm, entity_name, entity_type);
        }
        catch (const std::exception& e) {
            *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
            return SYS_INVALID_INPUT_PARAM;
        }

        std::string db_instance_name;;
        nanodbc::connection db_conn;

        try {
            std::tie(db_instance_name, db_conn) = ic::new_database_connection();
        }
        catch (const std::exception& e) {
            *_output = to_bytes_buffer(make_error_object(json{}, 0, e.what()).dump());
            return SYS_CONFIG_FILE_ERR;
        }

        if (!ic::user_has_permission_to_modify_metadata(*_comm, db_conn, object_id, entity_type)) {
            rodsLog(LOG_ERROR, "User not allowed to modify metadata [entity_name => %s, entity_type => %s, object_id => %i",
                    entity_name.c_str(), input.at("entity_type").get<std::string>().c_str(), object_id);
            *_output = to_bytes_buffer(make_error_object(json{}, 0, "User not allowed to modify metadata").dump());
            return CAT_NO_ACCESS_PERMISSION;
        }

        return ic::execute_transaction(db_conn, [&](auto& _trans) -> int
        {
            try {
                const auto& operations = input.at("operations");

                for (json::size_type i = 0; i < operations.size(); ++i) {
                    const auto [ec, bbuf] = execute_metadata_operation(_trans.connection(),
                                                                       db_instance_name,
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
                        "api_atomic_apply_metadata_operations",     // Operation name
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

