#ifndef GET_HOST_FOR_GET_H__
#define GET_HOST_FOR_GET_H__

#include "rcConnect.h"
#include "dataObjClose.h"

#define MAX_HOST_TO_SEARCH      10

typedef struct {
    int numHost;
    int totalCount;
    int count[MAX_HOST_TO_SEARCH];
} hostSearchStat_t;

#ifdef __cplusplus
extern "C"
#endif
int rcGetHostForGet( rcComm_t *conn, dataObjInp_t *dataObjInp, char **outHost );

#endif
