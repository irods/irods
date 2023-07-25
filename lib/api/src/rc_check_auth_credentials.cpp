#include "irods/check_auth_credentials.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_check_auth_credentials(RcComm* _comm, CheckAuthCredentialsInput* _input, int** _correct) -> int
{
    if (!_comm || !_input || !_correct) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm,
                          CHECK_AUTH_CREDENTIALS_AN,
                          _input,
                          nullptr,
                          reinterpret_cast<void**>(_correct), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                          nullptr);
} // rc_check_auth_credentials
