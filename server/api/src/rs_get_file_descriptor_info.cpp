#include "irods/rs_get_file_descriptor_info.hpp"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/rodsErrorTable.h"

#include "irods/irods_server_api_call.hpp"

#include <cstdlib>
#include <cstring>

auto rs_get_file_descriptor_info(RsComm* _comm, const char* _json_input, char** _json_output) -> int
{
    if (!_json_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input{};
    input.buf = const_cast<char*>(_json_input);
    input.len = static_cast<int>(std::strlen(_json_input)) + 1;

    bytesBuf_t* output{};

    const auto ec = irods::server_api_call_without_policy(GET_FILE_DESCRIPTOR_INFO_APN,
                                                          _comm, &input, &output);

    if (ec == 0) {
        *_json_output = static_cast<char*>(output->buf);
        std::free(output);
    }

    return ec;
}

