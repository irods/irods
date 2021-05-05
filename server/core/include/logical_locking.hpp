#ifndef IRODS_LOGICAL_LOCKING_HPP
#define IRODS_LOGICAL_LOCKING_HPP

#include "replica_state_table.hpp"

#include <string_view>

/// \file

struct DataObjInfo;
struct DataObjInp;
struct RsComm;

namespace irods::logical_locking
{
    // TODO: future work...
    // data_status column will contain something like the following when an object is locked for every replica:
    //{
    //    "original_status": 1,
    //    "agents": [
    //        {
    //            "hostname": <string>,
    //            "pid": <uint32>,
    //            "timestamp": <rodsLong>
    //        },
    //        ...
    //    ]
    //}
    //

    /// \brief Special value to indicate a replica status should be restored
    ///
    /// \parblock
    /// This is intended to be passed to the unlock interface in lieu of a real replica status.
    /// When the unlock implementation sees this value, the replica status stored in the data_status
    /// column is retrieved and used to set the new replica status in the RST and/or catalog.
    /// \endparblock
    ///
    /// \since 4.2.9
    static inline constexpr int restore_status = -1;

    /// \brief Type of lock to acquire
    ///
    /// \parblock
    /// Each lock type corresponds to a particular lock type:
    ///     - read == READ_LOCKED
    ///     - write == WRITE_LOCKED
    /// \endparblock
    ///
    /// \since 4.2.9
    enum class lock_type
    {
        read,
        write
    };

    /// \brief Convenience function to retrieve the status for a particular replica before a data object was locked
    ///
    /// \param[in] _data_id Data ID for locked object
    /// \param[in] _replica_number Replica for which we will retrieve the original status
    ///
    /// \returns The original replica status before the data object was locked (stored in data_status at time of lock)
    /// \retval -1 If data_status is empty or does not contain the original_status key
    ///
    /// \throws json::exception If the data_status column is improperly formatted JSON
    ///
    /// \since 4.2.9
    auto get_original_replica_status(
        const std::uint64_t _data_id,
        const int           _replica_number) -> int;

    /// \brief Lock data object using specified lock type, update RST
    ///
    /// \parblock
    /// Logical locking operates on replica information found in the replica_state_table.
    /// A valid replica_state_table entry must exist for the data object which is to be locked.
    ///
    /// Locking a data object is effectively declaring it to be in flight.
    ///
    /// Sets all of the replica statuses to the appropriate value based on the lock type,
    /// except for the target replica, which represents the replica which is being opened
    /// either for read or write. On read lock, the target replica is read-locked just like
    /// all of the sibling replicas. On write lock, the target replica is put into the intermediate
    /// state as it is being opened for write.
    ///
    /// The original status for each replica is stored in the data_status column in the
    /// following format:
    /// \code{.js}
    ///     {
    ///         "original_status": <int>
    ///     }
    /// \endcode
    ///
    /// The replica status and data_status column updates are not reflected in the catalog
    /// as a result of this call. This merely updates the appropriate entry in the replica_state_table.
    ///
    /// The caller is responsible for unlocking the data object and removing the replica_state_table entry.
    /// \endparblock
    ///
    /// \param[in] _data_id Data ID for object to lock
    /// \param[in] _replica_number Target replica number
    /// \param[in] _lock_type Indicates the type of lock to set the replicas to, except the target replica
    ///
    /// \returns Error code on failure
    ///
    /// \since 4.2.9
    auto lock(
        const std::uint64_t _data_id,
        const int           _replica_number,
        const lock_type     _lock_type) -> int;

    /// \brief Unlock data object, update RST
    ///
    /// \parblock
    /// Logical locking operates on replica information found in the replica_state_table.
    /// A valid replica_state_table entry must exist for the data object which is to be unlocked.
    /// Additionally, the original status for the replica must be present in the data_status column
    /// if the replica's status is to be restored.
    ///
    /// If no valid status is found in data_status, the replica status restoration will fail.
    /// This is a critical failure from which the system cannot recover. In this case,
    /// iadmin modrepl can be used to manually fix the replica status.
    ///
    /// Unlocking a data object is effectively declaring it to be at rest.
    ///
    /// Sets all of the replica statuses to the statuses specified by the inputs. If the restore_status
    /// value is used, the value stored in the original_status key of data_status column will be used
    /// to restore the replica to its original status.
    ///
    /// The data_status column is cleared after updating the replica status.
    ///
    /// The replica status and data_status column updates are not reflected in the catalog
    /// as a result of this call. This merely updates the appropriate entry in the replica_state_table.
    /// \endparblock
    ///
    /// \param[in] _data_id Data ID for object to lock
    /// \param[in] _replica_number Target replica number
    /// \param[in] _replica_status Desired replica status for the target replica
    /// \param[in] _other_replica_statuses The desired replica status for the sibling replicas
    ///
    /// \returns Error code on failure
    ///
    /// \since 4.2.9
    auto unlock(
        const std::uint64_t _data_id,
        const int           _replica_number,
        const int           _replica_status,
        const int           _other_replica_statuses = restore_status) -> int;

    /// \brief Lock data object using specified lock type, update RST, publish to catalog
    ///
    /// \parblock
    /// Calls the lock implemenation as described in lock"("const std::uint64_t, const int,const lock_type")"
    /// by updating the replica_state_table, then publishes the entry to the catalog immediately. file_modified
    /// will never be triggered by this operation.
    /// \endparblock
    ///
    /// \param[in,out] _comm
    /// \param[in] _ctx Context for publishing an RST entry (see irods::replica_state_table::publish::context for details)
    /// \param[in] _lock_type Indicates the type of lock to set the replicas to, except the target replica
    ///
    /// \returns Error code on failure
    ///
    /// \since 4.2.9
    auto lock_and_publish(
        RsComm&                      _comm,
        const irods::replica_state_table::publish::context& _ctx,
        const lock_type              _lock_type) -> int;

    /// \brief Unlock data object, update RST, publish to catalog
    ///
    /// \parblock
    /// Calls the unlock implemenation as described in unlock"("const std::uint64_t, const int, const int, const int")"
    /// by updating the replica_state_table, then publishes the entry to the catalog immediately. file_modified
    /// will never be triggered by this operation.
    /// \endparblock
    ///
    /// \param[in,out] _comm
    /// \param[in] _ctx Context for publishing an RST entry (see irods::replica_state_table::publish::context for details)
    /// \param[in] _replica_status Desired replica status for the target replica
    /// \param[in] _other_replica_statuses The desired replica status for the sibling replicas
    ///
    /// \returns Error code on failure
    ///
    /// \since 4.2.9
    auto unlock_and_publish(
        RsComm&                      _comm,
        const irods::replica_state_table::publish::context& _ctx,
        const int                    _replica_status,
        const int                    _other_replica_statuses = restore_status) -> int;

    /// \brief Check to see if the data object will lock out some operation.
    ///
    /// \parblock
    /// This overload does not target a specific replica and so if any replica of the
    /// specified data object is locked or intermediate, the lock will be tripped.
    /// \endparblock
    ///
    /// \param[in] _obj The data object on which we are checking for a lock
    /// \param[in] _lock_type Type of lock to test for
    ///
    /// \returns An error code describing the access violation or 0 if no violation
    /// \retval 0 The data object is not locked
    /// \retval LOCKED_DATA_OBJECT_ACCESS Another replica for the data object is locked
    ///
    /// \since 4.2.9
    auto try_lock(const DataObjInfo& _obj, const lock_type _lock_type) -> int;

    /// \brief Check to see if the data object will lock out some operation.
    ///
    /// \parblock
    /// This overload targets a specific replica based on resource hierarchy. If the
    /// target replica is in the intermediate state and the replica token does not match,
    /// the lock will be tripped with a specific error. If any replica is locked or any
    /// sibling replica is intermediate, the lock will be tripped.
    /// \endparblock
    ///
    /// \param[in] _obj The data object being tested for a lock
    /// \param[in] _lock_type Type of lock to test for
    /// \param[in] _target_replica_hierarchy Resource hierarchy for the target replica
    /// \param[in] _target_replica_token Replica token for target replica (can be empty)
    ///
    /// \returns An error code describing the access violation or 0 if no violation
    /// \retval 0 The data object is not locked
    /// \retval INTERMEDIATE_REPLICA_ACCESS The targeted replica is intermediate
    /// \retval LOCKED_DATA_OBJECT_ACCESS Another replica for the data object is locked
    ///
    /// \since 4.2.9
    auto try_lock(
        const DataObjInfo&     _obj,
        const lock_type        _lock_type,
        const std::string_view _target_replica_hierarchy,
        const std::string_view _target_replica_token = "") -> int;

    /// \brief Check to see if the data object will lock out some operation.
    ///
    /// \parblock
    /// This overload targets a specific replica based on resource hierarchy. If the
    /// target replica is in the intermediate state and the replica token does not match,
    /// the lock will be tripped with a specific error. If any replica is locked or any
    /// sibling replica is intermediate, the lock will be tripped.
    /// \endparblock
    ///
    /// \param[in] _obj The data object being tested for a lock
    /// \param[in] _lock_type Type of lock to test for
    /// \param[in] _target_replica_number Replica number for the target replica
    /// \param[in] _target_replica_token Replica token for target replica (can be empty)
    ///
    /// \returns An error code describing the access violation or 0 if no violation
    /// \retval 0 The data object is not locked
    /// \retval INTERMEDIATE_REPLICA_ACCESS The targeted replica is intermediate
    /// \retval LOCKED_DATA_OBJECT_ACCESS Another replica for the data object is locked
    ///
    /// \since 4.2.9
    auto try_lock(
        const DataObjInfo&     _obj,
        const lock_type        _lock_type,
        const int              _target_replica_number,
        const std::string_view _target_replica_token = "") -> int;
} // namespace irods::logical_locking

#endif // #ifndef IRODS_LOGICAL_LOCKING_HPP
