#include "atomic_apply_metadata_operations.h"

#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

#include <cstring>

extern "C"
auto rc_atomic_apply_metadata_operations(rcComm_t* _comm, const char* _json_input, char** _json_output) -> int
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

    return ec;
}
