#ifndef IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP
#define IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP

#include <json.hpp>

#include <system_error>

struct RsComm;

namespace irods::experimental
{
    /// \param[in] _comm
    /// \param[in] _ops
    ///
    /// \return An integer.
    /// \retval 0        On success.
    /// \retval non-zero On failure.
    ///
    /// \since 4.2.9
    auto atomic_apply_database_operations(RsComm& _comm, const nlohmann::json& _ops) -> int;

    /// \param[in]     _comm
    /// \param[in]     _ops
    /// \param[in,out] _ec
    ///
    /// \since 4.2.9
    auto atomic_apply_database_operations(RsComm& _comm, const nlohmann::json& _ops, std::error_code& _ec) -> void;
} // namespace irods::experimental

#endif // IRODS_ATOMIC_APPLY_DATABASE_OPERATIONS_HPP

