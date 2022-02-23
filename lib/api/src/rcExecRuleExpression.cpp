#include "irods/exec_rule_expression.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"

int rcExecRuleExpression(RcComm* _comm, ExecRuleExpression* _inp)
{
    return procApiRequest(_comm, EXEC_RULE_EXPRESSION_AN, _inp, nullptr, nullptr, nullptr);
}
