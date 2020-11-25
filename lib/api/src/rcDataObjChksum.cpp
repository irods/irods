#include "dataObjChksum.h"

#include "apiNumber.h"
#include "procApiRequest.h"

int rcDataObjChksum(RcComm* comm, DataObjInp* dataObjChksumInp, char** outChksum)
{
    return procApiRequest(comm, DATA_OBJ_CHKSUM_AN, dataObjChksumInp, nullptr, (void**) outChksum, nullptr);
}

