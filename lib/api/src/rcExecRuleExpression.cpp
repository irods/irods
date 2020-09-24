#include "exec_rule_expression.h"

#include "apiNumber.h"
#include "procApiRequest.h"

int rcExecRuleExpression(RcComm* _comm, ExecRuleExpression* _inp)
{
    return procApiRequest(_comm, EXEC_RULE_EXPRESSION_AN, _inp, nullptr, nullptr, nullptr);
}
