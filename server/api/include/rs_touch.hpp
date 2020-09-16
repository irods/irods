#ifndef IRODS_RS_TOUCH_HPP
#define IRODS_RS_TOUCH_HPP

/// \file

struct RsComm;

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Change the modification time (mtime) of a data object or collection.
///
/// Modeled after UNIX touch.
///
/// \param[in] _comm       A pointer to a RsComm.
/// \param[in] _json_input \parblock
/// A JSON string containing information for updating the mtime.
///
/// The JSON string must have the following structure:
/// \code{.js}
/// {
///   "logical_path": string,
///   "options": {
///     "no_create": boolean,
///     "replica_number": integer,
///     "leaf_resource_name": string,
///     "seconds_since_epoch": integer,
///     "reference": string
///   }
/// }
/// \endcode
/// \endparblock
///
/// \p logical_path must be an absolute path to a data object or collection.
///
/// \p options is the set of optional values for controlling the behavior
/// of the operation. This field and all fields within are optional.
///
/// \p no_create Instructs the system to not create the target data object
/// when it does not exist.
///
/// \p replica_number Identifies the replica to update. Replica numbers cannot
/// be used to create data objects or additional replicas. This option cannot
/// be used with \p leaf_resource_name.
///
/// \p leaf_resource_name Identifies the location of the replica to update.
/// If the object identified by \p logical_path does not exist and this option
/// is provided, the data object will be created at the specified resource.
/// This option cannot be used with \p replica_number.
///
/// \p seconds_since_epoch The new mtime in seconds. This option cannot be
/// used with \p reference.
///
/// \p reference The absolute path of a data object or collection whose mtime
/// will be used as the new mtime for the target object. This option cannot
/// be used with \p seconds_since_epoch.
///
/// If no object exists at \p logical_path and no options are provided, the
/// server will create a new data object on the resource defined by
/// ::msiSetDefaultResc.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.9
int rs_touch(RsComm* _comm, const char* _json_input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_TOUCH_HPP

