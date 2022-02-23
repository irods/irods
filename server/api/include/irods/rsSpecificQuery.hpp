#ifndef RS_SPECIFIC_QUERY_HPP
#define RS_SPECIFIC_QUERY_HPP

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/rodsGenQuery.h"

int rsSpecificQuery( rsComm_t *rsComm, specificQueryInp_t *specificQueryInp, genQueryOut_t **genQueryOut );
int _rsSpecificQuery( rsComm_t *rsComm, specificQueryInp_t *specificQueryInp, genQueryOut_t **genQueryOut );

#endif
