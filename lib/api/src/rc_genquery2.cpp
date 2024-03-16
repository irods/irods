#include "irods/genquery2.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_genquery2(RcComm* _comm, Genquery2Input* _input, char** _output) -> int
{
    if (!_comm || !_input || !_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm,
                          GENQUERY2_AN,
                          _input,
                          nullptr,
                          reinterpret_cast<void**>(_output), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                          nullptr);
} // rc_genquery2
