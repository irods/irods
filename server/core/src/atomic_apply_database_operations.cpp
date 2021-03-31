#include "atomic_apply_database_operations.hpp"

#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "irods_exception.hpp"
#include "catalog.hpp"

#include <boost/container/flat_map.hpp>
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
#include <sstream>
#include <chrono>
#include <experimental/iterator> // For make_ostream_joiner

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
    namespace ic = irods::experimental::catalog;

    using json               = nlohmann::json;
    using handler_key_type   = std::tuple<std::string_view, std::string_view>;
    using handler_value_type = int (*)(nanodbc::connection&, const std::string_view, const json&);
    using db_column_type     = std::tuple<std::string_view, std::string_view>;

    //
    // JSON property keys
    //

    const char* const jp_operations = "operations";
    const char* const jp_table      = "table";
    const char* const jp_op_name    = "op_name";
    const char* const jp_values     = "values";
    const char* const jp_conditions = "conditions";
    const char* const jp_operator   = "operator";
    const char* const jp_column     = "column";
    const char* const jp_value      = "value";
    const char* const jp_updates    = "updates";

    //
    // Supported Table Columns
    //

    template <typename ...Args>
    constexpr auto make_array(Args&&... _args)
    {
        return std::array<db_column_type, sizeof...(Args)>{std::forward<Args>(_args)...};
    } // make_array

    constexpr auto db_columns = make_array(
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
        db_column_type{"r_objt_metamap", "modify_ts"}
    ); // db_columns

    //
    // Function Prototypes
    //

    auto insert_collection(nanodbc::connection& _db_conn,
                           const std::string_view _db_instance_name,
                           const json& _op) -> int;

    auto insert_replica(nanodbc::connection& _db_conn,
                        const std::string_view _db_instance_name,
                        const json& _op) -> int;

    auto insert_metadata(nanodbc::connection& _db_conn,
                         const std::string_view _db_instance_name,
                         const json& _op) -> int;

    auto insert_metadata_link(nanodbc::connection& _db_conn,
                              const std::string_view _db_instance_name,
                              const json& _op) -> int;

    auto insert_resource(nanodbc::connection& _db_conn,
                         const std::string_view _db_instance_name,
                         const json& _op) -> int;

    auto delete_rows(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const json& _op) -> int;

    auto update_rows(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const json& _op) -> int;

    auto throw_if_invalid_table_column(const std::string_view _column,
                                       const std::string_view _table_name) -> void;

    auto throw_if_invalid_conditional_operator(const std::string_view _operator) -> void;

    auto current_timestamp() -> std::string;

    auto to_upper(std::string& _s) -> std::string&;
    // clang-format on

    // clang-format off
    constexpr std::array<std::string_view, 7> supported_operators{"=", "!=", ">", ">=", "<", "<=", "in"};

    const boost::container::flat_map<handler_key_type, handler_value_type> handlers{
        {{"insert", "r_coll_main"},    insert_collection},
        {{"insert", "r_data_main"},    insert_replica},
        {{"insert", "r_meta_map"},     insert_metadata},
        {{"insert", "r_objt_metamap"}, insert_metadata_link},
        {{"insert", "r_resc_main"},    insert_resource},

        {{"update", "r_coll_main"},    update_rows},
        {{"update", "r_data_main"},    update_rows},
        {{"update", "r_meta_map"},     update_rows},
        {{"update", "r_objt_metamap"}, update_rows},
        {{"update", "r_resc_main"},    update_rows},

        {{"delete", "r_coll_main"},    delete_rows},
        {{"delete", "r_data_main"},    delete_rows},
        {{"delete", "r_meta_map"},     delete_rows},
        {{"delete", "r_objt_metamap"}, delete_rows},
        {{"delete", "r_resc_main"},    delete_rows}
    };
    // clang-format on

    auto insert_collection(nanodbc::connection& _db_conn,
                           const std::string_view _db_instance_name,
                           const json& _op) -> int
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

        rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
#if 0
            if (_db_instance_name == "postgres") {
                nanodbc::prepare(stmt, "insert into R_COLL_MAIN ("
                                       " coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_map_id,"
                                       " coll_inheritance, coll_type, coll_info1, coll_info2, coll_expiry_ts, r_comment,"
                                       " create_ts, modify_ts) "
                                       "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else if (_db_instance_name == "oracle") {
                nanodbc::prepare(stmt, "insert into R_COLL_MAIN ("
                                       " coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_map_id,"
                                       " coll_inheritance, coll_type, coll_info1, coll_info2, coll_expiry_ts, r_comment,"
                                       " create_ts, modify_ts) "
                                       "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else if (_db_instance_name == "mysql") {
                nanodbc::prepare(stmt, "insert into R_COLL_MAIN ("
                                       " coll_id, parent_coll_name, coll_name, coll_owner_name, coll_owner_zone, coll_map_id,"
                                       " coll_inheritance, coll_type, coll_info1, coll_info2, coll_expiry_ts, r_comment,"
                                       " create_ts, modify_ts) "
                                       "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else {
                // TODO Handle error.
            }

            // FIXME Until the unixODBC or Postgres driver is fixed, this "for" statement must
            // be placed above the nanodbc::prepare call. See the following issue for more information:
            //
            //     https://github.com/nanodbc/nanodbc/issues/56
            //
            for (auto&& row : _op.at(jp_values)) {
#else
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

            for (auto&& row : _op.at(jp_values)) {
                nanodbc::prepare(stmt, sql.data());
#endif
                const auto parent_coll_name = row.at("parent_coll_name").get<std::string>();    // Cannot be null
                const auto coll_name = row.at("coll_name").get<std::string>();                  // Cannot be null
                const auto coll_owner_name = row.at("coll_owner_name").get<std::string>();      // Cannot be null
                const auto coll_owner_zone = row.at("coll_owner_zone").get<std::string>();      // Cannot be null
                const auto coll_map_id = row.value("coll_map_id", 0);                           // Cannot be null
                const auto coll_inheritance = row.value("coll_inheritance", "");
                const auto coll_type = row.value("coll_type", "");
                const auto coll_info_1 = row.value("coll_info1", "");
                const auto coll_info_2 = row.value("coll_info2", "");
                const auto coll_expiry_ts = row.value("coll_expiry_ts", "");
                const auto r_comment = row.value("r_comment", "");
                const auto create_ts = row.value("create_ts", timestamp);
                const auto modify_ts = row.value("modify_ts", timestamp);

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
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_collection

    auto insert_replica(nanodbc::connection& _db_conn,
                        const std::string_view _db_instance_name,
                        const json& _op) -> int
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

        rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
#if 0
            if (_db_instance_name == "postgres") {
                nanodbc::prepare(stmt, "insert into R_DATA_MAIN ("
                                       " data_id, coll_id, data_name, data_repl_num, data_version, data_type_name," 
                                       " data_size, resc_group_name, resc_name, data_path, data_owner_name,"
                                       " data_owner_zone, data_is_dirty, data_status, data_checksum, data_expiry_ts,"
                                       " data_map_id, data_mode, r_comment, create_ts, modify_ts, resc_hier, resc_id) "
                                       "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else if (_db_instance_name == "oracle") {
                nanodbc::prepare(stmt, "insert into R_DATA_MAIN ("
                                       " data_id, coll_id, data_name, data_repl_num, data_version, data_type_name," 
                                       " data_size, resc_group_name, resc_name, data_path, data_owner_name,"
                                       " data_owner_zone, data_is_dirty, data_status, data_checksum, data_expiry_ts,"
                                       " data_map_id, data_mode, r_comment, create_ts, modify_ts, resc_hier, resc_id) "
                                       "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else if (_db_instance_name == "mysql") {
                nanodbc::prepare(stmt, "insert into R_DATA_MAIN ("
                                       " data_id, coll_id, data_name, data_repl_num, data_version, data_type_name," 
                                       " data_size, resc_group_name, resc_name, data_path, data_owner_name,"
                                       " data_owner_zone, data_is_dirty, data_status, data_checksum, data_expiry_ts,"
                                       " data_map_id, data_mode, r_comment, create_ts, modify_ts, resc_hier, resc_id) "
                                       "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else {
                // TODO Handle error.
            }

            // FIXME Until the unixODBC or Postgres driver is fixed, this "for" statement must
            // be placed above the nanodbc::prepare call. See the following issue for more information:
            //
            //     https://github.com/nanodbc/nanodbc/issues/56
            //
            for (auto&& row : _op.at(jp_values)) {
#else
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

            for (auto&& row : _op.at(jp_values)) {
                nanodbc::prepare(stmt, sql.data());
#endif
                const auto coll_id = row.at("coll_id").get<int>();                          // Cannot be null
                const auto data_name = row.at("data_name").get<std::string>();              // Cannot be null
                const auto data_repl_num = row.at("data_repl_num").get<int>();              // Cannot be null
                const auto data_version = row.value("data_version", "0");
                const auto data_type_name = row.at("data_type_name").get<std::string>();    // Cannot be null
                const auto data_size = row.value("data_size", 0);                           // Cannot be null
                const auto resc_group_name = row.value("resc_group_name", "EMPTY_RESC_GROUP_NAME");
                const auto resc_name = row.value("resc_name", "EMPTY_RESC_NAME");           // Cannot be null
                const auto data_path = row.at("data_path").get<std::string>();              // Cannot be null
                const auto data_owner_name = row.at("data_owner_name").get<std::string>();  // Cannot be null
                const auto data_owner_zone = row.at("data_owner_zone").get<std::string>();  // Cannot be null
                const auto data_is_dirty = row.value("data_is_dirty", 0);
                const auto data_status = row.value("data_status", "");
                const auto data_checksum = row.value("data_checksum", "");
                const auto data_expiry_ts = row.value("data_expiry_ts", "");
                const auto data_map_id = row.value("data_map_id", 0);
                const auto data_mode = row.value("data_mode", "");
                const auto r_comment = row.value("r_comment", "");
                const auto create_ts = row.value("create_ts", timestamp);
                const auto modify_ts = row.value("modify_ts", timestamp);
                const auto resc_hier = row.value("resc_hier", "EMPTY_RESC_HIER");
                const auto resc_id = row.at("resc_id").get<int>();                          // This is required.

                int i = 0;
                stmt.bind(i, &coll_id);
                stmt.bind(++i, data_name.data());
                stmt.bind(++i, &data_repl_num);
                stmt.bind(++i, data_version.data());
                stmt.bind(++i, data_type_name.data());
                stmt.bind(++i, &data_size);
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
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_replica

    auto insert_metadata(nanodbc::connection& _db_conn,
                         const std::string_view _db_instance_name,
                         const json& _op) -> int
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

        rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
#if 0
            if (_db_instance_name == "postgres") {
                nanodbc::prepare(stmt, "insert into R_META_MAIN ("
                                       " meta_id, meta_namespace, meta_attr_name, meta_attr_value,"
                                       " meta_attr_unit, r_comment, create_ts, modify_ts) "
                                       "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?, ?, ?)");
            }
            else if (_db_instance_name == "oracle") {
                nanodbc::prepare(stmt, "insert into R_META_MAIN ("
                                       " meta_id, meta_namespace, meta_attr_name, meta_attr_value,"
                                       " meta_attr_unit, r_comment, create_ts, modify_ts) "
                                       "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?, ?, ?)");
            }
            else if (_db_instance_name == "mysql") {
                nanodbc::prepare(stmt, "insert into R_META_MAIN ("
                                       " meta_id, meta_namespace, meta_attr_name, meta_attr_value,"
                                       " meta_attr_unit, r_comment, create_ts, modify_ts) "
                                       "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?, ?, ?)");
            }
            else {
                // TODO Handle error.
            }

            // FIXME Until the unixODBC or Postgres driver is fixed, this "for" statement must
            // be placed above the nanodbc::prepare call. See the following issue for more information:
            //
            //     https://github.com/nanodbc/nanodbc/issues/56
            //
            for (auto&& row : _op.at(jp_values)) {
#else
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

            for (auto&& row : _op.at(jp_values)) {
                nanodbc::prepare(stmt, sql.data());
#endif
                const auto meta_namespace = row.value("meta_namespace", "");
                const auto meta_attr_name = row.at("meta_attr_name").get<std::string>();        // Cannot be null
                const auto meta_attr_value = row.at("meta_attr_value").get<std::string>();      // Cannot be null
                const auto meta_attr_unit = row.value("meta_attr_unit", "");
                const auto r_comment = row.value("r_comment", "");
                const auto create_ts = row.value("create_ts", timestamp);
                const auto modify_ts = row.value("modify_ts", timestamp);

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
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }
        return 0;
    } // insert_metadata

    auto insert_metadata_link(nanodbc::connection& _db_conn,
                              const std::string_view _db_instance_name,
                              const json& _op) -> int
    {
        /*
            Column   |         Type          | Modifiers 
          -----------+-----------------------+-----------
           object_id | bigint                | not null
           meta_id   | bigint                | not null
           create_ts | character varying(32) | 
           modify_ts | character varying(32) |
         */

        rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
#if 0
            nanodbc::prepare(stmt, "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) "
                                   "values (?, ?, ?, ?)");

            // FIXME Until the unixODBC or Postgres driver is fixed, this "for" statement must
            // be placed above the nanodbc::prepare call. See the following issue for more information:
            //
            //     https://github.com/nanodbc/nanodbc/issues/56
            //
            for (auto&& row : _op.at(jp_values)) {
#else
            for (auto&& row : _op.at(jp_values)) {
                nanodbc::prepare(stmt, "insert into R_OBJT_METAMAP (object_id, meta_id, create_ts, modify_ts) "
                                       "values (?, ?, ?, ?)");
#endif
                const auto object_id = row.at("object_id").get<int>();  // Cannot be null
                const auto meta_id = row.at("meta_id").get<int>();      // Cannot be null
                const auto create_ts = row.value("create_ts", timestamp);
                const auto modify_ts = row.value("modify_ts", timestamp);

                stmt.bind(0, &object_id);
                stmt.bind(1, &meta_id);
                stmt.bind(2, create_ts.data());
                stmt.bind(3, modify_ts.data());

                nanodbc::execute(stmt);
            }
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_metadata_link

    auto insert_resource(nanodbc::connection& _db_conn,
                         const std::string_view _db_instance_name,
                         const json& _op) -> int
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

        rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto timestamp = current_timestamp();

            nanodbc::statement stmt{_db_conn};
#if 0
            if (_db_instance_name == "postgres") {
                nanodbc::prepare(stmt, "insert into R_RESC_MAIN ("
                                       " resc_id, resc_name, zone_name, resc_type_name, resc_class_name,"
                                       " resc_net, resc_def_path, free_space, free_space_ts, resc_info,"
                                       " r_comment, resc_status, create_ts, modify_ts, resc_children,"
                                       " resc_context, resc_parent, resc_objcount, resc_parent_context) "
                                       "values (nextval('R_OBJECTID'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else if (_db_instance_name == "oracle") {
                nanodbc::prepare(stmt, "insert into R_RESC_MAIN ("
                                       " resc_id, resc_name, zone_name, resc_type_name, resc_class_name,"
                                       " resc_net, resc_def_path, free_space, free_space_ts, resc_info,"
                                       " r_comment, resc_status, create_ts, modify_ts, resc_children,"
                                       " resc_context, resc_parent, resc_objcount, resc_parent_context) "
                                       "values (select R_OBJECTID.nextval from DUAL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else if (_db_instance_name == "mysql") {
                nanodbc::prepare(stmt, "insert into R_RESC_MAIN ("
                                       " resc_id, resc_name, zone_name, resc_type_name, resc_class_name,"
                                       " resc_net, resc_def_path, free_space, free_space_ts, resc_info,"
                                       " r_comment, resc_status, create_ts, modify_ts, resc_children,"
                                       " resc_context, resc_parent, resc_objcount, resc_parent_context) "
                                       "values (R_OBJECTID_nextval(), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            }
            else {
                // TODO Handle error.
            }

            // FIXME Until the unixODBC or Postgres driver is fixed, this "for" statement must
            // be placed above the nanodbc::prepare call. See the following issue for more information:
            //
            //     https://github.com/nanodbc/nanodbc/issues/56
            //
            for (auto&& row : _op.at(jp_values)) {
#else
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

            for (auto&& row : _op.at(jp_values)) {
                nanodbc::prepare(stmt, sql.data());
#endif
                const auto resc_name = row.at("resc_name").get<std::string>();              // Cannot be null
                const auto zone_name = row.at("zone_name").get<std::string>();              // Cannot be null
                const auto resc_type_name = row.at("resc_type_name").get<std::string>();    // Cannot be null
                const auto resc_class_name = row.at("resc_class_name").get<std::string>();  // Cannot be null
                const auto resc_net = row.at("resc_net").get<std::string>();                // Cannot be null
                const auto resc_def_path = row.at("resc_def_path").get<std::string>();      // Cannot be null
                const auto free_space = row.value("free_space", "");
                const auto free_space_ts = row.value("free_space_ts", "");
                const auto resc_info = row.value("resc_info", "");
                const auto r_comment = row.value("r_comment", "");
                const auto resc_status = row.value("resc_status", "");
                const auto create_ts = row.value("create_ts", timestamp);
                const auto modify_ts = row.value("modify_ts", timestamp);
                const auto resc_children = row.value("resc_children", "");
                const auto resc_context = row.value("resc_context", "");
                const auto resc_parent = row.value("resc_parent", "");
                const auto resc_objcount = row.value("resc_objcount", 0);
                const auto resc_parent_context = row.value("resc_parent_context", "");

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
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // insert_metadata_link

    auto delete_rows(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const json& _op) -> int
    {
        rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            const auto& conditions = _op.at(jp_conditions);

            if (conditions.empty()) {
                return SYS_INVALID_INPUT_PARAM;
            }

            auto table_name = _op.at(jp_table).get<std::string>();

            std::vector<std::string> condition_clauses;
            condition_clauses.reserve(conditions.size());

            std::vector<std::string> values;
            values.reserve(conditions.size());

            // Build two lists of strings.
            // - One that holds the SQL condition syntax.
            // - One that holds the values for the prepared statement.
            std::for_each(std::begin(conditions), std::end(conditions),
                [&table_name, &condition_clauses, &values](const json& _c)
                {
                    const auto column = _c.at(jp_column).get<std::string>();
                    throw_if_invalid_table_column(column, table_name);

                    const auto op = _c.at(jp_operator).get<std::string>();
                    throw_if_invalid_conditional_operator(op);

                    condition_clauses.push_back(fmt::format("{} {} ?", column, op));
                    values.push_back(_c.at(jp_value).get<std::string>());
                });

            // Join the conditions together.
            std::ostringstream conditions_stream;
            std::copy(std::begin(condition_clauses),
                      std::end(condition_clauses),
                      std::experimental::make_ostream_joiner(conditions_stream, " and "));

            // Generate the final SQL.
            const auto sql = fmt::format("delete from {} where {}", to_upper(table_name), conditions_stream.str());
            rodsLog(LOG_NOTICE, "%s :: Generated SQL = [%s]", __func__, sql.data());

#ifdef ATOMIC_DB_OPS_RUN_SQL
            nanodbc::statement stmt{_db_conn};
            nanodbc::prepare(stmt, sql);

            // Bind condition values to prepared statement parameter markers.
            for (std::size_t i = 0; i < values.size(); ++i) {
                stmt.bind(i, values[i].data());
            }

            nanodbc::execute(stmt);
#endif
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // delete_rows

    auto update_rows(nanodbc::connection& _db_conn,
                     const std::string_view _db_instance_name,
                     const json& _op) -> int
    {
        rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _op.dump(4).data());

        try {
            auto table_name = _op.at(jp_table).get<std::string>();

            for (auto&& update : _op.at(jp_updates)) {
                const auto& values = update.at(jp_values);

                if (values.empty()) {
                    return SYS_INVALID_INPUT_PARAM;
                }

                const auto& conditions = update.at(jp_conditions);

                if (conditions.empty()) {
                    return SYS_INVALID_INPUT_PARAM;
                }

                std::vector<std::string> set_clauses;
                set_clauses.reserve(values.size());

                std::vector<std::string> bind_args;
                bind_args.reserve(values.size() + conditions.size());

                // Build two lists of strings.
                // - One that holds the SQL set syntax.
                // - One that holds the values for the prepared statement.
                for (auto&& [column, value] : values.items()) {
                    throw_if_invalid_table_column(column, table_name);
                    set_clauses.push_back(fmt::format("{} = ?", column));
                    bind_args.push_back(value.get<std::string>());
                }

                // Join the set operations together.
                std::ostringstream set_stream;
                std::copy(std::begin(set_clauses),
                          std::end(set_clauses),
                          std::experimental::make_ostream_joiner(set_stream, ", "));

                std::vector<std::string> condition_clauses;
                condition_clauses.reserve(conditions.size());

                // Build two lists of strings.
                // - One that holds the SQL condition syntax.
                // - One that holds the values for the prepared statement.
                std::for_each(std::begin(conditions), std::end(conditions),
                    [&table_name, &condition_clauses, &bind_args](const json& _c)
                    {
                        const auto column = _c.at(jp_column).get<std::string>();
                        throw_if_invalid_table_column(column, table_name);

                        const auto op = _c.at(jp_operator).get<std::string>();
                        throw_if_invalid_conditional_operator(op);

                        condition_clauses.push_back(fmt::format("{} {} ?", column, op));
                        bind_args.push_back(_c.at(jp_value).get<std::string>());
                    });

                // Join the conditions together.
                std::ostringstream conditions_stream;
                std::copy(std::begin(condition_clauses),
                          std::end(condition_clauses),
                          std::experimental::make_ostream_joiner(conditions_stream, " and "));

                // Generate the final SQL.
                const auto sql = fmt::format("update {} set {} where {}", to_upper(table_name), set_stream.str(), conditions_stream.str());
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
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, e.what());
        }

        return 0;
    } // update_rows

    auto throw_if_invalid_table_column(const std::string_view _column,
                                       const std::string_view _table_name) -> void
    {
        const auto pred = [target = db_column_type{_table_name, _column}](const db_column_type& _v) noexcept
        {
            return _v == target;
        };

        if (std::none_of(std::begin(db_columns), std::end(db_columns), pred)) {
            const auto msg = fmt::format("Unsupported table column [{1}.{0}].", _column, _table_name);
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
            const auto msg = fmt::format("Unsupported conditional operator [{}].", _operator);
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
        std::transform(std::begin(_s), std::end(_s), std::begin(_s), [](unsigned char _c) { return std::toupper(_c); });
        return _s;
    } // to_upper
} // anonymous namespace

namespace irods::experimental
{
    auto atomic_apply_database_operations(const nlohmann::json& _ops) -> int
    {
        rodsLog(LOG_NOTICE, "%s :: input = %s", __func__, _ops.dump(4).data());

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
            try {
                for (auto&& op : _ops.at(jp_operations)) {
                    const auto op_name = op.at(jp_op_name).get<std::string>();
                    const auto table = op.at(jp_table).get<std::string>();

                    if (auto iter = handlers.find({op_name, table}); iter != std::end(handlers)) {
                        if (const auto ec = (*iter->second)(db_conn, db_instance_name, op); ec != 0) {
                            return ec;
                        }
                    }
                    else {
                        rodsLog(LOG_ERROR, "%s :: Invalid operation [%s].", __func__, op_name.data());
                        return SYS_INVALID_INPUT_PARAM;
                    }
                }

                _trans.commit();

                return 0;
            }
            catch (const json::exception& e) {
                rodsLog(LOG_ERROR, "%s :: Caught exception: %s", __func__, e.what());
                return SYS_INVALID_INPUT_PARAM;
            }
            catch (const irods::exception& e) {
                rodsLog(LOG_ERROR, "%s :: Caught exception: %s", __func__, e.what());
                return e.code();
            }
            catch (const std::exception& e) {
                rodsLog(LOG_ERROR, "%s :: Caught exception: %s", __func__, e.what());
                return SYS_INTERNAL_ERR;
            }
        });
    } // atomic_apply_database_operations

    auto atomic_apply_database_operations(const nlohmann::json& _ops, std::error_code& _ec) -> void
    {
        try {
            const auto ec = atomic_apply_database_operations(_ops);
            _ec.assign(ec, std::generic_category());
        }
        catch (const irods::exception& e) {
            _ec.assign(e.code(), std::generic_category());
        }
        catch (...) {
            _ec.assign(SYS_UNKNOWN_ERROR, std::generic_category());
        }
    } // atomic_apply_database_operations
} // namespace irods::experimental

