#ifndef SUB_STRUCT_FILE_READDIR_H__
#define SUB_STRUCT_FILE_READDIR_H__

#include "irods/rodsType.h"
#include "irods/rcConnect.h"
#include "irods/subStructFileRead.h"

int rcSubStructFileReaddir( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReaddirInp, rodsDirent_t **rodsDirent );

#endif
