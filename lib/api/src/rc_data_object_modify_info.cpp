#include "data_object_modify_info.h"

#include "api_plugin_number.h"
#include "procApiRequest.h"
#include "rodsErrorTable.h"

extern "C"
auto rc_data_object_modify_info(rcComm_t* comm, modDataObjMeta_t* input) -> int
{
    return input ? procApiRequest(comm, DATA_OBJECT_MODIFY_INFO_APN, input, nullptr, nullptr, nullptr)
                 : SYS_INVALID_INPUT_PARAM;
}
