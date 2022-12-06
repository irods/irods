#ifndef IRODS_FILE_PUT_H
#define IRODS_FILE_PUT_H

#include "irods/rcConnect.h"
#include "irods/rodsDef.h"
#include "irods/fileOpen.h"

typedef struct FilePutOut
{
    char file_name[MAX_NAME_LEN];
} filePutOut_t;
#define filePutOut_PI "str file_name[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFilePut( rcComm_t *conn, fileOpenInp_t *filePutInp, bytesBuf_t *filePutInpBBuf, filePutOut_t** );

#endif // IRODS_FILE_PUT_H
