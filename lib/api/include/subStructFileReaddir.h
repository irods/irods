#ifndef SUB_STRUCT_FILE_READDIR_H__
#define SUB_STRUCT_FILE_READDIR_H__

#include "rodsType.h"
#include "rcConnect.h"
#include "subStructFileRead.h"

int rcSubStructFileReaddir( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReaddirInp, rodsDirent_t **rodsDirent );

#endif
