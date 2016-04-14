
#include "exec_rule_expression.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int rcExecRuleExpression(
    rcComm_t*               _comm,
    exec_rule_expression_t* _inp) {
    return procApiRequest(
               _comm,
               EXEC_RULE_EXPRESSION_AN,
               _inp, NULL,
               NULL, NULL);
}
