/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef DEBUG_HPP
#define DEBUG_HPP
#include "irods/private/re/restructs.hpp"
int pushReStack( RuleEngineEvent label, char* step );
int popReStack( RuleEngineEvent label, char *step );
int reDebug( RuleEngineEvent callLabel, int flag, RuleEngineEventParam *param, Node *node, Env *env, ruleExecInfo_t *rei );
int initializeReDebug( rsComm_t *svrComm );

#endif
