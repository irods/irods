#ifndef GET_HIERARCHY_FOR_RESC_H__
#define GET_HIERARCHY_FOR_RESC_H__


#include "rcConnect.h"

typedef struct GetHierarchyForRescInp {
    char resc_name_[ MAX_NAME_LEN ];
} getHierarchyForRescInp_t;
#define getHierarchyForRescInp_PI "str resc_name_[MAX_NAME_LEN];"

typedef struct GetHierarchyForRescOut {
    char resc_hier_[ MAX_NAME_LEN ];
} getHierarchyForRescOut_t;
#define getHierarchyForRescOut_PI "str resc_hier_[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcGetHierarchyForResc(rcComm_t* server_comm_ptr, getHierarchyForRescInp_t* incoming_resc_name, getHierarchyForRescOut_t** full_hier_to_resc);

#endif
