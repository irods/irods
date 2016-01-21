#ifndef SET_ROUND_ROBIN_CONTEXT_HPP
#define SET_ROUND_ROBIN_CONTEXT_HPP

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "rcConnect.h"

/// =-=-=-=-=-=-=-
/// @brief input structure for rsSetRoundRobinContext API plugin
typedef struct {
    char resc_name_[ NAME_LEN ];
    char context_  [ MAX_NAME_LEN ];
} setRoundRobinContextInp_t;

/// =-=-=-=-=-=-=-
/// @brief input packing structure for rsSetRoundRobinContext API plugin
#define SetRoundRobinContextInp_PI "str resc_name_[NAME_LEN]; str context_[MAX_NAME_LEN];"

#ifdef RODS_SERVER
    #define RS_SET_ROUNDROBIN_CONTEXT rsSetRoundRobinContext
    int rsSetRoundRobinContext(rsComm_t*,setRoundRobinContextInp_t*);
#else
    #define RS_SET_ROUNDROBIN_CONTEXT NULL
#endif
    
int rcSetRoundRobinContext(rcComm_t*,setRoundRobinContextInp_t*); 

#endif // SET_ROUND_ROBIN_CONTEXT_HPP



