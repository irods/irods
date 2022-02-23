#ifndef IRODS_REPLICATION_UTILITIES_HPP
#define IRODS_REPLICATION_UTILITIES_HPP

struct RsComm;

namespace irods::replication
{
    enum class log_errors
    {
        no,
        yes
    };

    auto is_allowed(
        RsComm& _comm,
        const irods::physical_object& _source_replica,
        const irods::physical_object& _destination_replica,
        const log_errors _log_errors) -> bool;
} // namespace irods::replication

#endif // IRODS_REPLICATION_UTILITIES_HPP
