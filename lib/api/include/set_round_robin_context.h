#ifndef SET_ROUND_ROBIN_CONTEXT_HPP
#define SET_ROUND_ROBIN_CONTEXT_HPP

#include "rodsDef.h"
#include "rcConnect.h"

typedef struct {
    char resc_name_[ NAME_LEN ];
    char context_  [ MAX_NAME_LEN ];
} setRoundRobinContextInp_t;
#define SetRoundRobinContextInp_PI "str resc_name_[NAME_LEN]; str context_[MAX_NAME_LEN];"


int rcSetRoundRobinContext(rcComm_t*,setRoundRobinContextInp_t*);

#endif
