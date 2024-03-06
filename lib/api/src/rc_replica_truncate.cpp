#include "irods/replica_truncate.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_replica_truncate(RcComm* _comm, DataObjInp* _inp, char** _out) -> int
{
    if (!_comm || !_inp || !_out) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm, REPLICA_TRUNCATE_AN, _inp, nullptr, reinterpret_cast<void**>(_out), nullptr);
} // rc_replica_truncate
