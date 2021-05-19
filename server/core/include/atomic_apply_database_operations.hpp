#ifndef IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP
#define IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP

#include <json.hpp>

#include <vector>
#include <string>
#include <variant>
#include <system_error>

namespace irods::experimental
{
    namespace dml
    {
        struct column
        {
            std::string name;
            std::string value;
        };

        struct row_data
        {
            std::vector<column> columns;
        };

        struct condition
        {
            const std::string column;
            const std::string op;
            const std::vector<std::string> values;

            condition(const std::string& _column,
                      const std::string& _operator,
                      const std::vector<std::string>& _values);
        };

        struct insert_op
        {
            const std::string table;
            const std::vector<row_data> rows;

            insert_op(const std::string& _table, const std::vector<row_data>& _rows);
        };

        struct update_op
        {
            const std::string table;
            const row_data row_data;
            const std::vector<condition> conditions;

            update_op(const std::string& _table,
                      const struct row_data& _row,
                      const std::vector<condition> _conditions);
        };

        struct delete_op
        {
            const std::string table;
            const std::vector<condition> conditions;

            delete_op(const std::string& _table,
                      const std::vector<condition>& _conditions);
        };

        using operation_type = std::variant<insert_op, update_op, delete_op>;
    } // namespace dml

    /// \param[in] _ops
    ///
    /// \return An integer.
    /// \retval 0        On success.
    /// \retval non-zero On failure.
    ///
    /// \since 4.2.9
    auto atomic_apply_database_operations(const nlohmann::json& _ops) -> int;

    /// \param[in]     _ops
    /// \param[in,out] _ec
    ///
    /// \since 4.2.9
    auto atomic_apply_database_operations(const nlohmann::json& _ops, std::error_code& _ec) -> void;

    /// \param[in] _ops
    ///
    /// \return An integer.
    /// \retval 0        On success.
    /// \retval non-zero On failure.
    ///
    /// \since 4.2.9
    auto atomic_apply_database_operations(const std::vector<dml::operation_type>& _ops) -> int;
} // namespace irods::experimental

#endif // IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP

