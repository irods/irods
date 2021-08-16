#include "replica_close.h"

#include "api_plugin_number.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

#include <cstring>

auto rc_replica_close(RcComm* _comm, const char* _json_input) -> int
{
    if (!_json_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input{};
    input.buf = const_cast<char*>(_json_input);
    input.len = static_cast<int>(std::strlen(_json_input)) + 1;

    return procApiRequest(_comm, REPLICA_CLOSE_APN, &input, nullptr, nullptr, nullptr);
}
