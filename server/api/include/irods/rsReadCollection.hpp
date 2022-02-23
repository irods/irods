#ifndef RS_READ_COLLECTION_HPP
#define RS_READ_COLLECTION_HPP

#include "irods/rcConnect.h"
#include "irods/miscUtil.h"

int rsReadCollection( rsComm_t *rsComm, int *handleInxInp, collEnt_t **collEnt );

#endif
