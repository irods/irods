/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ruleExecMod.h
 */

#ifndef RULE_EXEC_MOD_H
#define RULE_EXEC_MOD_H

/* This is a Metadata type API call */
#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "reGlobalsExtern.h"

typedef struct {
   char ruleId[NAME_LEN];
   keyValPair_t condInput;
} ruleExecModInp_t;

#define RULE_EXEC_MOD_INP_PI "str ruleId[NAME_LEN];struct KeyValPair_PI;"

#if defined(RODS_SERVER)
#define RS_RULE_EXEC_MOD rsRuleExecMod
/* prototype for the server handler */
int
rsRuleExecMod (rsComm_t *rsComm, ruleExecModInp_t *ruleExecModInp);
int
_rsRuleExecMod (rsComm_t *rsComm, ruleExecModInp_t *ruleExecModInp);
#else
#define RS_RULE_EXEC_MOD NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client */
int
rcRuleExecMod (rcComm_t *conn, ruleExecModInp_t *ruleExecModInp);

#ifdef  __cplusplus
}
#endif

#endif	/* RULE_EXEC_MOD_H */
