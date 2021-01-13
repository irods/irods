#include "rs_atomic_apply_metadata_operations.hpp"

#include "api_plugin_number.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstdlib>
#include <cstring>

auto rs_atomic_apply_metadata_operations(RsComm* _comm, const char* _json_input, char** _json_output) -> int
{
    if (!_json_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input{};
    input.buf = const_cast<char*>(_json_input);
    input.len = static_cast<int>(std::strlen(_json_input)) + 1;

    bytesBuf_t* output{};

    const auto ec = irods::server_api_call(ATOMIC_APPLY_METADATA_OPERATIONS_APN, _comm, &input, &output);

    *_json_output = static_cast<char*>(output->buf);
    std::free(output);

    return ec;
}
