#include "irods/ruleExecMod.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"

/**
 * \fn rcRuleExecMod( rcComm_t *conn, ruleExecModInp_t *ruleExecModInp )
 *
 * \brief Modify a queued delayed execution rule on the iCAT.
 *
 * \user client
 *
 * \ingroup delayed_execution
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
*
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ruleExecModInp
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int rcRuleExecMod( RcComm* _comm, RuleExecModifyInput* _ruleExecModInp )
{
    return procApiRequest(_comm, RULE_EXEC_MOD_AN, _ruleExecModInp, nullptr, nullptr, nullptr);
}
