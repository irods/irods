#ifndef RS_SIMPLE_QUERY_HPP
#define RS_SIMPLE_QUERY_HPP

#include "rcConnect.h"
#include "simpleQuery.h"

int rsSimpleQuery( rsComm_t *rsComm, simpleQueryInp_t *simpleQueryInp, simpleQueryOut_t **simpleQueryOut );
int _rsSimpleQuery( rsComm_t *rsComm, simpleQueryInp_t *simpleQueryInp, simpleQueryOut_t **simpleQueryOut );

#endif
