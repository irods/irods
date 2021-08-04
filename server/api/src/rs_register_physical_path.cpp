#include "rs_register_physical_path.hpp"

#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstdlib>
#include <cstring>

auto rs_register_physical_path(RsComm* _comm, DataObjInp* _in, char** _json_output) -> int
{
    if (!_in || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    BytesBuf* out{};

    const auto rc = irods::server_api_call_without_policy(REGISTER_PHYSICAL_PATH_APN,
                                                          _comm, _in, &out);


    if (out && out->buf) {
        *_json_output = static_cast<char*>(out->buf);
    }
    std::free(out);
    out = nullptr;

    return rc;
} // rs_register_physical_path

