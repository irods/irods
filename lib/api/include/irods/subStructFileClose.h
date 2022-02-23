#ifndef SUB_STRUCT_FILE_CLOSE_H__
#define SUB_STRUCT_FILE_CLOSE_H__

#include "irods/rcConnect.h"
#include "irods/subStructFileRead.h"

int rcSubStructFileClose( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileCloseInp );

#endif
