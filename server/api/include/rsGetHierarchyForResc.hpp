#ifndef RS_GET_HIERARCHY_FOR_RESC_HPP
#define RS_GET_HIERARCHY_FOR_RESC_HPP

#include "getHierarchyForResc.h"

int rsGetHierarchyForResc(rsComm_t* server_comm_ptr, getHierarchyForRescInp_t* incoming_resc_name, getHierarchyForRescOut_t** full_hier_to_resc);

#endif
