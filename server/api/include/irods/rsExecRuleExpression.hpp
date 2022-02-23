#ifndef RS_EXEC_RULE_EXPRESSION_HPP
#define RS_EXEC_RULE_EXPRESSION_HPP

#include "irods/exec_rule_expression.h"

struct RsComm;

int rsExecRuleExpression(RsComm* _comm, ExecRuleExpression* _input);

#endif // RS_EXEC_RULE_EXPRESSION_HPP
