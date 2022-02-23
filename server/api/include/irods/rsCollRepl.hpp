#ifndef RS_COLL_REPL_HPP
#define RS_COLL_REPL_HPP

#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"
#include "irods/rcConnect.h"

int rsCollRepl( rsComm_t *rsComm, collInp_t *collReplInp, collOprStat_t **collOprStat );

#endif
