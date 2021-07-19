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
            column(const std::string& _name, const std::string& _value)
                : name{_name}
                , value{_value}
            {
            }

            const std::string name;
            const std::string value;
        }; // struct column

        struct condition
        {
            condition(const std::string& _column,
                      const std::string& _operator,
                      const std::vector<std::string>& _values)
                : column{_column}
                , op{_operator}
                , values{_values}
            {
            }

            const std::string column;
            const std::string op;
            const std::vector<std::string> values;
        }; // struct condition

        struct insert_op
        {
            insert_op(const std::string& _table, const std::vector<column>& _data)
                : table{_table}
                , data{_data}
            {
            }

            const std::string table;
            const std::vector<column> data;
        }; // struct insert_op

        struct update_op
        {
            update_op(const std::string& _table,
                      const std::vector<column>& _data,
                      const std::vector<condition>& _conditions)
                : table{_table}
                , data{_data}
                , conditions{_conditions}
            {
            }

            const std::string table;
            const std::vector<column> data;
            const std::vector<condition> conditions;
        }; // struct update_op

        struct delete_op
        {
            delete_op(const std::string& _table,
                      const std::vector<condition>& _conditions)
                : table{_table}
                , conditions{_conditions}
            {
            }

            const std::string table;
            const std::vector<condition> conditions;
        }; // struct delete_op

        using operation_type = std::variant<insert_op, update_op, delete_op>;
    } // namespace dml

    /// \param[in] _ops
    ///
    /// \return An integer.
    /// \retval 0        On success.
    /// \retval non-zero On failure.
    ///
    /// \since 4.2.9
    auto atomic_apply_database_operations(const std::vector<dml::operation_type>& _ops) -> int;

    /// \param[in]     _ops
    /// \param[in,out] _ec
    ///
    /// \since 4.2.9
    auto atomic_apply_database_operations(const std::vector<dml::operation_type>& _ops,
                                          std::error_code& _ec) -> void;
} // namespace irods::experimental

#endif // IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP

