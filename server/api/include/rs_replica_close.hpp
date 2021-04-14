#ifndef IRODS_RS_REPLICA_CLOSE_HPP
#define IRODS_RS_REPLICA_CLOSE_HPP

/// \file

struct RsComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Updates the catalog (if requested) and closes the replica.
///
/// \parblock
/// This function is responsible for making sure the replica in question is reflected in the catalog
/// correctly. On a successful close, the catalog will reflect the correct size on disk and will have
/// a replica status of good (i.e. 1). Notifications will be triggered so that replication and other
/// policy fires correctly. This function is NOT responsible for making sure that sibling replicas are
/// in a proper state.
///
/// On a failure, the request may be resubmitted depending on what happened. Resubmission is possible
/// because the server updates the catalog before closing the replica. Once the replica is closed,
/// resubmission is not possible.
///
/// An important difference between this function and ::rcDataObjClose is that this function provides
/// options for controlling when the server should compute checksums and/or synchronize the catalog
/// with the replica on disk.
///
/// This function is best used in situations where multiple streams reference the same replica.
/// \endparblock
///
/// \since 4.2.9
///
/// \param[in] _comm       A pointer to a RsComm.
/// \param[in] _json_input \parblock
/// A JSON string containing the iRODS file descriptor as well as instructions for computing checksums
/// and updating the catalog.
/// 
/// The JSON string must have the following structure:
/// \code{.js}
/// {
///   "fd": integer,
///   "update_size": boolean,                 (default=true)
///   "update_status": boolean,               (default=true)
///   "compute_checksum": boolean,            (default=false)
///   "send_notifications": boolean,          (default=true)
///   "preserve_replica_state_table": boolean (default=false)
/// }
/// \endcode
///
/// - \p fd: The iRODS file descriptor returned by \p rxDataObjOpen or \p rx_replica_close.
///
/// - \p update_size: Controls whether the size is updated in the catalog.
///
/// - \p update_status: Controls whether the replica's status is updated in the catalog.
///
/// - \p compute_checksum: Controls whether a checksum is computed and stored in the catalog. If a
/// checksum already exists, it will be overwritten.
///
/// - \p send_notifications: Controls whether the file modified signal is triggered. This is necessary
/// for things such as replication.
///
/// - \p preserve_replica_state_table: Controls whether the replica state table is updated following
/// the closing of the replica. This must be set to true if \p rx_data_object_finalize will be
/// invoked.
/// \endparblock
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
int rs_replica_close(RsComm* _comm, const char* _json_input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_REPLICA_CLOSE_HPP

