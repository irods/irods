#include "rs_get_delay_rule_info.hpp"

#include "api_plugin_number.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstdlib>
#include <cstring>

auto rs_get_delay_rule_info(RsComm* _comm, const char* _rule_id, BytesBuf** _output) -> int
{
    if (!_rule_id || !_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return irods::server_api_call_without_policy(GET_DELAY_RULE_INFO_APN, _comm, _rule_id, _output);
}
