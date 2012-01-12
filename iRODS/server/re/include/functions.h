/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef FUNCTIONS_H
#define	FUNCTIONS_H

#include "hashtable.h"
#include "region.h"
#include "parser.h"




/*Hashtable *getSystemFunctionTypes(Region *r); */
void getSystemFunctions(Hashtable *ft, Region *r);

Res* eval(char *expr, Env *env, ruleExecInfo_t *rei, int saveREI, rError_t *errmsg, Region *r);

int getParamIOType(char *iotypes, int index);

FunctionDesc *getFuncDescFromChain(int n, FunctionDesc *fDesc);
Node *construct(char *fn, Node **args, int argc, Node* constype, Region *r);
Node *deconstruct(char *fn, Node **args, int argc, int proj, rError_t*errmsg, Region *r);
#endif	/* FUNCTIONS_H */

