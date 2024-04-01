#include "irods/private/genquery2_sql.hpp"

#include "irods/private/genquery2_ast_types.hpp"
#include "irods/private/vertex_property.hpp"
#include "irods/private/edge_property.hpp"
#include "irods/private/genquery2_table_column_mappings.hpp"

#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_logger.hpp"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/iterator/function_input_iterator.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    namespace gq = irods::experimental::genquery2;

    // clang-format off
    using graph_type         = boost::adjacency_list<boost::vecS,
                                                     boost::vecS,
                                                     boost::undirectedS,
                                                     irods::experimental::genquery::vertex_property,
                                                     irods::experimental::genquery::edge_property>;
    using vertex_type        = boost::graph_traits<graph_type>::vertex_descriptor;
    using vertices_size_type = boost::graph_traits<graph_type>::vertices_size_type;
    using edge_type          = std::pair<vertex_type, vertex_type>;

    using log_gq             = irods::experimental::log::genquery2;
    // clang-format on

    struct gq_state
    {
        // Holds pointers to column objects which contain SQL CAST syntax.
        // These pointers allow the parser to forward the SQL CAST text to the final output.
        std::vector<const gq::column*> ast_column_ptrs;

        std::vector<std::string_view> sql_tables;
        std::map<std::string, std::string> table_aliases;
        std::vector<std::string> values;

        int table_alias_id = 0;

        bool in_select_clause = false;

        bool add_joins_for_meta_data = false;
        bool add_joins_for_meta_coll = false;
        bool add_joins_for_meta_resc = false;
        bool add_joins_for_meta_user = false;

        bool add_sql_for_data_resc_hier = false;
    }; // struct gq_state

    // clang-format off
    constexpr auto table_names = std::to_array({
        "R_COLL_MAIN",                  // 0
        "R_DATA_MAIN",                  // 1
        "R_META_MAIN",                  // 2
        "R_OBJT_ACCESS",                // 3
        "R_OBJT_METAMAP",               // 4
        "R_RESC_MAIN",                  // 5
        "R_RULE_EXEC",                  // 6
        "R_SPECIFIC_QUERY",             // 7
        "R_TICKET_ALLOWED_HOSTS",       // 8
        "R_TICKET_ALLOWED_USERS",       // 9
        "R_TICKET_ALLOWED_GROUPS",      // 10
        "R_TICKET_MAIN",                // 11
        "R_TOKN_MAIN",                  // 12
        "R_USER_AUTH",                  // 13
        "R_USER_GROUP",                 // 14
        "R_USER_MAIN",                  // 15
        "R_USER_PASSWORD",              // 16
        "R_USER_SESSION_KEY",           // 17
        "R_ZONE_MAIN",                  // 18
        "R_QUOTA_MAIN",                 // 19
    }); // table_names
    // clang-format on

    constexpr auto to_index(const std::string_view _table_name) -> std::size_t
    {
        for (auto i = 0ULL; i < table_names.size(); ++i) {
            if (table_names.at(i) == _table_name) {
                return i;
            }
        }

        throw std::invalid_argument{fmt::format("table [{}] not supported", _table_name)};
    } // to_index

    // clang-format off
    constexpr auto table_edges = std::to_array<edge_type>({
        {to_index("R_COLL_MAIN"), to_index("R_DATA_MAIN")},                 // R_COLL_MAIN.coll_id = R_DATA_MAIN.coll_id
        {to_index("R_COLL_MAIN"), to_index("R_OBJT_ACCESS")},               // R_COLL_MAIN.coll_id = R_OBJT_ACCESS.object_id
        {to_index("R_COLL_MAIN"), to_index("R_OBJT_METAMAP")},              // R_COLL_MAIN.coll_id = R_OBJT_METAMAP.object_id
        {to_index("R_COLL_MAIN"), to_index("R_TICKET_MAIN")},               // R_COLL_MAIN.coll_id = R_TICKET_MAIN.object_id

        {to_index("R_DATA_MAIN"), to_index("R_OBJT_ACCESS")},               // R_DATA_MAIN.data_id = R_OBJT_ACCESS.object_id
        {to_index("R_DATA_MAIN"), to_index("R_OBJT_METAMAP")},              // R_DATA_MAIN.data_id = R_OBJT_METAMAP.object_id
        {to_index("R_DATA_MAIN"), to_index("R_RESC_MAIN")},                 // R_DATA_MAIN.resc_id = R_RESC_MAIN.resc_id
        {to_index("R_DATA_MAIN"), to_index("R_TICKET_MAIN")},               // R_DATA_MAIN.data_id = R_TICKET_MAIN.object_id

        {to_index("R_META_MAIN"), to_index("R_OBJT_METAMAP")},              // R_META_MAIN.meta_id = R_OBJT_METAMAP.meta_id

        {to_index("R_OBJT_ACCESS"), to_index("R_TOKN_MAIN")},               // R_OBJT_ACCESS.access_type_id = R_TOKN_MAIN.token_id

        {to_index("R_OBJT_METAMAP"), to_index("R_RESC_MAIN")},              // R_OBJT_METAMAP.object_id = R_RESC_MAIN.resc_id
        {to_index("R_OBJT_METAMAP"), to_index("R_USER_MAIN")},              // R_OBJT_METAMAP.object_id = R_USER_MAIN.user_id

        {to_index("R_TICKET_MAIN"), to_index("R_USER_MAIN")},               // R_TICKET_MAIN.user_id = R_USER_MAIN.user_id

        {to_index("R_TICKET_MAIN"), to_index("R_TICKET_ALLOWED_HOSTS")},    // R_TICKET_MAIN.ticket_id = R_TICKET_ALLOWED_HOSTS.ticket_id
        {to_index("R_TICKET_MAIN"), to_index("R_TICKET_ALLOWED_USERS")},    // R_TICKET_MAIN.ticket_id = R_TICKET_ALLOWED_USERS.ticket_id
        {to_index("R_TICKET_MAIN"), to_index("R_TICKET_ALLOWED_GROUPS")},   // R_TICKET_MAIN.ticket_id = R_TICKET_ALLOWED_GROUPS.ticket_id

        {to_index("R_USER_MAIN"), to_index("R_USER_AUTH")},                 // R_USER_MAIN.user_id = R_USER_AUTH.user_id
        {to_index("R_USER_MAIN"), to_index("R_USER_GROUP")},                // R_USER_MAIN.user_id = R_USER_GROUP.group_user_id
        {to_index("R_USER_MAIN"), to_index("R_USER_PASSWORD")},             // R_USER_MAIN.user_id = R_USER_PASSWORD.user_id
        {to_index("R_USER_MAIN"), to_index("R_USER_SESSION_KEY")},          // R_USER_MAIN.user_id = R_USER_SESSION_KEY.user_id

        // Handle R_USER_GROUP?
        // Handle R_QUOTA_MAIN
        // Handle R_QUOTA_USAGE
    }); // table_edges
    // clang-format on

    // clang-format off
    constexpr auto table_joins = std::to_array<irods::experimental::genquery::edge_property>({
        {"{}.coll_id = {}.coll_id", 0},
        {"{}.coll_id = {}.object_id", 1},
        {"{}.coll_id = {}.object_id", 2},
        {"{}.coll_id = {}.object_id", 3},

        {"{}.data_id = {}.object_id", 4},
        {"{}.data_id = {}.object_id", 5},
        {"{}.resc_id = {}.resc_id", 6},
        {"{}.data_id = {}.object_id", 7},

        {"{}.meta_id = {}.meta_id", 8},

        {"{}.access_type_id = {}.token_id", 9},

        {"{}.object_id = {}.resc_id", 10},
        {"{}.object_id = {}.user_id", 11},

        {"{}.user_id = {}.user_id", 12},

        {"{}.ticket_id = {}.ticket_id", 13},
        {"{}.ticket_id = {}.ticket_id", 14},
        {"{}.ticket_id = {}.ticket_id", 15},

        {"{}.user_id = {}.user_id", 16},
        {"{}.user_id = {}.group_user_id", 17},
        {"{}.user_id = {}.user_id", 18},
        {"{}.user_id = {}.user_id", 19},
    }); // table_joins
    // clang-format on

    auto generate_table_alias(gq_state& _state) -> std::string
    {
        return fmt::format("t{}", _state.table_alias_id++);
    } // generate_table_alias

    struct sql_visitor : public boost::static_visitor<std::string>
    {
        explicit sql_visitor(gq_state& _state)
            : state{&_state}
        {
        }

        template <typename T>
        auto operator()(const T& _arg) const -> std::string
        {
            return to_sql(*state, _arg);
        }

        gq_state* state; // NOLINT(misc-non-private-member-variables-in-classes)
    }; // struct sql_visitor

    template <typename Container, typename Value>
    constexpr bool contains(const Container& _container, const Value& _value)
    {
        return std::find(std::begin(_container), std::end(_container), _value) != std::end(_container);
    } // contains

    auto init_graph() -> graph_type
    {
        graph_type graph{table_edges.data(), table_edges.data() + table_edges.size(), table_names.size()};

        // Attach table names to table vertices.
        for (auto [iter, last] = boost::vertices(graph); iter != last; ++iter) {
            graph[*iter].table_name = table_names[*iter]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        // Attach the table joins to each edge.
        for (auto [iter, last] = boost::edges(graph); iter != last; ++iter) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            graph[*iter] = table_joins[table_edges.size() - std::distance(iter, last)];
        }

        return graph;
    } // init_graph

    auto generate_inner_joins(const graph_type& _graph,
                              const std::vector<std::string_view>& _tables,
                              const std::map<std::string, std::string>& _table_aliases) -> std::vector<std::string>
    {
        const auto get_table_join = [&_graph, &_table_aliases](const auto& _t1, const auto& _t2) -> std::string {
            auto t1_idx = to_index(_t1);
            auto t2_idx = to_index(_t2);
            const auto [edge, exists] = boost::edge(t1_idx, t2_idx, _graph);

            if (!exists) {
                return {};
            }

            std::string_view t2_alias = _table_aliases.at(std::string{_t2});
            const auto sql = fmt::format("inner join {} {} on {}", _t2, t2_alias, _graph[edge].join_condition);

            // It is likely that the order of the tables passed do NOT match the order of the join expression.
            // To resolve this, each edge property contains its position in the table_edges array. This is used
            // to lookup the edge definition in table_edges. This allows the parser to reorder the tables to
            // satisfy the table join expression.
            const auto& edge_def =
                table_edges[_graph[edge].position]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            auto& t1_alias = _table_aliases.at(
                table_names[edge_def.first]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            t2_alias = _table_aliases.at(
                table_names[edge_def.second]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

            return fmt::format(fmt::runtime(sql), t1_alias, t2_alias);
        };

        // The order of inner joins may or may not matter depending on the design of your SQL database.
        // There are cases in which the order of the inner joins matter in iRODS (e.g. permissions).
        // The following algorithm resolves the joins by scanning the list of tables multiple times.
        // The list of tables is obtained during parsing of the string.

        std::vector<std::string> inner_joins;
        inner_joins.reserve(_tables.size() - 1);

        // Copy all entries from "_tables" into the list except the very first one.
        // The first element is the table we are trying to join to. So, we consider that one handled.
        std::vector<std::string> remaining{std::begin(_tables) + 1, std::end(_tables)};
        log_gq::debug(fmt::format("remaining = [{}]", fmt::join(remaining, ", ")));

        std::vector<std::string> processed;
        processed.reserve(_tables.size());
        processed.emplace_back(std::string{_tables.front()});
        log_gq::debug(fmt::format("processed = [{}]", fmt::join(processed, ", ")));

        for (decltype(_tables.size()) i = 0; i < _tables.size() - 1; ++i) {
            const auto& last = processed.back();

            for (auto iter = std::begin(remaining); iter != std::end(remaining);) {
                if (auto j = get_table_join(last, *iter); !j.empty()) {
                    inner_joins.push_back(std::move(j));
                    processed.emplace_back(*iter);
                    iter = remaining.erase(iter);
                }
                else {
                    ++iter;
                }
            }
        }

        return inner_joins;
    } // generate_inner_joins

    auto generate_joins_for_metadata_columns(const gq_state& _state) -> std::string
    {
        // Below is an example which shows how the metadata tables must be joined in order to allow mixed
        // entity searches (i.e queries which include criteria for data objects, collections, etc).
        //
        //      select distinct d.data_id, c.coll_name, d.data_name, mmd.meta_attr_name, mmc.meta_attr_name
        //      from R_COLL_MAIN c
        //      inner join R_DATA_MAIN d on c.coll_id = d.coll_id
        //      left join R_OBJT_METAMAP ommd on d.data_id = ommd.object_id
        //      left join R_OBJT_METAMAP ommc on c.coll_id = ommc.object_id
        //      left join R_META_MAIN mmd on ommd.meta_id = mmd.meta_id
        //      left join R_META_MAIN mmc on ommc.meta_id = mmc.meta_id
        //      where mmd.meta_attr_name = 'job' or
        //            mmc.meta_attr_name = 'nope';

        std::string sql;

        if (_state.add_joins_for_meta_data) {
            sql += fmt::format(" left join R_OBJT_METAMAP ommd on {}.data_id = ommd.object_id "
                               "left join R_META_MAIN mmd on ommd.meta_id = mmd.meta_id",
                               _state.table_aliases.at("R_DATA_MAIN"));
        }

        if (_state.add_joins_for_meta_coll) {
            sql += fmt::format(" left join R_OBJT_METAMAP ommc on {}.coll_id = ommc.object_id "
                               "left join R_META_MAIN mmc on ommc.meta_id = mmc.meta_id",
                               _state.table_aliases.at("R_COLL_MAIN"));
        }

        if (_state.add_joins_for_meta_resc) {
            sql += fmt::format(" left join R_OBJT_METAMAP ommr on {}.resc_id = ommr.object_id "
                               "left join R_META_MAIN mmr on ommr.meta_id = mmr.meta_id",
                               _state.table_aliases.at("R_RESC_MAIN"));
        }

        if (_state.add_joins_for_meta_user) {
            sql += fmt::format(" left join R_OBJT_METAMAP ommu on {}.user_id = ommu.object_id "
                               "left join R_META_MAIN mmu on ommu.meta_id = mmu.meta_id",
                               _state.table_aliases.at("R_USER_MAIN"));
        }

        return sql;
    } // generate_joins_for_metadata_columns

    auto generate_joins_for_permissions(const std::map<std::string, std::string>& _table_aliases) -> std::string
    {
        // Always include the joins if the query involves columns related to data objects and/or collections.
        // This is required due to how columns in R_OBJT_ACCESS and other tables are handled.
        //
        //  select d.*
        //  from R_DATA_MAIN d
        //  inner join R_COLL_MAIN c on doa.object_id = d.data_id
        //  inner join R_OBJT_ACCESS doa on doa.object_id = d.data_id
        //  inner join R_OBJT_ACCESS coa on coa.object_id = c.coll_id
        //  inner join R_USER_MAIN du on du.user_id = doa.user_id
        //  inner join R_USER_MAIN cu on cu.user_id = coa.user_id
        //  where doa.access_type_id >= ? and
        //        coa.access_type_id >= ?
        //
        std::string sql;

        if (const auto iter = _table_aliases.find("R_DATA_MAIN"); iter != std::end(_table_aliases)) {
            sql += fmt::format(" inner join R_OBJT_ACCESS pdoa on {}.data_id = pdoa.object_id"
                               " inner join R_TOKN_MAIN pdt on pdoa.access_type_id = pdt.token_id"
                               " inner join R_USER_MAIN pdu on pdoa.user_id = pdu.user_id",
                               iter->second);
        }

        if (const auto iter = _table_aliases.find("R_COLL_MAIN"); iter != std::end(_table_aliases)) {
            sql += fmt::format(" inner join R_OBJT_ACCESS pcoa on {}.coll_id = pcoa.object_id"
                               " inner join R_TOKN_MAIN pct on pcoa.access_type_id = pct.token_id"
                               " inner join R_USER_MAIN pcu on pcoa.user_id = pcu.user_id",
                               iter->second);
        }

        return sql;
    } // generate_joins_for_permissions

    auto generate_condition_clause(gq_state& _state, const gq::options& _opts, const std::string& _conditions)
        -> std::string
    {
        // Below is an example showing the correct SQL for permission checking when the user is identified
        // as a rodsadmin.
        //
        //      select d.*
        //      from R_DATA_MAIN d
        //      inner join R_COLL_MAIN c on doa.object_id = d.data_id
        //      inner join R_OBJT_ACCESS doa on doa.object_id = d.data_id
        //      inner join R_OBJT_ACCESS coa on coa.object_id = c.coll_id
        //      inner join R_USER_MAIN du on du.user_id = doa.user_id
        //      inner join R_USER_MAIN cu on cu.user_id = coa.user_id
        //      where doa.access_type_id >= ? and
        //            coa.access_type_id >= ?
        //
        // When the user is NOT a rodsadmin, the SQL must include an additional condition which restricts
        // the user's visibility to those data objects and collections which they have permission to view.
        //
        // For example:
        //
        //      select d.*
        //      from R_DATA_MAIN d
        //      inner join R_COLL_MAIN c on doa.object_id = d.data_id
        //      inner join R_OBJT_ACCESS doa on doa.object_id = d.data_id
        //      inner join R_OBJT_ACCESS coa on coa.object_id = c.coll_id
        //      inner join R_USER_MAIN du on du.user_id = doa.user_id
        //      inner join R_USER_MAIN cu on cu.user_id = coa.user_id
        //      where doa.access_type_id >= ? and du.user_name = ? and du.zone_name = ? and
        //            coa.access_type_id >= ? and cu.user_name = ? and cu.zone_name = ?

        std::string sql;

        // The permission value for "null" is 1000. Therefore, if the user is a rodsadmin, they are
        // allowed to view all objects. We achieve this by setting the minimum permission level to the
        // value "null". Remember, the permission model in iRODS is linear.
        const auto min_perm_level = _opts.admin_mode ? 1000 : 1050;

        const auto d_iter = _state.table_aliases.find("R_DATA_MAIN");
        const auto c_iter = _state.table_aliases.find("R_COLL_MAIN");
        const auto end = std::end(_state.table_aliases);

        // In this implementation, the following table aliases exist.
        //
        // For data objects:
        // - pdoa: R_OBJT_ACCESS
        // - pdu : R_USER_MAIN
        //
        // For collections:
        // - pcoa: R_OBJT_ACCESS
        // - pcu : R_USER_MAIN

        if (!_conditions.empty()) {
            sql += fmt::format(" where {}", _conditions);

            if (_opts.admin_mode) {
                if (d_iter != end && c_iter != end) {
                    sql += fmt::format(" and pdoa.access_type_id >= {perm} and pcoa.access_type_id >= {perm}",
                                       fmt::arg("perm", min_perm_level));
                }
                else if (d_iter != end) {
                    sql += fmt::format(" and pdoa.access_type_id >= {}", min_perm_level);
                }
                else if (c_iter != end) {
                    sql += fmt::format(" and pcoa.access_type_id >= {}", min_perm_level);
                }
            }
            else {
                if (d_iter != end && c_iter != end) {
                    sql += fmt::format(" and pdu.user_name = ? and pdu.zone_name = '{zone}'"
                                       " and pcu.user_name = ? and pcu.zone_name = '{zone}'"
                                       " and pdoa.access_type_id >= {perm} and pcoa.access_type_id >= {perm}",
                                       fmt::arg("perm", min_perm_level),
                                       fmt::arg("zone", _opts.user_zone));
                    _state.values.emplace_back(std::string{_opts.user_name});
                    _state.values.emplace_back(std::string{_opts.user_name});
                }
                else if (d_iter != end) {
                    sql += fmt::format(" and pdu.user_name = ? and pdu.zone_name = '{}' and pdoa.access_type_id >= {}",
                                       _opts.user_zone,
                                       min_perm_level);
                    _state.values.emplace_back(std::string{_opts.user_name});
                }
                else if (c_iter != end) {
                    sql += fmt::format(" and pcu.user_name = ? and pcu.zone_name = '{}' and pcoa.access_type_id >= {}",
                                       _opts.user_zone,
                                       min_perm_level);
                    _state.values.emplace_back(std::string{_opts.user_name});
                }
            }

            return sql;
        }

        //
        // At this point, the user did not include any conditions.
        //

        if (_opts.admin_mode) {
            if (d_iter != end && c_iter != end) {
                sql += fmt::format(" where pdoa.access_type_id >= {perm} and pcoa.access_type_id >= {perm}",
                                   fmt::arg("perm", min_perm_level));
            }
            else if (d_iter != end) {
                sql += fmt::format(" where pdoa.access_type_id >= {}", min_perm_level);
            }
            else if (c_iter != end) {
                sql += fmt::format(" where pcoa.access_type_id >= {}", min_perm_level);
            }
        }
        else {
            if (d_iter != end && c_iter != end) {
                sql += fmt::format(" where pdu.user_name = ? and pdu.zone_name = '{zone}'"
                                   " and pcu.user_name = ? and pcu.zone_name = '{zone}'"
                                   " and pdoa.access_type_id >= {perm} and pcoa.access_type_id >= {perm}",
                                   fmt::arg("perm", min_perm_level),
                                   fmt::arg("zone", _opts.user_zone));
                _state.values.emplace_back(std::string{_opts.user_name});
                _state.values.emplace_back(std::string{_opts.user_name});
            }
            else if (d_iter != end) {
                sql += fmt::format(" where pdu.user_name = ? and pdu.zone_name = '{}' and pdoa.access_type_id >= {}",
                                   _opts.user_zone,
                                   min_perm_level);
                _state.values.emplace_back(std::string{_opts.user_name});
            }
            else if (c_iter != end) {
                sql += fmt::format(" where pcu.user_name = ? and pcu.zone_name = '{}' and pcoa.access_type_id >= {}",
                                   _opts.user_zone,
                                   min_perm_level);
                _state.values.emplace_back(std::string{_opts.user_name});
            }
        }

        return sql;
    } // generate_condition_clause

    auto generate_group_by_clause(const gq_state& _state,
                                  const gq::group_by& _group_by,
                                  const std::map<std::string_view, gq::column_info>& _column_name_mappings)
        -> std::string
    {
        if (_group_by.columns.empty()) {
            return {};
        }

        std::vector<std::string> resolved_columns;
        resolved_columns.reserve(_group_by.columns.size());

        for (const auto& c : _group_by.columns) {
            const auto iter = _column_name_mappings.find(c);

            if (iter == std::end(_column_name_mappings)) {
                throw std::invalid_argument{fmt::format("unknown column in group-by clause: {}", c)};
            }

            auto is_special_column = true;
            std::string_view alias;

            // clang-format off
            if      (c.starts_with("META_D")) { alias = "mmd"; }
            else if (c.starts_with("META_C")) { alias = "mmc"; }
            else if (c.starts_with("META_R")) { alias = "mmr"; }
            else if (c.starts_with("META_U")) { alias = "mmu"; }
            else if (c == "DATA_RESC_HIER")   { alias = "cte_drh"; }
            else                              { is_special_column = false; }
            // clang-format on

            if (!is_special_column) {
                alias = _state.table_aliases.at(std::string{iter->second.table});
            }

            const auto ast_iter =
                std::find_if(std::begin(_state.ast_column_ptrs),
                             std::end(_state.ast_column_ptrs),
                             [&c](const irods::experimental::genquery2::column* _p) { return _p->name == c; });

            if (std::end(_state.ast_column_ptrs) == ast_iter) {
                throw std::invalid_argument{"cannot generate SQL from General Query."};
            }

            if ((*ast_iter)->type_name.empty()) {
                resolved_columns.push_back(fmt::format("{}.{}", alias, iter->second.name));
            }
            else {
                resolved_columns.push_back(
                    fmt::format("cast({}.{} as {})", alias, iter->second.name, (*ast_iter)->type_name));
            }
        }

        // All columns in the group-by clause must exist in the list of columns to project.
        return fmt::format(" group by {}", fmt::join(resolved_columns, ", "));
    } // generate_group_by_clause

    auto generate_order_by_clause(const gq_state& _state,
                                  const gq::order_by& _order_by,
                                  const std::map<std::string_view, gq::column_info>& _column_name_mappings)
        -> std::string
    {
        if (_order_by.sort_expressions.empty()) {
            return {};
        }

        const auto& sort_expressions = _order_by.sort_expressions;

        std::vector<std::string> sort_expr;
        sort_expr.reserve(sort_expressions.size());

        for (const auto& se : sort_expressions) {
            const auto iter = _column_name_mappings.find(se.column);

            if (iter == std::end(_column_name_mappings)) {
                throw std::invalid_argument{fmt::format("unknown column in order-by clause: {}", se.column)};
            }

            auto is_special_column = true;
            std::string_view alias;

            // clang-format off
            if      (se.column.starts_with("META_D")) { alias = "mmd"; }
            else if (se.column.starts_with("META_C")) { alias = "mmc"; }
            else if (se.column.starts_with("META_R")) { alias = "mmr"; }
            else if (se.column.starts_with("META_U")) { alias = "mmu"; }
            else if (se.column == "DATA_RESC_HIER")   { alias = "cte_drh"; }
            else                                      { is_special_column = false; }
            // clang-format on

            if (!is_special_column) {
                alias = _state.table_aliases.at(std::string{iter->second.table});
            }

            const auto ast_iter =
                std::find_if(std::begin(_state.ast_column_ptrs),
                             std::end(_state.ast_column_ptrs),
                             [&se](const irods::experimental::genquery2::column* _p) { return _p->name == se.column; });

            if (std::end(_state.ast_column_ptrs) == ast_iter) {
                throw std::invalid_argument{"cannot generate SQL from General Query."};
            }

            if ((*ast_iter)->type_name.empty()) {
                sort_expr.push_back(
                    fmt::format("{}.{} {}", alias, iter->second.name, se.ascending_order ? "asc" : "desc"));
            }
            else {
                sort_expr.push_back(fmt::format("cast({}.{} as {}) {}",
                                                alias,
                                                iter->second.name,
                                                (*ast_iter)->type_name,
                                                se.ascending_order ? "asc" : "desc"));
            }
        }

        // All columns in the order by clause must exist in the list of columns to project.
        return fmt::format(" order by {}", fmt::join(sort_expr, ", "));
    } // generate_order_by_clause

    auto generate_with_clause_for_data_resc_hier(const gq_state& _state, const std::string_view _database)
        -> std::string
    {
        if (!_state.add_sql_for_data_resc_hier) {
            return "";
        }

        // clang-format off
        //
        // The following SQL is a recursive WITH clause which produces all resource hierarchies
        // in the database upon request.
        //
        // The following query will produce a resource hierarchy starting from a leaf resource ID.
        // Keep in mind that the ::<type> syntax may be specific to PostgreSQL. Remember to check the
        // other database systems for compatibility.
        //
        //     with recursive T as (
        //         select
        //             resc_id,
        //             resc_name hier,
        //             case
        //                 when resc_parent = '' then 0
        //                 else resc_parent::bigint
        //             end parent_id
        //         from
        //             r_resc_main
        //         where
        //             resc_id > 0     -- Or, resc_id = <child_resc_id>
        //
        //         union all
        //
        //         select
        //             T.resc_id,
        //             (U.resc_name || ';' || T.hier)::varchar(250),
        //             case
        //                 when U.resc_parent = '' then 0
        //                 else U.resc_parent::bigint
        //             end parent_id
        //         from T
        //         inner join r_resc_main U on U.resc_id = T.parent_id
        //     )
        //     select resc_id, hier from T where parent_id = 0 and resc_id = <resc_id>;
        //
        // Q. Can this be used with other queries?
        // A. Yes! CTEs can be joined just like any other table.
        //
        // Q. What tables need to be joined in order to support this?
        // A. R_RESC_MAIN is the only table that is needed.
        constexpr const char* data_resc_hier_with_clause =
            "with{recursive_op} cte_drh as ("
                "select "
                    "resc_id, "
                    "resc_name hier, "
                    "case "
                        "when resc_parent = '' then 0 "
                        "else cast(resc_parent as {int_type}) "
                    "end parent_id "
                "from R_RESC_MAIN "
                "where resc_id > 0 "
                "union all "
                "select "
                    "cte_drh.resc_id, "
                    "cast(concat(concat(U.resc_name, ';'), cte_drh.hier) as {char_type}(250)), "
                    "case "
                        "when U.resc_parent = '' then 0 "
                        "else cast(U.resc_parent as {int_type}) "
                    "end parent_id "
                "from cte_drh "
                "inner join R_RESC_MAIN U on U.resc_id = cte_drh.parent_id"
            ") ";
        // clang-format on

        // See https://modern-sql.com/caniuse/cast_as_bigint to understand why the data types
        // for MySQL and Oracle were chosen.

        if (_database == "mysql") {
            return fmt::format(data_resc_hier_with_clause,
                               fmt::arg("recursive_op", " recursive"),
                               fmt::arg("int_type", "signed"),
                               fmt::arg("char_type", "char"));
        }

        if (_database == "oracle") {
            return fmt::format(data_resc_hier_with_clause,
                               fmt::arg("recursive_op", ""),
                               fmt::arg("int_type", "integer"),
                               fmt::arg("char_type", "varchar"));
        }

        // Assume the database supports the BIGINT data type.
        // This is the preferred data type, is fully supported by PostgreSQL, and
        // is defined in ISO/IEC 9075:2016-2.
        return fmt::format(data_resc_hier_with_clause,
                           fmt::arg("recursive_op", " recursive"),
                           fmt::arg("int_type", "bigint"),
                           fmt::arg("char_type", "varchar"));
    } // generate_with_clause_for_data_resc_hier

    auto generate_limit_clause(const irods::experimental::genquery2::options& _opts,
                               const std::string_view _number_of_rows) -> std::string
    {
        if (!_number_of_rows.empty()) {
            if (_opts.database == "mysql") {
                return fmt::format(" limit {}", _number_of_rows);
            }

            return fmt::format(" fetch first {} rows only", _number_of_rows);
        }

        if (_opts.database == "mysql") {
            return fmt::format(" limit {}", _opts.default_number_of_rows);
        }

        return fmt::format(" fetch first {} rows only", _opts.default_number_of_rows);
    } // generate_limit_clause
} // anonymous namespace

namespace irods::experimental::genquery2
{
    auto setup_column_for_post_processing(gq_state& _state, const column& _column, const column_info& _column_info)
        -> std::tuple<bool, std::string_view>
    {
        auto is_special_column = true;
        std::string_view table_alias;

        auto add_r_data_main = false;
        auto add_r_coll_main = false;
        auto add_r_user_main = false;
        auto add_r_resc_main = false;

        // The columns that trigger these branches rely on special joins and therefore must use pre-defined
        // table aliases.
        if (_column.name.starts_with("META_D")) {
            add_r_data_main = _state.add_joins_for_meta_data = true;
            table_alias = "mmd";
        }
        else if (_column.name.starts_with("META_C")) {
            add_r_coll_main = _state.add_joins_for_meta_coll = true;
            table_alias = "mmc";
        }
        else if (_column.name.starts_with("META_R")) {
            add_r_resc_main = _state.add_joins_for_meta_resc = true;
            table_alias = "mmr";
        }
        else if (_column.name.starts_with("META_U")) {
            add_r_user_main = _state.add_joins_for_meta_user = true;
            table_alias = "mmu";
        }
        else if (_column.name.starts_with("DATA_ACCESS_")) {
            // There are three tables which are secretly joined to satisfy columns that trigger this branch.
            // The columns require special hard-coded table aliases to work properly. This is because the joins
            // cannot be worked out using only the graph.
            if (_column.name == "DATA_ACCESS_PERM_NAME") {
                add_r_data_main = true;
                table_alias = "pdt"; // The alias for R_TOKN_MAIN as it relates to data objects.
            }
            else if (_column.name == "DATA_ACCESS_USER_NAME") {
                add_r_data_main = true;
                table_alias = "pdu"; // The alias for R_USER_MAIN as it relates to data objects.
            }
            else {
                add_r_data_main = true;
                table_alias = "pdoa"; // The alias for R_OBJT_ACCESS as it relates to data objects.
            }
        }
        else if (_column.name.starts_with("COLL_ACCESS_")) {
            // There are three tables which are secretly joined to satisfy columns that trigger this branch.
            // The columns require special hard-coded table aliases to work properly. This is because the joins
            // cannot be worked out using only the graph.
            if (_column.name == "COLL_ACCESS_PERM_NAME") {
                add_r_coll_main = true;
                table_alias = "pct"; // The alias for R_TOKN_MAIN as it relates to collections.
            }
            else if (_column.name == "COLL_ACCESS_USER_NAME") {
                add_r_coll_main = true;
                table_alias = "pcu"; // The alias for R_USER_MAIN as it relates to collections.
            }
            else {
                add_r_coll_main = true;
                table_alias = "pcoa"; // The alias for R_OBJT_ACCESS as it relates to collections.
            }
        }
        else if (_column.name == "DATA_RESC_HIER") {
            add_r_resc_main = _state.add_sql_for_data_resc_hier = true;
            table_alias = "cte_drh";
        }
        else {
            is_special_column = false;
        }

        if (!contains(_state.sql_tables, _column_info.table)) {
            // Special columns such as the general query metadata columns are handled separately
            // because they require multiple table joins. For this reason, we don't allow any of those
            // tables to be added to the table list.
            if (is_special_column) {
                if (add_r_data_main) {
                    if (!contains(_state.sql_tables, "R_DATA_MAIN")) {
                        _state.sql_tables.emplace_back("R_DATA_MAIN");
                        _state.table_aliases["R_DATA_MAIN"] = generate_table_alias(_state);
                    }
                }
                else if (add_r_coll_main) {
                    if (!contains(_state.sql_tables, "R_COLL_MAIN")) {
                        _state.sql_tables.emplace_back("R_COLL_MAIN");
                        _state.table_aliases["R_COLL_MAIN"] = generate_table_alias(_state);
                    }
                }
                else if (add_r_resc_main) {
                    if (!contains(_state.sql_tables, "R_RESC_MAIN")) {
                        _state.sql_tables.emplace_back("R_RESC_MAIN");
                        _state.table_aliases["R_RESC_MAIN"] = generate_table_alias(_state);
                    }
                }
                else if (add_r_user_main) {
                    if (!contains(_state.sql_tables, "R_USER_MAIN")) {
                        _state.sql_tables.emplace_back("R_USER_MAIN");
                        _state.table_aliases["R_USER_MAIN"] = generate_table_alias(_state);
                    }
                }
            }
            else {
                _state.sql_tables.push_back(_column_info.table);
                _state.table_aliases[std::string{_column_info.table}] = generate_table_alias(_state);
            }
        }

        return {is_special_column, table_alias};
    } // setup_column_for_post_processing

    auto to_sql(gq_state& _state, const column& _column) -> std::string
    {
        const auto iter = column_name_mappings.find(_column.name);

        if (iter == std::end(column_name_mappings)) {
            throw std::invalid_argument{fmt::format("unknown column: {}", _column.name)};
        }

        // Capture all column objects as some parts of the implementation need to access them in
        // order to generate the proper SQL.
        _state.ast_column_ptrs.push_back(&_column);

        auto [is_special_column, table_alias] = setup_column_for_post_processing(_state, _column, iter->second);
        const std::string_view alias =
            is_special_column ? table_alias : _state.table_aliases.at(std::string{iter->second.table});

        if (_column.type_name.empty()) {
            return fmt::format("{}.{}", alias, iter->second.name);
        }

        return fmt::format("cast({}.{} as {})", alias, iter->second.name, _column.type_name);
    } // to_sql

    auto to_sql(gq_state& _state, const select_function& _select_function) -> std::string
    {
        const auto iter = column_name_mappings.find(_select_function.column.name);

        if (iter == std::end(column_name_mappings)) {
            throw std::invalid_argument{fmt::format("unknown column: {}", _select_function.column.name)};
        }

        // Capture all column objects as some parts of the implementation need to access them later in
        // order to generate the proper SQL.
        _state.ast_column_ptrs.push_back(&_select_function.column);

        // Aggregate functions are not allowed in the WHERE clause of an SQL statement!
        if (!_state.in_select_clause) {
            throw std::invalid_argument{"aggregate functions not allowed in where clause"};
        }

        auto [is_special_column, table_alias] =
            setup_column_for_post_processing(_state, _select_function.column, iter->second);
        const std::string_view alias =
            is_special_column ? table_alias : _state.table_aliases.at(std::string{iter->second.table});

        if (_select_function.column.type_name.empty()) {
            return fmt::format("{}({}.{})", _select_function.name, alias, iter->second.name);
        }

        return fmt::format("{}(cast({}.{} as {}))",
                           _select_function.name,
                           alias,
                           iter->second.name,
                           _select_function.column.type_name);
    } // to_sql

    auto to_sql(gq_state& _state, const selections& _selections) -> std::string
    {
        irods::at_scope_exit restore_value{[&_state] { _state.in_select_clause = false; }};

        // Provides additional context around the processing of the SELECT clause's column list.
        // This specifically helps the parser determine what should be included in the select
        // clause's column list. This is necessary because the gq::column data type is used in multiple
        // bison parser rules.
        _state.in_select_clause = true;

        _state.sql_tables.clear();

        if (_selections.empty()) {
            throw std::runtime_error{"no columns selected"};
        }

        std::vector<std::string> cols;
        sql_visitor v{_state};

        for (auto&& s : _selections) {
            cols.push_back(boost::apply_visitor(v, s));
        }

        return fmt::format("{}", fmt::join(cols, ", "));
    } // to_sql

    auto to_sql(gq_state& _state, const condition_operator_not& _op_not) -> std::string
    {
        return fmt::format(" not{}", boost::apply_visitor(sql_visitor{_state}, _op_not.expression));
    } // to_sql

    auto to_sql(gq_state& _state, const condition_not_equal& _not_equal) -> std::string
    {
        _state.values.push_back(_not_equal.string_literal);
        return " != ?";
    } // to_sql

    auto to_sql(gq_state& _state, const condition_equal& _equal) -> std::string
    {
        _state.values.push_back(_equal.string_literal);
        return " = ?";
    } // to_sql

    auto to_sql(gq_state& _state, const condition_less_than& _less_than) -> std::string
    {
        _state.values.push_back(_less_than.string_literal);
        return " < ?";
    } // to_sql

    auto to_sql(gq_state& _state, const condition_less_than_or_equal_to& _less_than_or_equal_to) -> std::string
    {
        _state.values.push_back(_less_than_or_equal_to.string_literal);
        return " <= ?";
    } // to_sql

    auto to_sql(gq_state& _state, const condition_greater_than& _greater_than) -> std::string
    {
        _state.values.push_back(_greater_than.string_literal);
        return " > ?";
    } // to_sql

    auto to_sql(gq_state& _state, const condition_greater_than_or_equal_to& _greater_than_or_equal_to) -> std::string
    {
        _state.values.push_back(_greater_than_or_equal_to.string_literal);
        return " >= ?";
    } // to_sql

    auto to_sql(gq_state& _state, const condition_between& _between) -> std::string
    {
        _state.values.push_back(_between.low);
        _state.values.push_back(_between.high);
        return " between ? and ?";
    } // to_sql

    auto to_sql(gq_state& _state, const condition_in& _in) -> std::string
    {
        _state.values.insert(
            std::end(_state.values), std::begin(_in.list_of_string_literals), std::end(_in.list_of_string_literals));

        struct
        {
            using result_type = std::string_view;
            auto operator()() const noexcept -> result_type
            {
                return "?";
            }
        } gen;

        using size_type = decltype(_in.list_of_string_literals)::size_type;

        auto first = boost::make_function_input_iterator(gen, size_type{0});
        auto last = boost::make_function_input_iterator(gen, _in.list_of_string_literals.size());

        return fmt::format(" in ({})", fmt::join(first, last, ", "));
    } // to_sql

    auto to_sql(gq_state& _state, const condition_like& _like) -> std::string
    {
        _state.values.push_back(_like.string_literal);
        return " like ?";
    } // to_sql

    auto to_sql([[maybe_unused]] gq_state& _state, const condition_is_null&) -> std::string
    {
        return " is null";
    } // to_sql

    auto to_sql([[maybe_unused]] gq_state& _state, const condition_is_not_null&) -> std::string
    {
        return " is not null";
    } // to_sql

    auto to_sql(gq_state& _state, const condition& _condition) -> std::string
    {
        return fmt::format("{}{}",
                           to_sql(_state, _condition.column),
                           boost::apply_visitor(sql_visitor{_state}, _condition.expression));
    } // to_sql

    auto to_sql(gq_state& _state, const conditions& _conditions) -> std::string
    {
        std::string ret;

        for (auto&& condition : _conditions) {
            ret += boost::apply_visitor(sql_visitor{_state}, condition);
        }

        return ret;
    } // to_sql

    auto to_sql(gq_state& _state, const logical_and& _condition) -> std::string
    {
        return fmt::format(" and {}", to_sql(_state, _condition.condition));
    } // to_sql

    auto to_sql(gq_state& _state, const logical_or& _condition) -> std::string
    {
        return fmt::format(" or {}", to_sql(_state, _condition.condition));
    } // to_sql

    auto to_sql(gq_state& _state, const logical_not& _condition) -> std::string
    {
        return fmt::format("not {}", to_sql(_state, _condition.condition));
    } // to_sql

    auto to_sql(gq_state& _state, const logical_grouping& _condition) -> std::string
    {
        return fmt::format("({})", to_sql(_state, _condition.conditions));
    } // to_sql

    auto to_sql(const select& _select, const options& _opts) -> std::tuple<std::string, std::vector<std::string>>
    {
        try {
            gq_state state;

            log_gq::trace("### PHASE 1: Gather");

            const auto cols = to_sql(state, _select.selections);
            log_gq::debug("SELECT COLUMNS = {}", cols);

            // Convert the conditions of the general query statement into SQL with prepared
            // statement placeholders.
            const auto conds = to_sql(state, _select.conditions);
            log_gq::debug("CONDITIONS = {}", conds);

            if (state.sql_tables.empty()) {
                return {{}, {}};
            }

            {
                std::sort(std::begin(state.ast_column_ptrs), std::end(state.ast_column_ptrs));
                auto end = std::end(state.ast_column_ptrs);
                state.ast_column_ptrs.erase(std::unique(std::begin(state.ast_column_ptrs), end), end);
            }

            std::for_each(std::begin(state.sql_tables), std::end(state.sql_tables), [&state](auto&& _t) {
                std::string_view alias = "";

                if (const auto iter = state.table_aliases.find(std::string{_t}); iter != std::end(state.table_aliases))
                {
                    alias = iter->second;
                }

                log_gq::debug("TABLE => {} [alias={}]", _t, alias);
            });

            log_gq::debug("Requires metadata table joins for R_DATA_MAIN? {}", state.add_joins_for_meta_data);
            log_gq::debug("Requires metadata table joins for R_COLL_MAIN? {}", state.add_joins_for_meta_coll);
            log_gq::debug("Requires metadata table joins for R_RESC_MAIN? {}", state.add_joins_for_meta_resc);
            log_gq::debug("Requires metadata table joins for R_USER_MAIN? {}", state.add_joins_for_meta_user);
            log_gq::debug("Requires table joins for DATA_RESC_HIER? {}", state.add_sql_for_data_resc_hier);

            //
            // Generate SQL
            //

            log_gq::trace("### PHASE 2: SQL Generation");

            auto graph = init_graph();

            // Generate the SELECT clause.
            //
            // This step does not concern itself with special columns (e.g. META_DATA_ATTR_NAME). Those
            // will be handled in a later step.
            //
            // The tables stored in sql_tables must be directly joinable to at least one other table in
            // the sql_tables list. This step is NOT allowed to introduce intermediate tables.
            auto select_clause =
                fmt::format("{with_clause}select {distinct}{columns} from {table} {alias}",
                            fmt::arg("with_clause", generate_with_clause_for_data_resc_hier(state, _opts.database)),
                            fmt::arg("distinct", _select.distinct ? "distinct " : ""),
                            fmt::arg("columns", cols),
                            fmt::arg("table", state.sql_tables.front()),
                            fmt::arg("alias", state.table_aliases.at(std::string{state.sql_tables.front()})));

            log_gq::debug("SELECT CLAUSE => {}", select_clause);

            const auto inner_joins = generate_inner_joins(graph, state.sql_tables, state.table_aliases);

            std::for_each(std::begin(inner_joins), std::end(inner_joins), [](auto&& _j) {
                log_gq::debug("INNER JOIN => {}", _j);
            });

            if (inner_joins.size() != state.sql_tables.size() - 1) {
                throw std::invalid_argument{"invalid general query"};
            }

            auto sql = select_clause;

            if (!inner_joins.empty()) {
                sql += fmt::format(" {}", fmt::join(inner_joins, " "));
            }

            // Q. Should tickets be scoped to data objects and collections separately?
            // Q. What happens if a user attempts to query data objects, collections, and tickets in the same query?
            // Q. Should these questions be handled by specific queries instead?

            sql += generate_joins_for_permissions(state.table_aliases);
            sql += generate_joins_for_metadata_columns(state);

            if (state.add_sql_for_data_resc_hier) {
                sql += fmt::format(
                    " inner join cte_drh on cte_drh.resc_id = {}.resc_id", state.table_aliases.at("R_RESC_MAIN"));
            }

            sql += generate_condition_clause(state, _opts, conds);
            sql += generate_group_by_clause(state, _select.group_by, column_name_mappings);
            sql += generate_order_by_clause(state, _select.order_by, column_name_mappings);
            sql += generate_limit_clause(_opts, _select.range.number_of_rows);

            // MySQL requires that the OFFSET clause be defined after the LIMIT clause, therefore we
            // handle OFFSET here.
            //
            // See https://dev.mysql.com/doc/refman/8.0/en/select.html.
            if (!_select.range.offset.empty()) {
                sql += fmt::format(" offset {}", _select.range.offset);
            }

            std::for_each(std::begin(state.values), std::end(state.values), [](auto&& _j) {
                log_gq::debug("BINDABLE VALUE => {}", _j);
            });

            log_gq::debug("GENERATED SQL => [{}]", sql);

            return {sql, std::move(state.values)};
        }
        catch (const std::exception& e) {
            log_gq::error(e.what());
        }

        return {{}, {}};
    } // to_sql
} // namespace irods::experimental::genquery2
