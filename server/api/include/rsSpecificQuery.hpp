#ifndef RS_SPECIFIC_QUERY_HPP
#define RS_SPECIFIC_QUERY_HPP

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsGenQuery.h"

int rsSpecificQuery( rsComm_t *rsComm, specificQueryInp_t *specificQueryInp, genQueryOut_t **genQueryOut );
int _rsSpecificQuery( rsComm_t *rsComm, specificQueryInp_t *specificQueryInp, genQueryOut_t **genQueryOut );

#endif
