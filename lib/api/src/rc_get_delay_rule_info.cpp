#include "irods/get_delay_rule_info.h"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

#include <cstdlib>
#include <cstring>

auto rc_get_delay_rule_info(RcComm* _comm, const char* _rule_id, BytesBuf** _output) -> int
{
    if (!_rule_id || !_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm,
                          GET_DELAY_RULE_INFO_APN,
                          const_cast<char*>(_rule_id), // NOLINT(cppcoreguidelines-pro-type-const-cast)
                          nullptr,
                          reinterpret_cast<void**>(_output), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                          nullptr);
}
