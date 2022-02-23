#include "irods/fileChksum.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"

int rcFileChksum(RcComm* conn, FileChksumInp* fileChksumInp, char** chksumStr)
{
    return procApiRequest(conn, FILE_CHKSUM_AN, fileChksumInp, nullptr, reinterpret_cast<void**>(chksumStr), nullptr);
}

