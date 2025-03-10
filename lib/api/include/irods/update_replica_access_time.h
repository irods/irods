#ifndef IRODS_UPDATE_REPLICA_ACCESS_TIME_H
#define IRODS_UPDATE_REPLICA_ACCESS_TIME_H

/// \file

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// TODO
///
/// \since 5.0.0
int rc_update_replica_access_time(struct RcComm* _comm, const char* _input, char** _output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_UPDATE_REPLICA_ACCESS_TIME_H
