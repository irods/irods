#ifndef EXEC_RULE_EXPRESSION_H
#define EXEC_RULE_EXPRESSION_H

#include "rodsDef.h"
#include "rodsType.h"
#include "rcConnect.h"
#include "msParam.h"

typedef struct {
    bytesBuf_t      rule_text_;
    bytesBuf_t      packed_rei_;
    msParamArray_t* params_;
} exec_rule_expression_t;

#define ExecRuleExpression_PI "struct BytesBuf_PI; struct BytesBuf_PI; struct *MsParamArray_PI;"

int rcExecRuleExpression(rcComm_t*,exec_rule_expression_t*);

#endif
