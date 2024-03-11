#ifndef RS_DATA_OBJ_TRUNCATE_HPP
#define RS_DATA_OBJ_TRUNCATE_HPP

#include "irods/rodsConnect.h"
#include "irods/dataObjInpOut.h"
#include "irods/objInfo.h"

// clang-format off

[[deprecated("rsDataObjTruncate is deprecated. Use rs_replica_truncate instead.")]]
int rsDataObjTruncate(rsComm_t* rsComm, dataObjInp_t* dataObjInp);

// clang-format on

#endif
