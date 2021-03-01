#include "fileChksum.h"

#include "apiNumber.h"
#include "procApiRequest.h"

int rcFileChksum(RcComm* conn, FileChksumInp* fileChksumInp, char** chksumStr)
{
    return procApiRequest(conn, FILE_CHKSUM_AN, fileChksumInp, nullptr, reinterpret_cast<void**>(chksumStr), nullptr);
}

