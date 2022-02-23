#ifndef RS_STREAM_CLOSE_HPP
#define RS_STREAM_CLOSE_HPP

#include "irods/rcConnect.h"
#include "irods/fileClose.h"

int rsStreamClose( rsComm_t *rsComm, fileCloseInp_t *fileCloseInp );

#endif
