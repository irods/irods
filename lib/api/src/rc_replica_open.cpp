#include "replica_open.h"

#include "api_plugin_number.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

#include <cstdlib>
#include <cstring>

auto rc_replica_open(RcComm* _comm, DataObjInp* _input, char** _json_output) -> int
{
    if (!_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t* output{};

    const int ec = procApiRequest(_comm, REPLICA_OPEN_APN,
                                  _input, nullptr,
                                  reinterpret_cast<void**>(&output), nullptr);

    if (ec >= 3) {
        *_json_output = static_cast<char*>(output->buf);
        std::free(output);
    }

    return ec;
}
