/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* specificQuery.h
 */

#ifndef SPECIFIC_QUERY_HPP
#define SPECIFIC_QUERY_HPP

/* This is a metadata type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

#include "rodsGenQuery.hpp"  /* for output struct, etc */

typedef struct {
    char *sql;
    char *args[10];  /* optional arguments (bind variables) */

    /* The following match items in genQueryInp_t: */
    int maxRows;             /* max number of rows to return, if 0
                               close out the SQL statement call (i.e. instead
                               of getting more rows until it is finished). */
    int continueInx;         /* if non-zero, this is the value returned in
                                the genQueryOut structure and the current
                                call is to get more rows. */
    int rowOffset;           /* Currently unused. */
    int options;             /* Bits for special options, currently unused. */
    keyValPair_t condInput;
} specificQueryInp_t;

#define specificQueryInp_PI "str *sql; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7; str *arg8; str *arg9; str *arg10; int maxRows; int continueInx; int rowOffset; int options; struct KeyValPair_PI;"

#if defined(RODS_SERVER)
#define RS_SPECIFIC_QUERY rsSpecificQuery
/* prototype for the server handler */
int
rsSpecificQuery( rsComm_t *rsComm, specificQueryInp_t *specificQueryInp,
                 genQueryOut_t **genQueryOut );
int
_rsSpecificQuery( rsComm_t *rsComm, specificQueryInp_t *specificQueryInp,
                  genQueryOut_t **genQueryOut );
#else
#define RS_SPECIFIC_QUERY NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcSpecificQuery( rcComm_t *conn, specificQueryInp_t *specificQueryInp,
                 genQueryOut_t **genQueryOut );

#ifdef __cplusplus
}
#endif
#endif	/* SPECIFIC_QUERY_H */
