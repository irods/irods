/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef GET_HOST_FOR_GET_HPP
#define GET_HOST_FOR_GET_HPP

/* This is a Object File I/O call */

#include "rods.h"
#include "rcMisc.hpp"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "dataObjWrite.hpp"
#include "dataObjClose.hpp"

#define MAX_HOST_TO_SEARCH	10

typedef struct {
    int numHost;
    int totalCount;
    int count[MAX_HOST_TO_SEARCH];
} hostSearchStat_t;

#if defined(RODS_SERVER)
#define RS_GET_HOST_FOR_GET rsGetHostForGet
/* prototype for the server handler */
int
rsGetHostForGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                 char **outHost );

#if 0 // unused #1472
int
getBestRescForGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                   rescInfo_t **outRescInfo );

int
getRescForGetInColl( rsComm_t *rsComm, collInp_t *collInp,
                     hostSearchStat_t *hostSearchStat );
int
getRescForGetInDataObj( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                        hostSearchStat_t *hostSearchStat );
#endif

#else
#define RS_GET_HOST_FOR_GET NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
int
rcGetHostForGet( rcComm_t *conn, dataObjInp_t *dataObjInp,
                 char **outHost );
#ifdef __cplusplus
}
#endif
#endif	/* GET_HOST_FOR_GET_H */
