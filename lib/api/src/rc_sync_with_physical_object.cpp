#include "sync_with_physical_object.h"

#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

#include <cstring>

auto rc_sync_with_physical_object(rcComm_t* _comm, const char* _json_input) -> int
{
    if (!_json_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input_buf{};
    input_buf.len = static_cast<int>(std::strlen(_json_input));
    input_buf.buf = const_cast<char*>(_json_input);

    return procApiRequest(_comm, SYNC_WITH_PHYSICAL_OBJECT_APN, &input_buf,
                          nullptr, nullptr, nullptr);
}
