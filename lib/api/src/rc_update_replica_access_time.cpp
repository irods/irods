#include "irods/update_replica_access_time.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

#include <cstdlib>
#include <cstring>

auto rc_update_replica_access_time(RcComm* _comm, const char* _json_input, char** _output) -> int
{
    if (!_comm || !_json_input || !_output) {
        return INVALID_INPUT_ARGUMENT_NULL_POINTER;
    }

    bytesBuf_t input_buf{};
    input_buf.buf = const_cast<char*>(_json_input);
    input_buf.len = static_cast<int>(std::strlen(_json_input));

    return procApiRequest(
        _comm, UPDATE_REPLICA_ACCESS_TIME_AN, &input_buf, nullptr, reinterpret_cast<void**>(_output), nullptr);
}
