#ifndef STREAM_CLOSE_H__
#define STREAM_CLOSE_H__

#include "rcConnect.h"
#include "fileClose.h"


#ifdef __cplusplus
extern "C"
#endif
int rcStreamClose( rcComm_t *conn, fileCloseInp_t *fileCloseInp );

#endif
