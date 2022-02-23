#ifndef RS_BULK_DATA_OBJ_PUT_HPP
#define RS_BULK_DATA_OBJ_PUT_HPP

#include "irods/rodsDef.h"
#include "irods/rcConnect.h"
#include "irods/bulkDataObjPut.h"

int rsBulkDataObjPut(rsComm_t *rsComm, bulkOprInp_t *bulkOprInp, bytesBuf_t *bulkOprInpBBuf);

#endif
