/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "ruleExecSubmit.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcRuleExecSubmit( rcComm_t *conn, ruleExecSubmitInp_t *ruleExecSubmitInp, char **ruleExecId )
 *
 * \brief Submit a rule to be queued on the iCAT for delayed execution.
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
 * \param[in] ruleExecSubmitInp
 * \param[out] ruleExecId
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcRuleExecSubmit( rcComm_t *conn, ruleExecSubmitInp_t *ruleExecSubmitInp,
                  char **ruleExecId ) {
    int status;
    status = procApiRequest( conn, RULE_EXEC_SUBMIT_AN, ruleExecSubmitInp,
                             NULL, ( void ** ) ruleExecId, NULL );

    return status;
}

