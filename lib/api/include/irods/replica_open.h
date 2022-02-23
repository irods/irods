#ifndef IRODS_REPLICA_OPEN_H
#define IRODS_REPLICA_OPEN_H

struct RcComm;
struct DataObjInp;

#ifdef __cplusplus
extern "C" {
#endif

/// Opens a replica and returns the associated iRODS file descriptor information as JSON.
///
/// \param[in]  _comm        A pointer to a RcComm.
/// \param[in]  _input       A pointer to a DataObjInp containing options for opening a replica.
/// \param[out] _json_output A JSON string containing all file descriptor information.
///
/// \return An integer representing the file descriptor. Valid file descriptors are greater than
///         or equal to 3. Negative values indicate an error has occurred.
///
/// \since 4.2.9
int rc_replica_open(struct RcComm* _comm, struct DataObjInp* _input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_REPLICA_OPEN_H

