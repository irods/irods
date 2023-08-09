#include "irods/get_resource_info_for_operation.h"

#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

auto rc_get_resource_info_for_operation(RcComm* _comm, const DataObjInp* _dataObjInp, char** _out_info) -> int
{
    if (!_comm || !_dataObjInp || !_out_info) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm,
                          GET_RESOURCE_INFO_FOR_OPERATION_AN,
                          _dataObjInp,
                          nullptr,
                          reinterpret_cast<void**>(_out_info), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                          nullptr);
} // rc_get_resource_info_for_operation
