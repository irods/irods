#include "irods/fileRename.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"

int rcFileRename(RcComm* conn, FileRenameInp* fileRenameInp, FileRenameOut** fileRenameOut)
{
    return procApiRequest(conn, FILE_RENAME_AN, fileRenameInp, nullptr, reinterpret_cast<void**>(fileRenameOut), nullptr);
}

