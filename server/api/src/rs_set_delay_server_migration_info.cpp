#include "irods/rs_set_delay_server_migration_info.hpp"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/rodsErrorTable.h"

#include "irods/irods_server_api_call.hpp"

auto rs_set_delay_server_migration_info(RsComm* _comm, const DelayServerMigrationInput* _input) -> int
{
    if (!_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return irods::server_api_call_without_policy(SET_DELAY_SERVER_MIGRATION_INFO_APN, _comm, _input);
}

