/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef RULE_EXEC_DEL_H__
#define RULE_EXEC_DEL_H__

/* This is Object File I/O type API call */

#include "rcConnect.h"
#include "rodsDef.h"

typedef struct {
    char ruleExecId[NAME_LEN];
} ruleExecDelInp_t;

#define RULE_EXEC_DEL_INP_PI "str ruleExecId[NAME_LEN];"

#if defined(RODS_SERVER)
#define RS_RULE_EXEC_DEL rsRuleExecDel
/* prototype for the server handler */
int
rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp );
int
_rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp );
#else
#define RS_RULE_EXEC_DEL NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
int
rcRuleExecDel( rcComm_t *conn, ruleExecDelInp_t *ruleExecDelInp );

#ifdef __cplusplus
}
#endif
#endif	// RULE_EXEC_DEL_H__
