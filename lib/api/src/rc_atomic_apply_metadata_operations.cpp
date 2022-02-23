#include "irods/atomic_apply_metadata_operations.h"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

#include <cstdlib>
#include <cstring>

auto rc_atomic_apply_metadata_operations(RcComm* _comm, const char* _json_input, char** _json_output) -> int
{
    if (!_json_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input_buf{};
    input_buf.buf = const_cast<char*>(_json_input);
    input_buf.len = static_cast<int>(std::strlen(_json_input)) + 1;

    bytesBuf_t* output_buf{};

    const int ec = procApiRequest(_comm, ATOMIC_APPLY_METADATA_OPERATIONS_APN,
                                  &input_buf, nullptr,
                                  reinterpret_cast<void**>(&output_buf), nullptr);

    *_json_output = static_cast<char*>(output_buf->buf);
    std::free(output_buf);

    return ec;
}
