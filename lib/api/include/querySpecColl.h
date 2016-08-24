#ifndef QUERY_SPEC_COLL_H__
#define QUERY_SPEC_COLL_H__

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsGenQuery.h"
#include "dataObjInpOut.h"

typedef struct specCollDesc {
    int l3descInx;
    int inuseFlag;
    dataObjInfo_t *dataObjInfo;
    int parentInx;
} specCollDesc_t;

#define NUM_SPEC_COLL_DESC    100
#define MAX_SPEC_COLL_ROW     100

#define QuerySpecCollInp_PI "str objPath[MAX_NAME_LEN]; int recurFlag; int type; int continueInx; struct SpecCollMeta_PI;"

#ifdef __cplusplus
extern "C"
#endif
int rcQuerySpecColl( rcComm_t *conn, dataObjInp_t *querySpecCollInp, genQueryOut_t **genQueryOut );

#endif
