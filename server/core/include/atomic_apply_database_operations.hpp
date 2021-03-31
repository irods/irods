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
        struct column_value
        {
            std::string column;
            std::string value;
        };

        struct insert_op
        {
            std::string table;
            std::vector<column_value> values;
        };

        struct update_op
        {

        };

        struct delete_op
        {

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

