#include "irods_file_object.hpp"
#include "irods_log.hpp"
#include "replication_utilities.hpp"
#include "rodsError.h"

#include "fmt/format.h"

#include <string>

namespace irods::replication
{
    auto is_allowed(
        RsComm& _comm,
        const irods::physical_object& _source_replica,
        const irods::physical_object& _destination_replica,
        const log_errors _log_errors) -> bool
    {
        // Assumes that source and destination replicas already exist!
        // If source does not exist, replication is not possible.
        // If destination does not exist, replication is allowed.
        if (_source_replica.resc_id() == _destination_replica.resc_id()) {
            const std::string msg = fmt::format(
                "destination hierarchy [{}] matches source hierarchy [{}]; replication is not allowed.",
                _destination_replica.resc_hier(), _source_replica.resc_hier());

            irods::log(LOG_DEBUG, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

            if (log_errors::yes == _log_errors) {
                addRErrorMsg(&_comm.rError, SYS_NOT_ALLOWED, msg.data());
            }

            return false;
        }

        if (GOOD_REPLICA != _source_replica.replica_status()) {
            const std::string msg = fmt::format(
                "selected source hierarchy [{}] is not good and will overwrite an existing replica; replication is not allowed.",
                _source_replica.resc_hier());

            irods::log(LOG_DEBUG, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

            if (log_errors::yes == _log_errors) {
                addRErrorMsg(&_comm.rError, SYS_NOT_ALLOWED, msg.data());
            }

            return false;
        }

        if (STALE_REPLICA != _destination_replica.replica_status()) {
            const std::string msg = fmt::format(
                "selected destination hierarchy [{}] is not stale; replication is not allowed.",
                _destination_replica.resc_hier());

            irods::log(LOG_DEBUG, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

            if (log_errors::yes == _log_errors) {
                addRErrorMsg(&_comm.rError, SYS_NOT_ALLOWED, msg.data());
            }

            return false;
        }

        return true;
    } // replication_is_allowed
} // namespace irods::replication
