/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef L3_FILE_PUT_SINGLE_BUF_HPP
#define L3_FILE_PUT_SINGLE_BUF_HPP

/* This is a Object File I/O call */

#include "rods.h"
#include "rcMisc.hpp"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "dataObjPut.hpp"

#if defined(RODS_SERVER)
#define RS_L3_FILE_PUT_SINGLE_BUF rsL3FilePutSingleBuf
/* prototype for the server handler */
int
rsL3FilePutSingleBuf( rsComm_t *rsComm, int *l1descInx,
                      bytesBuf_t *dataObjInBBuf );
#else
#define RS_L3_FILE_PUT_SINGLE_BUF NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
int
rcL3FilePutSingleBuf( rcComm_t *conn, int l1descInx,
                      bytesBuf_t *dataObjInBBuf );
#ifdef __cplusplus
}
#endif
#endif	/* L3_FILE_PUT_SINGLE_BUF_H */
