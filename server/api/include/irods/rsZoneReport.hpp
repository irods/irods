#ifndef RS_ZONE_REPORT_HPP
#define RS_ZONE_REPORT_HPP

#include "irods/rcConnect.h"
#include "irods/objInfo.h"

int rsZoneReport(rsComm_t* server_comm_ptr, bytesBuf_t** json_response);

#endif
