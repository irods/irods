#include "rs_register_physical_path.hpp"

#include <memory>

auto rsPhyPathReg(RsComm* _comm, DataObjInp* _inp) -> int
{
    char* out{};

    const int rc = rs_register_physical_path(_comm, _inp, &out);

    std::free(out);
    out = nullptr;

    return rc;
} // rsPhyPathReg

