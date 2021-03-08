#include "api_plugin_number.h"
#include "client_api_whitelist.hpp"
#include "fileDriver.hpp"
#include "irods_configuration_keywords.hpp"
#include "json_deserialization.hpp"
#include "rcConnect.h"
#include "rodsDef.h"
#include "rodsErrorTable.h"
#include "rodsPackInstruct.h"

#include "apiHandler.hpp"

#include <functional>
#include <stdexcept>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "data_object_finalize.h"

#include "catalog.hpp"
#include "catalog_utilities.hpp"
#include "irods_exception.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_get_l1desc.hpp"
#include "irods_logger.hpp"
#include "irods_re_serialization.hpp"
#include "irods_resource_manager.hpp"
#include "irods_rs_comm_query.hpp"
#include "irods_server_api_call.hpp"
#include "irods_stacktrace.hpp"
#include "miscServerFunct.hpp"
#include "objDesc.hpp"
#include "rodsConnect.h"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"

#include "json.hpp"
#include "fmt/format.h"
#include "nanodbc/nanodbc.h"

#include <cstdlib>
#include <string>
#include <string_view>
#include <tuple>
#include <chrono>
#include <system_error>

extern irods::resource_manager resc_mgr;

namespace
{
    // clang-format off
    namespace fs          = irods::experimental::filesystem;
    namespace ic          = irods::experimental::catalog;
    namespace replica     = irods::experimental::replica;
    namespace data_object = irods::experimental::data_object;

    using log       = irods::experimental::log;
    using json      = nlohmann::json;
    using operation = std::function<int(RsComm*, BytesBuf*, BytesBuf**)>;
    // clang-format on

    const auto& cmap = ic::data_objects::column_mapping_operators;

    auto make_error_object(const std::string& _error_msg) -> json
    {
        return json{{"error_message", _error_msg}};
    } // make_error_object

    auto to_bytes_buffer(const std::string& _s) -> BytesBuf*
    {
        constexpr auto allocate = [](const auto bytes) noexcept
        {
            return std::memset(std::malloc(bytes), 0, bytes);
        };

        const auto buf_size = _s.length() + 1;

        auto* buf = static_cast<char*>(allocate(sizeof(char) * buf_size));
        std::strncpy(buf, _s.c_str(), _s.length());

        auto* bbp = static_cast<BytesBuf*>(allocate(sizeof(BytesBuf)));
        bbp->len = buf_size;
        bbp->buf = buf;

        return bbp;
    } // to_bytes_buffer

    auto call_data_object_finalize(
        irods::api_entry* _api,
        RsComm* _comm,
        BytesBuf* _input,
        BytesBuf** _output) -> int
    {
        return _api->call_handler<BytesBuf*, BytesBuf**>(_comm, _input, _output);
    } // call_data_object_finalize

    auto validate_values(json& _obj) -> void
    {
        // clang-format off
        using clock_type    = fs::object_time_type::clock;
        using duration_type = fs::object_time_type::duration;
        // clang-format on

        if (std::string_view{SET_TIME_TO_NOW_KW} == _obj.at("modify_ts")) {
            const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());

            _obj["modify_ts"] = fmt::format("{:011}", now.time_since_epoch().count());
        }
    } // validate_values

    auto set_replica_state(
        nanodbc::connection& _db_conn,
        const json& _before,
        const json& _after) -> void
    {
        std::string sql{"update R_DATA_MAIN set"};

        for (auto&& c : cmap) {
            sql += fmt::format(" {} = ?,", c.first);
        }
        sql.pop_back();

        sql += " where data_id = ? and resc_id = ?";

        log::database::debug("statement:[{}]", sql);

        nanodbc::statement statement{_db_conn};
        prepare(statement, sql);

        log::database::debug("before:{}", _before.dump());
        log::database::debug("after:{}", _after.dump());

        // Reserve the size ahead of time to prevent pointer invalidation.
        // Need to store the bind values in a variable which will outlive
        // the bind operation scope.
        std::vector<ic::bind_type> bind_values;
        bind_values.reserve(cmap.size());

        // Bind values to the statement.
        std::size_t index = 0;
        for (auto&& c : cmap) {
            const auto& key = c.first;

            const auto& bind_fcn = c.second;
            ic::bind_parameters bp{statement, index, _after, key, bind_values};

            bind_fcn(bp);

            index++;
        }

        const auto data_id = std::stoul(_before.at("data_id").get<std::string>());
        log::database::trace("binding data_id:[{}] at [{}]", data_id, index);
        statement.bind(index++, &data_id);

        const auto resc_id = std::stoul(_before.at("resc_id").get<std::string>());
        log::database::trace("binding resc_id:[{}] at [{}]", resc_id, index);
        statement.bind(index, &resc_id);

        execute(statement);
    } // set_replica_state

    auto set_data_object_state(
        nanodbc::connection& _db_conn,
        nanodbc::transaction& _trans,
        json& _replicas) -> void
    {
        try {
            for (auto& r : _replicas) {
                auto& after = r.at("after");
                validate_values(after);

                set_replica_state(_db_conn, r.at("before"), after);
            }

            irods::log(LOG_DEBUG10, "committing transaction");
            _trans.commit();
        }
        catch (const nanodbc::database_error& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }
        catch (const std::exception& e) {
            THROW(SYS_INTERNAL_ERR, e.what());
        }
    } // set_data_object_state

    auto set_file_object_keywords(const json& _src, irods::file_object_ptr _obj) -> void
    {
        irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - src:[{}]", __FUNCTION__, __LINE__, _src.dump()));

        const auto src = irods::experimental::make_key_value_proxy(*irods::to_key_value_pair(_src));

        // Template argument deduction is ambiguous here because cond_input() is overloaded
        auto out = irods::experimental::make_key_value_proxy<KeyValPair>(_obj->cond_input());

        // TODO: just make a make_key_value_proxy which accepts a std::map/json structure...
        if (src.contains(ADMIN_KW)) {
            out[ADMIN_KW] = src.at(ADMIN_KW);
        }
        if (src.contains(IN_PDMO_KW)) {
            _obj->in_pdmo(src.at(IN_PDMO_KW).value().data());
            out[IN_PDMO_KW] = src.at(IN_PDMO_KW);
        }
        if (src.contains(OPEN_TYPE_KW)) {
            out[OPEN_TYPE_KW] = src.at(OPEN_TYPE_KW);
        }
        if (src.contains(SYNC_OBJ_KW)) {
            out[SYNC_OBJ_KW] = src.at(SYNC_OBJ_KW);
        }
        if (src.contains(REPL_STATUS_KW)) {
            out[REPL_STATUS_KW] = src.at(REPL_STATUS_KW);
        }
        if (src.contains(IN_REPL_KW)) {
            out[IN_REPL_KW] = src.at(IN_REPL_KW);
        }
    } // set_file_object_keywords

    auto invoke_file_modified(RsComm& _comm, json& _replicas) -> int
    {
        try {
            for (auto&& replica : _replicas) {
                irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - replica:[{}]", __FUNCTION__, __LINE__, replica.dump()));

                if (replica.contains(FILE_MODIFIED_KW)) {
                    const auto data_id = std::stoll(replica.at("after").at("data_id").get<std::string>());
                    auto obj = irods::file_object_factory(_comm, data_id);

                    const auto leaf_resource_id = std::stoll(replica.at("after").at("resc_id").get<std::string>());
                    obj->resc_hier(resc_mgr.leaf_id_to_hier(leaf_resource_id));

                    set_file_object_keywords(replica.at(FILE_MODIFIED_KW), obj);

                    const auto obj_cond_input = irods::experimental::make_key_value_proxy(obj->cond_input());
                    if (obj_cond_input.contains(IN_REPL_KW) ||
                        (obj_cond_input.contains(OPEN_TYPE_KW) &&
                         obj_cond_input.at(OPEN_TYPE_KW).value() == std::to_string(OPEN_FOR_READ_TYPE))) {
                        return 0;
                    }

                    if (const auto ret = fileModified(&_comm, obj); !ret.ok()) {
                        irods::log(LOG_ERROR, fmt::format(
                            "[{}] - failed to signal the resource that [{}] on [{}] was modified",
                            __FUNCTION__, obj->logical_path(), obj->resc_hier()));

                        return ret.code();
                    }

                    irods::log(LOG_DEBUG, fmt::format(
                        "[{}:{}] - fileModified complete,obj:[{}],hier:[{}]",
                        __FUNCTION__, __LINE__, obj->logical_path(), obj->resc_hier()));

                    // TODO: consider more than one?
                    return 0;
                }
            }

            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - no fileModified",
                __FUNCTION__, __LINE__));

            return 0;
        }
        catch (const irods::exception& e) {
            irods::log(e);
            return e.code();
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}] - [{}]", __FUNCTION__, e.what()));
            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - unknown error occurred", __FUNCTION__, __LINE__));
            return SYS_UNKNOWN_ERROR;
        }
    } // invoke_file_modified

    auto rs_data_object_finalize(
        RsComm* _comm,
        BytesBuf* _input,
        BytesBuf** _output) -> int
    {
        try {
            if (!ic::connected_to_catalog_provider(*_comm)) {
                log::api::trace("Redirecting request to catalog service provider ...");

                auto host_info = ic::redirect_to_catalog_provider(*_comm);

                std::string_view json_input(static_cast<const char*>(_input->buf), _input->len);
                char* json_output = nullptr;

                const auto ec = rc_data_object_finalize(host_info.conn, json_input.data(), &json_output);
                *_output = to_bytes_buffer(json_output);

                return ec;
            }

            ic::throw_if_catalog_provider_service_role_is_invalid();
        }
        catch (const irods::exception& e) {
            std::string_view msg = e.what();
            log::api::error(msg.data());
            *_output = to_bytes_buffer(make_error_object(msg.data()).dump());
            return e.code();
        }

        json input;

        try {
            input = json::parse(std::string(static_cast<const char*>(_input->buf), _input->len));
            log::database::debug("json input:[{}]", input.dump());
        }
        catch (const json::parse_error& e) {
            std::string_view msg = e.what();
            log::api::error({{"log_message", "Failed to parse input into JSON"},
                             {"error_message", msg.data()}});

            const auto err_info = make_error_object(msg.data());
            *_output = to_bytes_buffer(err_info.dump());

            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }

        json replicas;
        bool trigger_file_modified;

        try {
            replicas = input.at("replicas");
            trigger_file_modified = input.contains("trigger_file_modified") && input.at("trigger_file_modified").get<bool>();
        }
        catch (const json::type_error& e) {
            *_output = to_bytes_buffer(make_error_object(e.what()).dump());
            return SYS_INVALID_INPUT_PARAM;
        }
        catch (const std::exception& e) {
            *_output = to_bytes_buffer(make_error_object(e.what()).dump());
            return SYS_INVALID_INPUT_PARAM;
        }

        // TODO: check permissions on data object?

        nanodbc::connection db_conn;

        try {
            std::tie(std::ignore, db_conn) = ic::new_database_connection();
        }
        catch (const std::exception& e) {
            log::database::error(e.what());
            return SYS_CONFIG_FILE_ERR;
        }

        try {
            const auto ec = ic::execute_transaction(db_conn, [&](auto& _trans) -> int
            {
                set_data_object_state(db_conn, _trans, replicas);
                *_output = to_bytes_buffer("{}");
                return 0;
            });

            if (ec < 0 || !trigger_file_modified) {
                return ec;
            }

            return invoke_file_modified(*_comm, replicas);
        }
        catch (const irods::exception& e) {
            log::database::error(e.what());
            *_output = to_bytes_buffer(make_error_object(e.what()).dump());
            return e.code();
        }
    } // rs_data_object_finalize

    const operation op = rs_data_object_finalize;
    #define CALL_DATA_OBJECT_FINALIZE call_data_object_finalize
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace {
    using operation = std::function<int(RsComm*, BytesBuf*, BytesBuf**)>;
    const operation op{};
    #define CALL_DATA_OBJECT_FINALIZE nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_whitelist::instance().add(DATA_OBJECT_FINALIZE_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{DATA_OBJECT_FINALIZE_APN,                   // API number
                        RODS_API_VERSION,                           // API version
                        NO_USER_AUTH,                               // Client auth
                        NO_USER_AUTH,                               // Proxy auth
                        "BytesBuf_PI", 0,                           // In PI / bs flag
                        "BytesBuf_PI", 0,                           // Out PI / bs flag
                        op,                                         // Operation
                        "data_object_finalize",                     // Operation name
                        nullptr,                                    // Null clear function
                        (funcPtr) CALL_DATA_OBJECT_FINALIZE};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    api->out_pack_key = "BytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
}

