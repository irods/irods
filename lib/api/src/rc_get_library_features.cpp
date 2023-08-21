#include "irods/get_library_features.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_get_library_features(RcComm* _comm, char** _features) -> int
{
    if (!_comm || !_features) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm,
                          GET_LIBRARY_FEATURES_AN,
                          nullptr,
                          nullptr,
                          reinterpret_cast<void**>(_features), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                          nullptr);
} // rc_get_library_features
