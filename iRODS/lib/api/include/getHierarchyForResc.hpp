#ifndef __GET_HIERARCHY_FOR_RESC_HPP__
#define __GET_HIERARCHY_FOR_RESC_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

typedef struct GetHierarchyForRescInp {
    char resc_name_[ MAX_NAME_LEN ];

} getHierarchyForRescInp_t;
#define getHierarchyForRescInp_PI "str resc_name_[MAX_NAME_LEN];"

typedef struct GetHierarchyForRescOut {
    char resc_hier_[ MAX_NAME_LEN ];

} getHierarchyForRescOut_t;
#define getHierarchyForRescOut_PI "str resc_hier_[MAX_NAME_LEN];"

// =-=-=-=-=-=-=-
// prototype for server
#if defined(RODS_SERVER)
#define RS_GET_HIER_FOR_RESC rsGetHierarchyForResc
int rsGetHierarchyForResc(
    rsComm_t*,                    // server comm ptr
    getHierarchyForRescInp_t*,    // incoming resc name
    getHierarchyForRescOut_t** ); // full hier to resc
#else
#define RS_GET_HIER_FOR_RESC NULL
#endif


#ifdef __cplusplus
extern "C" {
#endif
// =-=-=-=-=-=-=-
// prototype for client
int rcGetHierarchyForResc(
    rcComm_t*,                    // server comm ptr
    getHierarchyForRescInp_t*,    // incoming resc name
    getHierarchyForRescOut_t** ); // full hier to resc
#ifdef __cplusplus
}
#endif

#endif // __GET_HIERARCHY_FOR_RESC_HPP__



