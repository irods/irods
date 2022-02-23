#ifndef COLL_REPL_H__
#define COLL_REPL_H__

#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"
#include "irods/rcConnect.h"

#ifdef __cplusplus
extern "C"
#endif
int rcCollRepl( rcComm_t *conn, collInp_t *collReplInp, int vFlag );
int _rcCollRepl( rcComm_t *conn, collInp_t *collReplInp, collOprStat_t **collOprStat );

#endif
