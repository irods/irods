#include "atomic_apply_database_operations.hpp"

#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "irods_exception.hpp"
#include "catalog.hpp"

#include <boost/iterator/function_input_iterator.hpp>
#include <fmt/format.h>
#include <nanodbc/nanodbc.h>

#include <cctype>
#include <string>
#include <string_view>
#include <iterator>
#include <tuple>
#include <array>
#include <vector>
#include <algorithm>
#include <chrono>

// TEAM DISCUSSION:
// - Keep it a library (APIs can invoke this and guard this call with permission checks, etc.).
// - Automated Ingest, NFSRODS, Change Log Listener would use this library.
// - IDEA: Any API Plugin that invokes this library needs to provide a [cryptographic] access token in order for things to work.
//   - How do we provide protection around this?
//     - Could have a special group that can execute this.
//     - Do we narrow it even more and put the secret in each server_config.json?
//       - Scoped to a particular server.
//       - The plugin specific configuration can contain a list of blessed users and groups, etc.
//         - Example: {"user_1": "token_1", "user_2": "token_2", "group_1": "token_3"}
//     - You must be an admin. 
// - Provide overloads that accept std::string, std::string_view, etc.
//
// ADD SUPPORT FOR R_RESC_MAIN:
// iadmin modresc <resc> move <from_parent_resc_name> <to_parent_resc_name>
// iadmin addchildtoresc <parent> <child>
// iadmin moveresc <resc> <from> <to>
// iadmin mvresc <resc> <from> <to>
// iadmin mr <resc> <from> <to>
// iadmin mkparentresc <resc> <parent>
// iadmin makeparentresc <resc> <parent>
// iadmin mkpr <resc> <parent>
// iadmin newparent <resc> <parent>
//
// **iadmin setparentresc <resc> <parent>**
// - This must log all of its thoughts.
// - Check for cycles (ancestors and descendants).
// - "iadmin addchildtoresc" can call "iadmin setparentresc".
// - "iadmin setparentresc <resc> <empty_string>" is an error.

namespace
{
    // clang-format off
    namespace ix  = irods::experimental;
    namespace ic  = irods::experimental::catalog;
    namespace dml = irods::experimental::dml;

    using db_operation_type = std::tuple<std::string_view, std::string_view>;
    using db_column_type    = std::tuple<std::string_view, std::string_view>;

    //
    // Supported Table Columns
    //

    // Helpers for visitor pattern. See https://en.cppreference.com/w/cpp/utility/variant/visit
    // for more details.
    template <typename ...Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <typename ...Ts> overloaded(Ts...) -> overloaded<Ts...>;

    template <typename T, typename ...Args>
    constexpr auto make_array(Args&&... _args)
    {
        return std::array<T, sizeof...(Args)>{std::forward<Args>(_args)...};
    } // make_array

    constexpr auto db_columns = make_array<db_column_type>(
        db_column_type{"r_data_main", "data_id"},
        db_column_type{"r_data_main", "coll_id"},
        db_column_type{"r_data_main", "data_name"},
        db_column_type{"r_data_main", "data_repl_num"},
        db_column_type{"r_data_main", "data_version"},
        db_column_type{"r_data_main", "data_type_name"},
        db_column_type{"r_data_main", "data_size"},
        db_column_type{"r_data_main", "resc_group_name"},
        db_column_type{"r_data_main", "resc_name"},
        db_column_type{"r_data_main", "data_path"},
        db_column_type{"r_data_main", "data_owner_name"},
        db_column_type{"r_data_main", "data_owner_zone"},
        db_column_type{"r_data_main", "data_is_dirty"},
        db_column_type{"r_data_main", "data_status"},
        db_column_type{"r_data_main", "data_checksum"},
        db_column_type{"r_data_main", "data_expiry_ts"},
        db_column_type{"r_data_main", "data_map_id"},
        db_column_type{"r_data_main", "data_mode"},
        db_column_type{"r_data_main", "r_comment"},
        db_column_type{"r_data_main", "create_ts"},
        db_column_type{"r_data_main", "modify_ts"},
        db_column_type{"r_data_main", "resc_hier"},
        db_column_type{"r_data_main", "resc_id"},

        db_column_type{"r_coll_main", "coll_id"},
        db_column_type{"r_coll_main", "parent_coll_name"},
        db_column_type{"r_coll_main", "coll_name"},
        db_column_type{"r_coll_main", "coll_owner_name"},
        db_column_type{"r_coll_main", "coll_owner_zone"},
        db_column_type{"r_coll_main", "coll_map_id"},
        db_column_type{"r_coll_main", "coll_inheritance"},
        db_column_type{"r_coll_main", "coll_type"},
        db_column_type{"r_coll_main", "coll_info1"},
        db_column_type{"r_coll_main", "coll_info2"},
        db_column_type{"r_coll_main", "coll_expiry_ts"},
        db_column_type{"r_coll_main", "r_comment"},
        db_column_type{"r_coll_main", "create_ts"},
        db_column_type{"r_coll_main", "modify_ts"},

        db_column_type{"r_meta_main", "meta_id"},
        db_column_type{"r_meta_main", "meta_namespace"},
        db_column_type{"r_meta_main", "meta_attr_name"},
        db_column_type{"r_meta_main", "meta_attr_value"},
        db_column_type{"r_meta_main", "meta_attr_unit"},
        db_column_type{"r_meta_main", "r_comment"},
        db_column_type{"r_meta_main", "create_ts"},
        db_column_type{"r_meta_main", "modify_ts"},

        db_column_type{"r_objt_metamap", "object_id"},
        db_column_type{"r_objt_metamap", "meta_id"},
        db_column_type{"r_objt_metamap", "create_ts"},
        db_column_type{"r_objt_metamap", "modify_ts"},

        db_column_type{"r_resc_main", "resc_id"},
        db_column_type{"r_resc_main", "resc_name"},
        db_column_type{"r_resc_main", "zone_name"},
        db_column_type{"r_resc_main", "resc_type_name"},
        db_column_type{"r_resc_main", "resc_class_name"},
        db_column_type{"r_resc_main", "resc_net"},
        db_column_type{"r_resc_main", "resc_def_path"},
        db_column_type{"r_resc_main", "free_space"},
        db_column_type{"r_resc_main", "free_space_ts"},
        db_column_type{"r_resc_main", "resc_info"},
        db_column_type{"r_resc_main", "r_comment"},
        db_column_type{"r_resc_main", "resc_status"},
        db_column_type{"r_resc_main", "create_ts"},
        db_column_type{"r_resc_main", "modify_ts"},
        db_column_type{"r_resc_main", "resc_children"},
        db_column_type{"r_resc_main", "resc_context"},
        db_column_type{"r_resc_main", "resc_parent"},
        db_column_type{"r_resc_main", "resc_objcount"},
        db_column_type{"r_resc_main", "resc_parent_context"}
    ); // db_columns

    constexpr std::array<std::string_view, 7> supported_operators{"=", "!=", ">", ">=", "<", "<=", "in"};

    constexpr auto supported_operations = make_array<db_operation_type>(
        db_operation_type{"insert", "r_coll_main"},
        db_operation_type{"insert", "r_data_main"},
        db_operation_type{"insert", "r_meta_map"},
        db_operation_type{"insert", "r_objt_metamap"},
        db_operation_type{"insert", "r_resc_main"},

        db_operation_type{"update", "r_coll_main"},
        db_operation_type{"update", "r_data_main"},
        db_operation_type{"update", "r_meta_map"},
        db_operation_type{"update", "r_objt_metamap"},
        db_operation_type{"update", "r_resc_main"},

        db_operation_type{"delete", "r_coll_main"},
        db_operation_type{"delete", "r_data_main"},
        db_operation_type{"delete", "r_meta_map"},
        db_operation_type{"delete", "r_objt_metamap"},
        db_operation_type{"delete", "r_resc_main"}
    ); // supported_operations

    //
    // Function Prototypes
    //

    auto insert_collection(nanodbc::connection& _db_conn,
                           const std::string_view _db_instance_name,
                           const dml::insert_op& _op) -> int;

    auto insert_replica(nanodbc::connection& _db_conn,
                        const std::string_view _db_instance_name,
                        const dml::insert_op& _op) -> int;

    auto insert_metadata(nanodbc::connection& _db_conn,
                         const std::string_view _db_instance_name,
                         const dml::insert_op& _op) -> int;

    auto insert_metadata_link(nanodbc::connection& _db_conn,
                              const std::string_view _db_instance_name,
                              const dml::insert_op& _op) -> int;

    auto insert_resource(nanodbc::connection& _db_conn,
                         const std::string_view _db_instance_name,
                         const dml::insert_op& _op) -> int;

    auto update_rows(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::update_op& _op) -> int;

    auto delete_rows(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::delete_op& _op) -> int;

    auto exec_insert(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::insert_op& _op) -> int;

    auto exec_update(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::update_op& _op) -> int;

    auto exec_delete(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::delete_op& _op) -> int;

    auto is_db_operation_supported(const std::string_view _db_op,
                                   const std::string_view _table_name) -> bool;

    auto throw_if_invalid_table_column(const std::string_view _column,
                                       const std::string_view _table_name) -> void;

    auto throw_if_invalid_conditional_operator(const std::string_view _operator) -> void;

    auto current_timestamp() -> std::string;

    auto to_upper(std::string& _s) -> std::string&;

    auto to_upper_copy(const std::string& _s) -> std::string;

    template <typename Container>
    auto at(const Container& _data, const std::string_view _column_name) -> std::string;

    template <typename Container>
    auto value(const Container& _data,
               const std::string_view _column_name,
               const std::string_view _fallback_value) -> std::string;

    auto init_conditionals(const std::string_view _table_name,
                           const std::vector<dml::condition>& _conditions)
        -> std::tuple<std::string, std::vector<std::string>>;
    // clang-format on

    auto insert_collection(nanodbc::connection& _db_conn,
                           const std::string_view _db_instance_name,
                           const dml::insert_op& _op) -> int
    {
        /*
                Column      |          Type           |           Modifiers
          ------------------+-------------------------+--------------------------------
           coll_id          | bigint                  | not null
           parent_coll_name | character varying(2700) | not null
           coll_name        | character varying(2700) | not null
           coll_owner_name  | character varying(250)  | not null
           coll_owner_zone  | character varying(250)  | not null
           coll_map_id      | bigint                  | default 0
           coll_inheritance | character varying(1000) | 
           coll_type        | character varying(250)  | default '0'::character varying
           coll_info1       | character varying(2700) | default '0'::character varying
           coll_info2       | character varying(2700) | default '0'::character varying
           coll_expiry_ts   | character varying(32)   | 
           r_comment        | character varying(1000) | 
           create_ts        | character varying(32)   | 
           modify_ts        | character varying(32)   |
         */

        //rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
            std::string_view sql;

            if (_db_instance_name == "postgres") {
                sql = "insert into R_COLL_MAIN ("
                      " coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_map_id,"
                      " coll_inheritance, coll_type, coll_info1, coll_info2, coll_expiry_ts, r_comment,"
                      " create_ts, modify_ts) "
                      "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else if (_db_instance_name == "oracle") {
                sql = "insert into R_COLL_MAIN ("
                      " coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_map_id,"
                      " coll_inheritance, coll_type, coll_info1, coll_info2, coll_expiry_ts, r_comment,"
                      " create_ts, modify_ts) "
                      "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else if (_db_instance_name == "mysql") {
                sql = "insert into R_COLL_MAIN ("
                      " coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_map_id,"
                      " coll_inheritance, coll_type, coll_info1, coll_info2, coll_expiry_ts, r_comment,"
                      " create_ts, modify_ts) "
                      "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else {
                // TODO Handle error.
            }

            nanodbc::prepare(stmt, sql.data());

            const auto& parent_coll_name = at(_op.data, "parent_coll_name");          // Cannot be null
            const auto& coll_name = at(_op.data, "coll_name");                        // Cannot be null
            const auto& coll_owner_name = at(_op.data, "coll_owner_name");            // Cannot be null
            const auto& coll_owner_zone = at(_op.data, "coll_owner_zone");            // Cannot be null
            const auto coll_map_id = std::stoi(value(_op.data, "coll_map_id", "0"));  // Cannot be null
            const auto coll_inheritance = value(_op.data, "coll_inheritance", "");
            const auto coll_type = value(_op.data, "coll_type", "");
            const auto coll_info_1 = value(_op.data, "coll_info1", "");
            const auto coll_info_2 = value(_op.data, "coll_info2", "");
            const auto coll_expiry_ts = value(_op.data, "coll_expiry_ts", "");
            const auto r_comment = value(_op.data, "r_comment", "");
            const auto create_ts = value(_op.data, "create_ts", timestamp);
            const auto modify_ts = value(_op.data, "modify_ts", timestamp);

            int i = 0;
            stmt.bind(i, parent_coll_name.data());
            stmt.bind(++i, coll_name.data());
            stmt.bind(++i, coll_owner_name.data());
            stmt.bind(++i, coll_owner_zone.data());
            stmt.bind(++i, &coll_map_id);
            stmt.bind(++i, coll_inheritance.data());
            stmt.bind(++i, coll_type.data());
            stmt.bind(++i, coll_info_1.data());
            stmt.bind(++i, coll_info_2.data());
            stmt.bind(++i, coll_expiry_ts.data());
            stmt.bind(++i, r_comment.data());
            stmt.bind(++i, create_ts.data());
            stmt.bind(++i, modify_ts.data());

            nanodbc::execute(stmt);
        }
        catch (const irods::exception&) {
            throw;
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_collection

    auto insert_replica(nanodbc::connection& _db_conn,
                        const std::string_view _db_instance_name,
                        const dml::insert_op& _op) -> int
    {
        /*
               Column      |          Type           |           Modifiers            
          -----------------+-------------------------+--------------------------------
           data_id         | bigint                  | not null
           coll_id         | bigint                  | not null
           data_name       | character varying(1000) | not null
           data_repl_num   | integer                 | not null
           data_version    | character varying(250)  | default '0'::character varying
           data_type_name  | character varying(250)  | not null
           data_size       | bigint                  | not null
           resc_group_name | character varying(250)  | 
           resc_name       | character varying(250)  | not null
           data_path       | character varying(2700) | not null
           data_owner_name | character varying(250)  | not null
           data_owner_zone | character varying(250)  | not null
           data_is_dirty   | integer                 | default 0
           data_status     | character varying(250)  | 
           data_checksum   | character varying(1000) | 
           data_expiry_ts  | character varying(32)   | 
           data_map_id     | bigint                  | default 0
           data_mode       | character varying(32)   | 
           r_comment       | character varying(1000) | 
           create_ts       | character varying(32)   | 
           modify_ts       | character varying(32)   | 
           resc_hier       | character varying(1000) | 
           resc_id         | bigint                  | 
         */

        //rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
            std::string_view sql;

            if (_db_instance_name == "postgres") {
                sql = "insert into R_DATA_MAIN ("
                      " data_id, coll_id, data_name, data_repl_num, data_version, data_type_name," 
                      " data_size, resc_group_name, resc_name, data_path, data_owner_name,"
                      " data_owner_zone, data_is_dirty, data_status, data_checksum, data_expiry_ts,"
                      " data_map_id, data_mode, r_comment, create_ts, modify_ts, resc_hier, resc_id) "
                      "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else if (_db_instance_name == "oracle") {
                sql = "insert into R_DATA_MAIN ("
                      " data_id, coll_id, data_name, data_repl_num, data_version, data_type_name," 
                      " data_size, resc_group_name, resc_name, data_path, data_owner_name,"
                      " data_owner_zone, data_is_dirty, data_status, data_checksum, data_expiry_ts,"
                      " data_map_id, data_mode, r_comment, create_ts, modify_ts, resc_hier, resc_id) "
                      "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else if (_db_instance_name == "mysql") {
                sql = "insert into R_DATA_MAIN ("
                      " data_id, coll_id, data_name, data_repl_num, data_version, data_type_name," 
                      " data_size, resc_group_name, resc_name, data_path, data_owner_name,"
                      " data_owner_zone, data_is_dirty, data_status, data_checksum, data_expiry_ts,"
                      " data_map_id, data_mode, r_comment, create_ts, modify_ts, resc_hier, resc_id) "
                      "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else {
                // TODO Handle error.
            }

            nanodbc::prepare(stmt, sql.data());

            const auto coll_id = std::stoi(at(_op.data, "coll_id"));                           // Cannot be null
            const auto& data_name = at(_op.data, "data_name");                                 // Cannot be null
            const auto data_repl_num = std::stoi(at(_op.data, "data_repl_num"));               // Cannot be null
            const auto data_version = value(_op.data, "data_version", "0");
            const auto& data_type_name = at(_op.data, "data_type_name");                       // Cannot be null
            const auto data_size = value(_op.data, "data_size", "0");                          // Cannot be null
            const auto resc_group_name = value(_op.data, "resc_group_name", "EMPTY_RESC_GROUP_NAME");
            const auto resc_name = value(_op.data, "resc_name", "EMPTY_RESC_NAME");            // Cannot be null
            const auto& data_path = at(_op.data, "data_path");                                 // Cannot be null
            const auto& data_owner_name = at(_op.data, "data_owner_name");                     // Cannot be null
            const auto& data_owner_zone = at(_op.data, "data_owner_zone");                     // Cannot be null
            const auto data_is_dirty = std::stoi(value(_op.data, "data_is_dirty", "0"));
            const auto data_status = value(_op.data, "data_status", "");
            const auto data_checksum = value(_op.data, "data_checksum", "");
            const auto data_expiry_ts = value(_op.data, "data_expiry_ts", "");
            const auto data_map_id = std::stoll(value(_op.data, "data_map_id", "0"));
            const auto data_mode = value(_op.data, "data_mode", "");
            const auto r_comment = value(_op.data, "r_comment", "");
            const auto create_ts = value(_op.data, "create_ts", timestamp);
            const auto modify_ts = value(_op.data, "modify_ts", timestamp);
            const auto resc_hier = value(_op.data, "resc_hier", "EMPTY_RESC_HIER");
            const auto resc_id = std::stoi(at(_op.data, "resc_id"));                          // This is required.

            int i = 0;
            stmt.bind(i, &coll_id);
            stmt.bind(++i, data_name.data());
            stmt.bind(++i, &data_repl_num);
            stmt.bind(++i, data_version.data());
            stmt.bind(++i, data_type_name.data());
            stmt.bind(++i, data_size.data());
            stmt.bind(++i, resc_group_name.data());
            stmt.bind(++i, resc_name.data());
            stmt.bind(++i, data_path.data());
            stmt.bind(++i, data_owner_name.data());
            stmt.bind(++i, data_owner_zone.data());
            stmt.bind(++i, &data_is_dirty);
            stmt.bind(++i, data_status.data());
            stmt.bind(++i, data_checksum.data());
            stmt.bind(++i, data_expiry_ts.data());
            stmt.bind(++i, &data_map_id);
            stmt.bind(++i, data_mode.data());
            stmt.bind(++i, r_comment.data());
            stmt.bind(++i, create_ts.data());
            stmt.bind(++i, modify_ts.data());
            stmt.bind(++i, resc_hier.data());
            stmt.bind(++i, &resc_id);

            nanodbc::execute(stmt);
        }
        catch (const irods::exception&) {
            throw;
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_replica

    auto insert_metadata(nanodbc::connection& _db_conn,
                         const std::string_view _db_instance_name,
                         const dml::insert_op& _op) -> int
    {
        /*
               Column      |          Type           | Modifiers 
          -----------------+-------------------------+-----------
           meta_id         | bigint                  | not null
           meta_namespace  | character varying(250)  | 
           meta_attr_name  | character varying(2700) | not null
           meta_attr_value | character varying(2700) | not null
           meta_attr_unit  | character varying(250)  | 
           r_comment       | character varying(1000) | 
           create_ts       | character varying(32)   | 
           modify_ts       | character varying(32)   |
         */

        //rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
            std::string_view sql;

            if (_db_instance_name == "postgres") {
                sql = "insert into R_META_MAIN ("
                      " meta_id, meta_namespace, meta_attr_name, meta_attr_value,"
                      " meta_attr_unit, r_comment, create_ts, modify_ts) "
                      "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?, ?, ?)";
            }
            else if (_db_instance_name == "oracle") {
                sql = "insert into R_META_MAIN ("
                      " meta_id, meta_namespace, meta_attr_name, meta_attr_value,"
                      " meta_attr_unit, r_comment, create_ts, modify_ts) "
                      "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?, ?, ?)";
            }
            else if (_db_instance_name == "mysql") {
                sql = "insert into R_META_MAIN ("
                      " meta_id, meta_namespace, meta_attr_name, meta_attr_value,"
                      " meta_attr_unit, r_comment, create_ts, modify_ts) "
                      "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?, ?, ?)";
            }
            else {
                // TODO Handle error.
            }

            nanodbc::prepare(stmt, sql.data());

            const auto meta_namespace = value(_op.data, "meta_namespace", "");
            const auto& meta_attr_name = at(_op.data, "meta_attr_name");          // Cannot be null
            const auto& meta_attr_value = at(_op.data, "meta_attr_value");        // Cannot be null
            const auto meta_attr_unit = value(_op.data, "meta_attr_unit", "");
            const auto r_comment = value(_op.data, "r_comment", "");
            const auto create_ts = value(_op.data, "create_ts", timestamp);
            const auto modify_ts = value(_op.data, "modify_ts", timestamp);

            int i = 0;
            stmt.bind(i, meta_namespace.data());
            stmt.bind(++i, meta_attr_name.data());
            stmt.bind(++i, meta_attr_value.data());
            stmt.bind(++i, meta_attr_unit.data());
            stmt.bind(++i, r_comment.data());
            stmt.bind(++i, create_ts.data());
            stmt.bind(++i, modify_ts.data());

            nanodbc::execute(stmt);
        }
        catch (const irods::exception&) {
            throw;
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_metadata

    auto insert_metadata_link(nanodbc::connection& _db_conn,
                              const std::string_view _db_instance_name,
                              const dml::insert_op& _op) -> int
    {
        /*
            Column   |         Type          | Modifiers 
          -----------+-----------------------+-----------
           object_id | bigint                | not null
           meta_id   | bigint                | not null
           create_ts | character varying(32) | 
           modify_ts | character varying(32) |
         */

        //rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};

            nanodbc::prepare(stmt, "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) "
                                   "values (?, ?, ?, ?)");

            const auto& object_id = at(_op.data, "object_id");  // Cannot be null
            const auto& meta_id = at(_op.data, "meta_id");      // Cannot be null
            const auto create_ts = value(_op.data, "create_ts", timestamp);
            const auto modify_ts = value(_op.data, "modify_ts", timestamp);

            stmt.bind(0, object_id.data());
            stmt.bind(1, meta_id.data());
            stmt.bind(2, create_ts.data());
            stmt.bind(3, modify_ts.data());

            nanodbc::execute(stmt);
        }
        catch (const irods::exception&) {
            throw;
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_metadata_link

    auto insert_resource(nanodbc::connection& _db_conn,
                         const std::string_view _db_instance_name,
                         const dml::insert_op& _op) -> int
    {
        /*
                 Column        |          Type           | Modifiers 
          ---------------------+-------------------------+-----------
           resc_id             | bigint                  | not null
           resc_name           | character varying(250)  | not null
           zone_name           | character varying(250)  | not null
           resc_type_name      | character varying(250)  | not null
           resc_class_name     | character varying(250)  | not null
           resc_net            | character varying(250)  | not null
           resc_def_path       | character varying(1000) | not null
           free_space          | character varying(250)  | 
           free_space_ts       | character varying(32)   | 
           resc_info           | character varying(1000) | 
           r_comment           | character varying(1000) | 
           resc_status         | character varying(32)   | 
           create_ts           | character varying(32)   | 
           modify_ts           | character varying(32)   | 
           resc_children       | character varying(1000) | 
           resc_context        | character varying(1000) | 
           resc_parent         | character varying(1000) | 
           resc_objcount       | bigint                  | default 0
           resc_parent_context | character varying(4000) | 
           Indexes:
               "idx_resc_main2" UNIQUE, btree (zone_name, resc_name)
               "idx_resc_main1" btree (resc_id)
         */

        //rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
            std::string_view sql;

            if (_db_instance_name == "postgres") {
                sql = "insert into R_RESC_MAIN ("
                      " resc_id, resc_name, zone_name, resc_type_name, resc_class_name,"
                      " resc_net, resc_def_path, free_space, free_space_ts, resc_info,"
                      " r_comment, resc_status, create_ts, modify_ts, resc_children,"
                      " resc_context, resc_parent, resc_objcount, resc_parent_context) "
                      "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else if (_db_instance_name == "oracle") {
                sql = "insert into R_RESC_MAIN ("
                      " resc_id, resc_name, zone_name, resc_type_name, resc_class_name,"
                      " resc_net, resc_def_path, free_space, free_space_ts, resc_info,"
                      " r_comment, resc_status, create_ts, modify_ts, resc_children,"
                      " resc_context, resc_parent, resc_objcount, resc_parent_context) "
                      "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else if (_db_instance_name == "mysql") {
                sql = "insert into R_RESC_MAIN ("
                      " resc_id, resc_name, zone_name, resc_type_name, resc_class_name,"
                      " resc_net, resc_def_path, free_space, free_space_ts, resc_info,"
                      " r_comment, resc_status, create_ts, modify_ts, resc_children,"
                      " resc_context, resc_parent, resc_objcount, resc_parent_context) "
                      "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            }
            else {
                // TODO Handle error.
            }

            nanodbc::prepare(stmt, sql.data());

            const auto& resc_name = at(_op.data, "resc_name");              // Cannot be null
            const auto& zone_name = at(_op.data, "zone_name");              // Cannot be null
            const auto& resc_type_name = at(_op.data, "resc_type_name");    // Cannot be null
            const auto& resc_class_name = at(_op.data, "resc_class_name");  // Cannot be null
            const auto& resc_net = at(_op.data, "resc_net");                // Cannot be null
            const auto& resc_def_path = at(_op.data, "resc_def_path");      // Cannot be null
            const auto free_space = value(_op.data, "free_space", "");
            const auto free_space_ts = value(_op.data, "free_space_ts", "");
            const auto resc_info = value(_op.data, "resc_info", "");
            const auto r_comment = value(_op.data, "r_comment", "");
            const auto resc_status = value(_op.data, "resc_status", "");
            const auto create_ts = value(_op.data, "create_ts", timestamp);
            const auto modify_ts = value(_op.data, "modify_ts", timestamp);
            const auto resc_children = value(_op.data, "resc_children", "");
            const auto resc_context = value(_op.data, "resc_context", "");
            const auto resc_parent = value(_op.data, "resc_parent", "");
            const auto resc_objcount = std::stoi(value(_op.data, "resc_objcount", "0"));
            const auto resc_parent_context = value(_op.data, "resc_parent_context", "");

            int i = 0;
            stmt.bind(i, resc_name.data());
            stmt.bind(++i, zone_name.data());
            stmt.bind(++i, resc_type_name.data());
            stmt.bind(++i, resc_class_name.data());
            stmt.bind(++i, resc_net.data());
            stmt.bind(++i, resc_def_path.data());
            stmt.bind(++i, free_space.data());
            stmt.bind(++i, free_space_ts.data());
            stmt.bind(++i, resc_info.data());
            stmt.bind(++i, r_comment.data());
            stmt.bind(++i, resc_status.data());
            stmt.bind(++i, create_ts.data());
            stmt.bind(++i, modify_ts.data());
            stmt.bind(++i, resc_children.data());
            stmt.bind(++i, resc_context.data());
            stmt.bind(++i, resc_parent.data());
            stmt.bind(++i, &resc_objcount);
            stmt.bind(++i, resc_parent_context.data());

            nanodbc::execute(stmt);
        }
        catch (const irods::exception&) {
            throw;
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_resource

    auto update_rows(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::update_op& _op) -> int
    {
        //rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            if (_op.data.empty()) {
                return SYS_INVALID_INPUT_PARAM;
            }

            if (_op.conditions.empty()) {
                return SYS_INVALID_INPUT_PARAM;
            }

            std::vector<std::string> set_clauses;
            set_clauses.reserve(_op.data.size());

            std::vector<std::string> set_bind_args;
            set_bind_args.reserve(_op.data.size());

            // Build two lists of strings.
            // - One that holds the SQL set syntax.
            // - One that holds the values for the prepared statement.
            for (auto&& [name, value] : _op.data) {
                throw_if_invalid_table_column(name, _op.table);
                set_clauses.push_back(fmt::format("{} = ?", name));
                set_bind_args.push_back(value);
            }

            // Join the set operations together.
            const auto set_string = fmt::format("{}", fmt::join(set_clauses, ", "));

            const auto [conditions_string, conditions_bind_args] = init_conditionals(_op.table, _op.conditions);

            // Generate the final SQL.
            const auto sql = fmt::format("update {} set {} where {}", to_upper_copy(_op.table), set_string, conditions_string);
            rodsLog(LOG_NOTICE, "%s :: Generated SQL = [%s]", __func__, sql.data());

#ifdef ATOMIC_DB_OPS_RUN_SQL
            nanodbc::statement stmt{_db_conn};
            nanodbc::prepare(stmt, sql);

            // Bind "set" values to prepared statement parameter markers.
            for (std::size_t i = 0; i < set_bind_args.size(); ++i) {
                stmt.bind(i, set_bind_args[i].data());
            }

            // Bind "condition" values to prepared statement parameter markers.
            for (std::size_t i = 0; i < conditions_bind_args.size(); ++i) {
                stmt.bind(i + set_bind_args.size(), conditions_bind_args[i].data());
            }

            nanodbc::execute(stmt);
#endif
        }
        catch (const irods::exception&) {
            throw;
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // update_rows

    auto delete_rows(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::delete_op& _op) -> int
    {
        //rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            if (_op.conditions.empty()) {
                return SYS_INVALID_INPUT_PARAM;
            }

            const auto [conditions_string, bind_args] = init_conditionals(_op.table, _op.conditions);

            // Generate the final SQL.
            const auto sql = fmt::format("delete from {} where {}", to_upper_copy(_op.table), conditions_string);
            rodsLog(LOG_NOTICE, "%s :: Generated SQL = [%s]", __func__, sql.data());

#ifdef ATOMIC_DB_OPS_RUN_SQL
            nanodbc::statement stmt{_db_conn};
            nanodbc::prepare(stmt, sql);

            // Bind condition values to prepared statement parameter markers.
            for (std::size_t i = 0; i < bind_args.size(); ++i) {
                stmt.bind(i, bind_args[i].data());
            }

            nanodbc::execute(stmt);
#endif
        }
        catch (const irods::exception&) {
            throw;
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // delete_rows

    auto exec_insert(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::insert_op& _op) -> int
    {
        if (!is_db_operation_supported("insert", _op.table)) {
            return INVALID_OPERATION;
        }

        // clang-format off
        if (_op.table == "r_coll_main")    { return insert_collection(_db_conn, _db_instance_name, _op); }
        if (_op.table == "r_data_main")    { return insert_replica(_db_conn, _db_instance_name, _op); }
        if (_op.table == "r_meta_main")    { return insert_metadata(_db_conn, _db_instance_name, _op); }
        if (_op.table == "r_objt_metamap") { return insert_metadata_link(_db_conn, _db_instance_name, _op); }
        if (_op.table == "r_resc_main")    { return insert_resource(_db_conn, _db_instance_name, _op); }
        // clang-format on

        rodsLog(LOG_ERROR, "%s :: No insert statement executed! This message should not be reachable!", __func__);

        return SYS_INTERNAL_ERR;
    } // exec_insert

    auto exec_update(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::update_op& _op) -> int
    {
        if (!is_db_operation_supported("update", _op.table)) {
            return INVALID_OPERATION;
        }

        return update_rows(_db_conn, _db_instance_name, _op);
    } // exec_update

    auto exec_delete(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const dml::delete_op& _op) -> int
    {
        if (!is_db_operation_supported("delete", _op.table)) {
            return INVALID_OPERATION;
        }

        return delete_rows(_db_conn, _db_instance_name, _op);
    } // exec_delete

    auto is_db_operation_supported(const std::string_view _db_op,
                                   const std::string_view _table_name) -> bool
    {
        const auto pred = [target = db_operation_type{_db_op, _table_name}](const db_operation_type& _v) noexcept
        {
            return _v == target;
        };

        if (std::none_of(std::begin(supported_operations), std::end(supported_operations), pred)) {
            const auto msg = fmt::format("Database operation [{}] not supported on table [{}].", _db_op, _table_name);
            rodsLog(LOG_ERROR, "%s :: %s", __func__, msg.data());
            return false;
        }

        return true;
    } // is_db_operation_supported

    auto throw_if_invalid_table_column(const std::string_view _column,
                                       const std::string_view _table_name) -> void
    {
        const auto pred = [target = db_column_type{_table_name, _column}](const db_column_type& _v) noexcept
        {
            return _v == target;
        };

        if (std::none_of(std::begin(db_columns), std::end(db_columns), pred)) {
            const auto msg = fmt::format("Invalid table column [{1}.{0}].", _column, _table_name);
            rodsLog(LOG_ERROR, "%s :: %s", __func__, msg.data());
            THROW(SYS_INVALID_INPUT_PARAM, msg);
        }
    } // throw_if_invalid_table_column

    auto throw_if_invalid_conditional_operator(const std::string_view _operator) -> void
    {
        const auto pred = [&_operator](const std::string_view _v) noexcept
        {
            return _v == _operator;
        };

        if (std::none_of(std::begin(supported_operators), std::end(supported_operators), pred)) {
            const auto msg = fmt::format("Invalid conditional operator [{}].", _operator);
            rodsLog(LOG_ERROR, "%s :: %s", __func__, msg.data());
            THROW(SYS_INVALID_INPUT_PARAM, msg);
        }
    } // throw_if_invalid_conditional_operator

    auto current_timestamp() -> std::string
    {
        using std::chrono::system_clock;
        using std::chrono::duration_cast;
        using std::chrono::seconds;

        return fmt::format("{:011}", duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
    } // current_timestamp

    auto to_upper(std::string& _s) -> std::string&
    {
        std::transform(std::begin(_s), std::end(_s), std::begin(_s), [](unsigned char _c) {
            return std::toupper(_c);
        });

        return _s;
    } // to_upper

    auto to_upper_copy(const std::string& _s) -> std::string
    {
        auto t = _s;
        return to_upper(t);
    } // to_upper_copy

    template <typename Container>
    auto at(const Container& _data, const std::string_view _column_name) -> std::string
    {
        const auto end = std::end(_data);
        const auto iter = std::find_if(std::begin(_data), end, [&_column_name](const dml::column& _v) {
            return _v.name == _column_name;
        });

        if (iter == end) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("No data set for column [{}].", _column_name));
        }

        return iter->value;
    } // at

    template <typename Container>
    auto value(const Container& _data,
               const std::string_view _column_name,
               const std::string_view _fallback_value) -> std::string
    {
        const auto end = std::end(_data);
        const auto iter = std::find_if(std::begin(_data), end, [&_column_name](const dml::column& _v) {
            return _v.name == _column_name;
        });

        if (iter == end) {
            return _fallback_value.data();
        }

        return iter->value;
    } // value

    auto init_conditionals(const std::string_view _table_name,
                           const std::vector<dml::condition>& _conditions)
        -> std::tuple<std::string, std::vector<std::string>>
    {
        using vector_type = std::vector<std::string>;

        vector_type condition_clauses;
        condition_clauses.reserve(_conditions.size());

        vector_type values;
        values.reserve(_conditions.size());

        // Build two lists of strings.
        // - One that holds the SQL condition syntax.
        // - One that holds the values for the prepared statement.
        for (auto&& c : _conditions) {
            throw_if_invalid_table_column(c.column, _table_name);
            throw_if_invalid_conditional_operator(c.op);

            if ("in" == c.op) {
                struct
                {
                    using result_type = std::string_view;
                    auto operator()() const noexcept -> result_type { return "?"; }
                } gen;

                // Generate the placeholder syntax for the "in" SQL operator and add
                // the bind argument for each placeholder to the values vector.
                auto first = boost::make_function_input_iterator(gen, vector_type::size_type{0});
                auto last = boost::make_function_input_iterator(gen, c.values.size());
                condition_clauses.push_back(fmt::format("{} in ({})", c.column, fmt::join(first, last, ", ")));
                values.insert(std::end(values), std::begin(c.values), std::end(c.values));
            }
            else {
                condition_clauses.push_back(fmt::format("{} {} ?", c.column, c.op));
                values.push_back(c.values.front());
            }
        }

        return {fmt::format("{}", fmt::join(condition_clauses, " and ")), std::move(values)};
    } // init_conditionals
} // anonymous namespace

namespace irods::experimental
{
    auto atomic_apply_database_operations(const std::vector<dml::operation_type>& _ops) -> int
    {
        try {
            //rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _ops.dump(4).data());

            std::string db_instance_name;
            nanodbc::connection db_conn;

            try {
                std::tie(db_instance_name, db_conn) = ic::new_database_connection();
            }
            catch (const std::exception& e) {
                rodsLog(LOG_ERROR, e.what());
                return CAT_CONNECT_ERR;
            }

            return ic::execute_transaction(db_conn, [&](nanodbc::transaction& _trans) -> int
            {
                for (auto&& op : _ops) {
                    const auto ec = std::visit(overloaded{
                        [&db_conn, &db_instance_name](const dml::insert_op& _op) {
                            return exec_insert(db_conn, db_instance_name, _op);
                        },
                        [&db_conn, &db_instance_name](const dml::update_op& _op) {
                            return exec_update(db_conn, db_instance_name, _op);
                        },
                        [&db_conn, &db_instance_name](const dml::delete_op& _op) {
                            return exec_delete(db_conn, db_instance_name, _op);
                        }
                    }, op);

                    if (ec < 0) {
                        return ec;
                    }
                }

                _trans.commit();

                return 0;
            });
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, "%s :: Caught exception: %s", __func__, e.what());
            return e.code();
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, "%s :: Caught exception: %s", __func__, e.what());
            return SYS_INTERNAL_ERR;
        }
    } // atomic_apply_database_operations

    auto atomic_apply_database_operations(const std::vector<dml::operation_type>& _ops,
                                          std::error_code& _ec) -> void
    {
        try {
            const auto ec = atomic_apply_database_operations(_ops);
            _ec.assign(ec, std::generic_category());
        }
        catch (...) {
            _ec.assign(SYS_UNKNOWN_ERROR, std::generic_category());
        }
    } // atomic_apply_database_operations
} // namespace irods::experimental

