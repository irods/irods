/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "ruleExecDel.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcRuleExecDel( rcComm_t *conn, ruleExecDelInp_t *ruleExecDelInp )
 *
 * \brief Delete a queued delayed execution rule on the iCAT.
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
 * \param[in] ruleExecDelInp
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcRuleExecDel( rcComm_t *conn, ruleExecDelInp_t *ruleExecDelInp ) {
    int status;
    status = procApiRequest( conn, RULE_EXEC_DEL_AN, ruleExecDelInp,
                             NULL, ( void ** ) NULL, NULL );

    return status;
}

