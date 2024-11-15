#include "irods/delay_rule_lock.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_delay_rule_lock(RcComm* _comm, DelayRuleLockInput* _input) -> int
{
    if (!_comm || !_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm, DELAY_RULE_LOCK_AN, _input, nullptr, nullptr, nullptr);
} // rc_delay_rule_lock
