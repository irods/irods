#include "rs_data_object_finalize.hpp"

#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstdlib>
#include <cstring>

auto rs_data_object_finalize(RsComm* _comm, const char* _json_input, char** _json_output) -> int
{
    if (!_json_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input{};
    input.buf = const_cast<char*>(_json_input);
    input.len = static_cast<int>(std::strlen(_json_input)) + 1;

    bytesBuf_t* output{};

    const auto ec = irods::server_api_call_without_policy(DATA_OBJECT_FINALIZE_APN,
                                                          _comm, &input, &output);

    *_json_output = static_cast<char*>(output->buf);
    std::free(output);

    return ec;
} // rs_data_object_finalize

