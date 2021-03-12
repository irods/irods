#ifndef IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP
#define IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP

/// \file

#include <vector>
#include <string>
#include <variant>
#include <system_error>

namespace irods::experimental
{
    /// Defines data types and functions that are used for database manipulation.
    ///
    /// \since 4.3.0
    namespace dml
    {
        /// A class that associates a value to a database column.
        ///
        /// Objects of this type are immutable.
        ///
        /// \since 4.3.0
        struct column
        {
            /// Constructor.
            ///
            /// \param[in] _name  The column name.
            /// \param[in] _value The column value.
            ///
            /// \since 4.3.0
            column(const std::string& _name, const std::string& _value)
                : name{_name}
                , value{_value}
            {
            }

            const std::string name;
            const std::string value;
        }; // struct column

        /// A class representing a SQL DML condition.
        ///
        /// Objects of this type are immutable.
        ///
        /// \since 4.3.0
        struct condition
        {
            /// Constructor.
            ///
            /// \param[in] _column   The column name.
            /// \param[in] _operator The operator. Valid operators are: =, !=, >, >=, <, <=, in
            /// \param[in] _values   The list of values.
            ///
            /// \since 4.3.0
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

        /// An class representing a database insert operation.
        ///
        /// Objects of this type are immutable.
        ///
        /// \since 4.3.0
        struct insert_op
        {
            /// Constructor.
            ///
            /// \param[in] _table The database table to insert into.
            /// \param[in] _data  The list of column values to insert.
            ///
            /// \since 4.3.0
            insert_op(const std::string& _table, const std::vector<column>& _data)
                : table{_table}
                , data{_data}
            {
            }

            const std::string table;
            const std::vector<column> data;
        }; // struct insert_op

        /// A class representing a database update operation.
        ///
        /// Objects of this type are immutable.
        ///
        /// \since 4.3.0
        struct update_op
        {
            /// Constructor.
            ///
            /// \param[in] _table      The database table to update.
            /// \param[in] _data       The list of columns within the table to update.
            /// \param[in] _conditions The list of conditions used to target rows that
            ///                        should be updated.
            ///
            /// \since 4.3.0
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

        /// A class representing a database delete operation.
        ///
        /// Objects of this type are immutable.
        ///
        /// \since 4.3.0
        struct delete_op
        {
            /// Constructor.
            ///
            /// \param[in] _table      The database table to delete from.
            /// \param[in] _conditions The list of conditions used to target rows that
            ///                        should be deleted.
            ///
            /// \since 4.3.0
            delete_op(const std::string& _table,
                      const std::vector<condition>& _conditions)
                : table{_table}
                , conditions{_conditions}
            {
            }

            const std::string table;
            const std::vector<condition> conditions;
        }; // struct delete_op

        /// A type capable of holding any type of DML operation type.
        ///
        /// \since 4.3.0
        using operation_type = std::variant<insert_op, update_op, delete_op>;
    } // namespace dml

    /// Executes a list of database manipulation operations in the order specified.
    ///
    /// \param[in] _ops The heterogeneous list of DML operations to execute.
    ///
    /// \throws irods::exception If the operation experiences an unrecoverable failure.
    ///
    /// \return An integer.
    /// \retval 0        On success.
    /// \retval non-zero On failure.
    ///
    /// \since 4.3.0
    auto atomic_apply_database_operations(const std::vector<dml::operation_type>& _ops) -> int;

    /// The non-throwing version of atomic_apply_database_operations(const std::vector<dml::operation_type>& _ops).
    ///
    /// \param[in]     _ops The heterogeneous list of DML operations to execute.
    /// \param[in,out] _ec  Holds the error information following invocation.
    ///
    /// \since 4.3.0
    auto atomic_apply_database_operations(const std::vector<dml::operation_type>& _ops,
                                          std::error_code& _ec) -> void;
} // namespace irods::experimental

#endif // IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP

