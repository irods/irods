#include "rs_get_file_descriptor_info.hpp"

#include "api_plugin_number.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstdlib>
#include <cstring>

auto rs_get_file_descriptor_info(RsComm* _comm, const char* _json_input, char** _json_output) -> int
{
    if (!_json_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input{};
    input.buf = const_cast<char*>(_json_input);
    input.len = static_cast<int>(std::strlen(_json_input));

    bytesBuf_t* output{};

    const auto ec = irods::server_api_call(GET_FILE_DESCRIPTOR_INFO_APN, _comm, &input, &output);

    if (ec == 0) {
        *_json_output = static_cast<char*>(output->buf);
        std::free(output);
    }

    return ec;
}

