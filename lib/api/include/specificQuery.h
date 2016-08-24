#ifndef SPECIFIC_QUERY_H__
#define SPECIFIC_QUERY_H__

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsGenQuery.h"

typedef struct {
    char *sql;
    char *args[10];  // optional arguments (bind variables)

    // The following match items in genQueryInp_t:
    int maxRows;             // max number of rows to return, if 0 close out the SQL statement call (i.e. instead of getting more rows until it is finished).
    int continueInx;         // if non-zero, this is the value returned in the genQueryOut structure and the current call is to get more rows.
    int rowOffset;           // Currently unused.
    int options;             // Bits for special options, currently unused.
    keyValPair_t condInput;
} specificQueryInp_t;
#define specificQueryInp_PI "str *sql; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7; str *arg8; str *arg9; str *arg10; int maxRows; int continueInx; int rowOffset; int options; struct KeyValPair_PI;"

#ifdef __cplusplus
extern "C"
#endif
int rcSpecificQuery( rcComm_t *conn, specificQueryInp_t *specificQueryInp, genQueryOut_t **genQueryOut );

#endif
