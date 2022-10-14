#ifndef RS_SIMPLE_QUERY_HPP
#define RS_SIMPLE_QUERY_HPP

#include "irods/rcConnect.h"
#include "irods/simpleQuery.h"

// clang-format off

[[deprecated("SimpleQuery is deprecated. Use GenQuery or SpecificQuery instead.")]]
auto rsSimpleQuery(rsComm_t* rsComm, simpleQueryInp_t* simpleQueryInp, simpleQueryOut_t** simpleQueryOut) -> int;

// clang-format on

#endif
