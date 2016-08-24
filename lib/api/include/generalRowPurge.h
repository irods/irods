#ifndef GENERAL_ROW_PURGE_H__
#define GENERAL_ROW_PURGE_H__

#include "rcConnect.h"

typedef struct {
    char *tableName;
    char *secondsAgo;
} generalRowPurgeInp_t;
#define generalRowPurgeInp_PI "str *tableName; str *secondsAgo;"

#ifdef __cplusplus
extern "C"
#endif
int rcGeneralRowPurge( rcComm_t *conn, generalRowPurgeInp_t *generalRowPurgeInp );

#endif
