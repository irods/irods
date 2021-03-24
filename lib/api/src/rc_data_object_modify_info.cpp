#include "data_object_modify_info.h"

#include "api_plugin_number.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

auto rc_data_object_modify_info(RcComm* _comm, ModDataObjMetaInp* _input) -> int
{
    return _input ? procApiRequest(_comm, DATA_OBJECT_MODIFY_INFO_APN, _input, nullptr, nullptr, nullptr)
                  : SYS_INVALID_INPUT_PARAM;
}
