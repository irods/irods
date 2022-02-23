#include "irods/rs_register_physical_path.hpp"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"

#include "irods/irods_server_api_call.hpp"

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

