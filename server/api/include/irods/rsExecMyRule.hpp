#ifndef RS_EXEC_MY_RULE_HPP
#define RS_EXEC_MY_RULE_HPP

#include "irods/objInfo.h"
#include "irods/msParam.h"
#include "irods/rcConnect.h"
#include "irods/execMyRule.h"

int rsExecMyRule( rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp, msParamArray_t **outParamArray );
int remoteExecMyRule( rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp, msParamArray_t **outParamArray, rodsServerHost_t *rodsServerHost );

#endif
