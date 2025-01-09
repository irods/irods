#ifndef RS_GENERAL_ROW_PURGE_HPP
#define RS_GENERAL_ROW_PURGE_HPP

#include "irods/rcConnect.h"
#include "irods/generalRowPurge.h"

[[deprecated]] int rsGeneralRowPurge(rsComm_t* rsComm, generalRowPurgeInp_t* generalRowPurgeInp);
[[deprecated]] int _rsGeneralRowPurge(rsComm_t* rsComm, generalRowPurgeInp_t* generalRowPurgeInp);

#endif
