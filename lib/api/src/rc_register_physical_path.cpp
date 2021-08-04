#include "register_physical_path.h"

#include "api_plugin_number.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

#include <cstdlib>
#include <cstring>

auto rc_register_physical_path(RcComm* _comm, DataObjInp* _in, char** _json_output) -> int
{
    if (!_in || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    BytesBuf* out{};

    const int ec = procApiRequest(_comm, REGISTER_PHYSICAL_PATH_APN,
                                  _in, nullptr,
                                  reinterpret_cast<void**>(&out), nullptr);

    if (out && out->buf) {
        *_json_output = static_cast<char*>(out->buf);
        std::free(out);
        out = nullptr;
    }

    return ec;
}
