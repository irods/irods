#ifndef GENERAL_QUERY_H__
#define GENERAL_QUERY_H__

#include "rcConnect.h"
#include "rodsGenQuery.h"

#ifdef __cplusplus
extern "C"
#endif
int rcGenQuery( rcComm_t *conn, genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut );

#endif
