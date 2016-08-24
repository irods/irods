#ifndef RS_GEN_QUERY_HPP
#define RS_GEN_QUERY_HPP

#include "rodsConnect.h"
#include "rodsGenQuery.h"

int rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut );
int _rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut );

#endif
