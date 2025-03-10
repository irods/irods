#ifndef IRODS_RS_UPDATE_REPLICA_ACCESS_TIME_HPP
#define IRODS_RS_UPDATE_REPLICA_ACCESS_TIME_HPP

/// \file

struct RsComm;

#include "irods/rodsDef.h" // For BytesBuf.

/// TODO
///
/// \since 5.0.0
int rs_update_replica_access_time(RsComm* _comm, BytesBuf* _input, char** _output);

#endif // IRODS_RS_UPDATE_REPLICA_ACCESS_TIME_HPP
