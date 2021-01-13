#include "get_file_descriptor_info.h"

#include "api_plugin_number.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

#include <cstdlib>
#include <cstring>

auto rc_get_file_descriptor_info(RcComm* _comm, const char* _json_input, char** _json_output) -> int
{
    if (!_json_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input_buf{};
    input_buf.buf = const_cast<char*>(_json_input);
    input_buf.len = static_cast<int>(std::strlen(_json_input));

    bytesBuf_t* output_buf{};

    const int ec = procApiRequest(_comm, GET_FILE_DESCRIPTOR_INFO_APN,
                                  &input_buf, nullptr,
                                  reinterpret_cast<void**>(&output_buf), nullptr);

    if (ec == 0) {
        *_json_output = static_cast<char*>(output_buf->buf);
        std::free(output_buf);
    }

    return ec;
}
