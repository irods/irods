

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

#ifdef RODS_SERVER
    #define RS_EXEC_RULE_EXPRESSION rsExecRuleExpression
    int rsExecRuleExpression(rsComm_t*,exec_rule_expression_t*);
#else
    #define RS_EXEC_RULE_EXPRESSION  NULL
#endif

int rcExecRuleExpression(rcComm_t*,exec_rule_expression_t*);

#endif // EXEC_RULE_EXPRESSION_H


