#include "irods/data_object_modify_info.h"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_data_object_modify_info(RcComm* _comm, ModDataObjMetaInp* _input) -> int
{
    if (nullptr == _comm || nullptr == _input) {
        return INVALID_INPUT_ARGUMENT_NULL_POINTER;
    }

    return _input ? procApiRequest(_comm, DATA_OBJECT_MODIFY_INFO_APN, _input, nullptr, nullptr, nullptr)
                  : SYS_INVALID_INPUT_PARAM;
}
