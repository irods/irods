#include "ruleExecMod.h"
#include "procApiRequest.h"
#include "apiNumber.h"

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
int
rcRuleExecMod( rcComm_t *conn, ruleExecModInp_t *ruleExecModInp ) {
    int status;
    status = procApiRequest( conn, RULE_EXEC_MOD_AN, ruleExecModInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
