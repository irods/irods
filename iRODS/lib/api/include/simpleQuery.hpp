/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* simpleQuery.h
 */

#ifndef SIMPLE_QUERY_HPP
#define SIMPLE_QUERY_HPP

/* This is a high level file type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

typedef struct {
    char *sql;
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    int control;
    int form;
    int maxBufSize;
} simpleQueryInp_t;

#define simpleQueryInp_PI "str *sql; str *arg1; str *arg2; str *arg3; str *arg4; int control; int form; int maxBufSize;"

typedef struct {
    int control;
    char *outBuf;
} simpleQueryOut_t;

#define simpleQueryOut_PI "int control; str *outBuf;"

#if defined(RODS_SERVER)
#define RS_SIMPLE_QUERY rsSimpleQuery
/* prototype for the server handler */
int
rsSimpleQuery( rsComm_t *rsComm, simpleQueryInp_t *simpleQueryInp,
               simpleQueryOut_t **simpleQueryOut );
int
_rsSimpleQuery( rsComm_t *rsComm, simpleQueryInp_t *simpleQueryInp,
                simpleQueryOut_t **simpleQueryOut );
#else
#define RS_SIMPLE_QUERY NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcSimpleQuery( rcComm_t *conn, simpleQueryInp_t *simpleQueryInp,
               simpleQueryOut_t **simpleQueryOut );

#ifdef __cplusplus
}
#endif
#endif	/* SIMPLE_QUERY_H */
