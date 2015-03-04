#ifndef SET_ROUND_ROBIN_CONTEXT_HPP__
#define SET_ROUND_ROBIN_CONTEXT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"

/// =-=-=-=-=-=-=-
/// @brief input structure for rsSetRoundRobinContext API plugin
typedef struct {
    char resc_name_[ NAME_LEN ];
    char context_  [ MAX_NAME_LEN ];

} setRoundRobinContextInp_t;

/// =-=-=-=-=-=-=-
/// @brief input packing structure for rsSetRoundRobinContext API plugin
#define SetRoundRobinContextInp_PI "str resc_name_[NAME_LEN]; str context_[MAX_NAME_LEN];"

/// =-=-=-=-=-=-=-
/// @brief rsSetRoundRobinContext API index
#define SET_RR_CTX_AN 5000

#endif // SET_ROUND_ROBIN_CONTEXT_HPP__



