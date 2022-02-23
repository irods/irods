#ifndef EXEC_RULE_EXPRESSION_H
#define EXEC_RULE_EXPRESSION_H

#include "irods/rodsDef.h"

struct RcComm;
struct MsParamArray;

typedef struct ExecRuleExpression {
    bytesBuf_t           rule_text_;
    bytesBuf_t           packed_rei_;
    struct MsParamArray* params_;
} exec_rule_expression_t;

#define ExecRuleExpression_PI "struct BytesBuf_PI; struct BytesBuf_PI; struct *MsParamArray_PI;"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int rcExecRuleExpression(struct RcComm* _comm, struct ExecRuleExpression* _input);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // EXEC_RULE_EXPRESSION_H
