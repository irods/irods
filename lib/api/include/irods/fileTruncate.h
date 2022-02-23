#ifndef FILE_TRUNCATE_H__
#define FILE_TRUNCATE_H__

#include "irods/rcConnect.h"
#include "irods/fileOpen.h"

#ifdef __cplusplus
extern "C"
#endif
int rcFileTruncate( rcComm_t *conn, fileOpenInp_t *fileTruncateInp );

#endif
