#ifndef RS_FILE_PUT_HPP
#define RS_FILE_PUT_HPP

#include "irods/rodsConnect.h"
#include "irods/rodsDef.h"
#include "irods/fileOpen.h"
#include "irods/filePut.h"

int rsFilePut( rsComm_t *rsComm, fileOpenInp_t *filePutInp, bytesBuf_t *filePutInpBBuf, filePutOut_t** );
int _rsFilePut( rsComm_t *rsComm, fileOpenInp_t *filePutInp, bytesBuf_t *filePutInpBBuf, rodsServerHost_t *rodsServerHost, filePutOut_t** );
int remoteFilePut( rsComm_t *rsComm, fileOpenInp_t *filePutInp, bytesBuf_t *filePutInpBBuf, rodsServerHost_t *rodsServerHost, filePutOut_t** );

#endif
