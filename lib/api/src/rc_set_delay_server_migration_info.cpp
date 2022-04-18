#include "irods/set_delay_server_migration_info.h"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_set_delay_server_migration_info(RcComm* _comm, const DelayServerMigrationInput* _input) -> int
{
    if (!_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm, SET_DELAY_SERVER_MIGRATION_INFO_APN, _input, nullptr, nullptr, nullptr);
}
