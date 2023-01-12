#include "get_delay_rule_info.h"

#include "api_plugin_number.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

#include <cstdlib>
#include <cstring>

auto rc_get_delay_rule_info(RcComm* _comm, const char* _rule_id, BytesBuf** _output) -> int
{
    if (!_rule_id || !_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm, GET_DELAY_RULE_INFO_APN,
                          const_cast<char*>(_rule_id), nullptr,
                          reinterpret_cast<void**>(_output), nullptr);
}
