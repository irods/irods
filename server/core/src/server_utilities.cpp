#include "server_utilities.hpp"

#include "dataObjInpOut.h"
#include "key_value_proxy.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

namespace irods
{
    auto is_force_flag_required(RsComm& _comm, const DataObjInp& _input) -> bool
    {
        namespace ix = irods::experimental;
        namespace fs = irods::experimental::filesystem;

        return fs::server::exists(_comm, _input.objPath) &&
               !ix::key_value_proxy{_input.condInput}.contains(FORCE_FLAG_KW);
    }
} // namespace irods

