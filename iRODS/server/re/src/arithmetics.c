/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include "utils.h"
#include "restructs.h"
#include "parser.h"
#include "arithmetics.h"
#include "datetime.h"
#include "index.h"
#include "rules.h"
#include "functions.h"
#include "configuration.h"





#ifdef USE_EIRODS
//	#include "eirods_ms_plugin.h"
//	extern eirods::ms_table MicrosTable;
//	extern int NumOfAction;
#else
	#ifndef DEBUG
	#include "regExpMatch.h"
	#include "rodsErrorTable.h"
	typedef struct {
	  char action[MAX_ACTION_SIZE];
	  int numberOfStringArgs;
	  funcPtr callAction;
	} microsdef_t;
	extern int NumOfAction;
	extern microsdef_t MicrosTable[];
	#endif
#endif // ifdef USE_EIRODS

#define RE_ERROR(x, y) if(x) {if((y)!=NULL){(y)->type.t=RE_ERROR;*errnode=node;}return;}
#define OUTOFMEMORY(x, res) if(x) {(res)->value.e = OUT_OF_MEMORY;TYPE(res) = RE_ERROR;return;}

#define RE_ERROR2(x,y) if(x) {localErrorMsg=(y);goto error;}
extern int GlobalREDebugFlag;
extern int GlobalREAuditFlag;

/* utilities */
int initializeEnv(Node *params, Res *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc, Hashtable *env, Region *r) {


	Node** args2 = params->subtrees;
/*	int argc2 = ruleHead->degree; */
	int i;
        /*getSystemFunctions(env, r); */
	for (i = 0; i < argc ; i++) {
		insertIntoHashTable(env, args2[i]->text, args[i]);
	}
	return (0);
}

char *matchWholeString(char *buf) {
    char *buf2 = (char *)malloc(sizeof(char)*strlen(buf)+2+1);
    buf2[0]='^';
    strcpy(buf2+1, buf);
    buf2[strlen(buf)+1]='$';
    buf2[strlen(buf)+2]='\0';
    return buf2;
}

char *wildCardToRegex(char *buf) {
    char *buf2 = (char *)malloc(sizeof(char)*strlen(buf)*3+2+1);
    char *p = buf2;
    int i;
    *(p++)='^';
    int n = strlen(buf);
    for(i=0;i<n;i++) {
    	switch(buf[i]) {
    		case '*':
    			*(p++) = '.';
    			*(p++) = buf[i];
    			break;
    		case ']':
    		case '[':
    		case '^':
    			*(p++) = '\\';
    			*(p++) = buf[i];
    			break;
    		default:
    			*(p++) = '[';
    			*(p++) = buf[i];
    			*(p++) = ']';
    			break;
    	}
    }
    *(p++)='$';
    *(p++)='\0';
    return buf2;
}

char* getVariableName(Node *node) {
    return node->subtrees[0]->text;
}


Res* evaluateExpression3(Node *expr, int applyAll, int force, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r) {
/*
    printTree(expr, 0);
*/
    char errbuf[ERR_MSG_LEN];
    Res *res = newRes(r), *funcRes = NULL, *argRes = NULL;
    FunctionDesc *fd = NULL;
    int i;
    Res **tupleComps = NULL;
    /* Only input parameters are evaluated here;
     * The original AST node is needed for parameters of other IO types in evaluateFunction3.
     */
    if(force || getIOType(expr) == IO_TYPE_INPUT ) {
		switch(getNodeType(expr)) {
			case TK_BOOL:
				res->exprType = newSimpType(T_BOOL, r);
				RES_BOOL_VAL_LVAL(res) = strcmp(expr->text, "true")==0?1:0;
				break;
			case TK_INT:
				res->exprType = newSimpType(T_INT,r);
				RES_INT_VAL_LVAL(res)=atoi(expr->text);
				break;
			case TK_DOUBLE:
				res->exprType = newSimpType(T_DOUBLE,r);
				RES_DOUBLE_VAL_LVAL(res)=atof(expr->text);
				break;
			case TK_STRING:
				res = newStringRes(r, expr->text);
				break;
			case TK_VAR:
				res = evaluateVar3(expr->text, expr, rei, reiSaveFlag,  env, errmsg,r);
				break;
			case TK_TEXT:
					fd = (FunctionDesc *)lookupFromEnv(ruleEngineConfig.extFuncDescIndex, expr->text);
					if(fd!=NULL && fd->exprType != NULL) {
						int nArgs = 0;
						ExprType *type = fd->exprType;
						while(getNodeType(type) == T_CONS && strcmp(type->text, FUNC) == 0) {
							type = type->subtrees[1];
							nArgs ++;
						}
						if(nArgs == 0) {
							Node *appNode = newPartialApplication(expr, newTupleRes(0,NULL,r), 0, r);
							res = evaluateFunction3(appNode, applyAll, expr, env, rei, reiSaveFlag, errmsg, r);
						} else {
							res = newFuncSymLink(expr->text, nArgs, r);
						}
					} else {
						res = newFuncSymLink(expr->text, 1, r);
					}
					break;


			case N_APPLICATION:
							/* try to evaluate as a function, */
	/*
							printf("start execing %s\n", oper1);
							printEnvToStdOut(env);

	*/			funcRes = evaluateExpression3(expr->subtrees[0], applyAll > 1 ? applyAll : 0, 0, rei, reiSaveFlag, env, errmsg,r);
				if(getNodeType(funcRes)==N_ERROR) {
					res = funcRes;
					break;
				}
				/* printTree(expr->subtrees[1], 0); */
				argRes = evaluateExpression3(expr->subtrees[1], applyAll > 1 ? applyAll : 0, 0, rei, reiSaveFlag, env, errmsg,r);
				if(getNodeType(argRes)==N_ERROR) {
					res = argRes;
					break;
				}
				res = evaluateFunctionApplication(funcRes, argRes, applyAll, expr, rei, reiSaveFlag, env, errmsg,r);
	/*
							printf("finish execing %s\n", oper1);
							printEnvToStdOut(env);
	*/
							break;
			case N_TUPLE:
				tupleComps = (Res **) region_alloc(r, sizeof(Res *) * expr->degree);
				for(i=0;i<expr->degree;i++) {
					res = tupleComps[i] = evaluateExpression3(expr->subtrees[i], applyAll > 1 ? applyAll : 0, 0, rei, reiSaveFlag, env, errmsg, r);
					if(getNodeType(res) == N_ERROR) {
						break;
					}
				}
				if(expr->degree == 0 || getNodeType(res) != N_ERROR) {
					if(N_TUPLE_CONSTRUCT_TUPLE(expr) || expr->degree != 1) {
						res = newTupleRes(expr->degree, tupleComps, r);
					}
				}
				break;
			case N_ACTIONS_RECOVERY:
							res = evaluateActions(expr->subtrees[0], expr->subtrees[1], applyAll, rei, reiSaveFlag, env, errmsg, r);
							break;

			case N_ACTIONS:
							generateErrMsg("error: evaluate actions using function evaluateExpression3, use function evaluateActions instead.", NODE_EXPR_POS(expr), expr->base, errbuf);
							addRErrorMsg(errmsg, RE_UNSUPPORTED_AST_NODE_TYPE, errbuf);
							res = newErrorRes(r, RE_UNSUPPORTED_AST_NODE_TYPE);
							break;
					default:
							generateErrMsg("error: unsupported ast node type.", NODE_EXPR_POS(expr), expr->base, errbuf);
							addRErrorMsg(errmsg, RE_UNSUPPORTED_AST_NODE_TYPE, errbuf);
							res = newErrorRes(r, RE_UNSUPPORTED_AST_NODE_TYPE);
							break;
		}
    } else {
    	res = expr;
    	while(getNodeType(res) == N_TUPLE && res->degree == 1) {
    		res = res->subtrees[0];
    	}
    }
        /* coercions are applied at application locations only */
        return res;
}

Res* processCoercion(Node *node, Res *res, ExprType *type, Hashtable *tvarEnv, rError_t *errmsg, Region *r) {
        char buf[ERR_MSG_LEN>1024?ERR_MSG_LEN:1024];
        char *buf2;
        char buf3[ERR_MSG_LEN];
        ExprType *coercion = type;
        if(getNodeType(coercion) == T_FLEX) {
            coercion = coercion->subtrees[0];
        } else if(getNodeType(coercion) == T_FIXD) {
        	coercion = coercion->subtrees[1];
        }
        coercion = instantiate(coercion, tvarEnv, 0, r);
        if(getNodeType(coercion) == T_VAR) {
            if(T_VAR_NUM_DISJUNCTS(coercion) == 0) {
                /* generateErrMsg("error: cannot instantiate coercion type for node.", NODE_EXPR_POS(node), node->base, buf);
                addRErrorMsg(errmsg, -1, buf);
                return newErrorRes(r, -1); */
                return res;
            }
            /* here T_VAR must be a set of bounds
             * we fix the set of bounds to the default bound */
            ExprType *defaultType = T_VAR_DISJUNCT(coercion,0);
            updateInHashTable(tvarEnv, getTVarName(T_VAR_ID(coercion), buf), defaultType);
            coercion = defaultType;
        }
        if(typeEqSyntatic(coercion, res->exprType)) {
            return res;
        } else {
            if(TYPE(res)==T_UNSPECED) {
                generateErrMsg("error: dynamic coercion from an uninitialized value", NODE_EXPR_POS(node), node->base, buf);
                addRErrorMsg(errmsg, RE_DYNAMIC_COERCION_ERROR, buf);
                return newErrorRes(r, RE_DYNAMIC_COERCION_ERROR);
            }
            switch(getNodeType(coercion)) {
                case T_DYNAMIC:
                    return res;
                case T_INT:
                    switch(TYPE(res) ) {
                        case T_DOUBLE:
                        case T_BOOL:
                            if((int)RES_DOUBLE_VAL(res)!=RES_DOUBLE_VAL(res)) {
                                generateErrMsg("error: dynamic type conversion DOUBLE -> INTEGER: the double is not an integer", NODE_EXPR_POS(node), node->base, buf);
                                addRErrorMsg(errmsg, RE_DYNAMIC_COERCION_ERROR, buf);
                                return newErrorRes(r, RE_DYNAMIC_COERCION_ERROR);
                            } else {
                                return newIntRes(r, RES_INT_VAL(res));
                            }
                        case T_STRING:
                            return newIntRes(r, atoi(res->text));
                        default:
                            break;
                    }
                    break;
                case T_DOUBLE:
                    switch(TYPE(res) ) {
                        case T_INT:
                        case T_BOOL:
                            return newDoubleRes(r, RES_DOUBLE_VAL(res));
                        case T_STRING:
                            return newDoubleRes(r, atof(res->text));
                        default:
                            break;
                    }
                    break;
                case T_STRING:
                    switch(TYPE(res) ) {
                        case T_INT:
                        case T_DOUBLE:
                        case T_BOOL:
                            buf2 = convertResToString(res);

                            res = newStringRes(r, buf2);
                            free(buf2);
                            return res;
                        default:
                            break;
                    }
                    break;
                case T_BOOL:
                    switch(TYPE(res) ) {
                        case T_INT:
                        case T_DOUBLE:
                            return newBoolRes(r, RES_BOOL_VAL(res));
                        case T_STRING:
                            if(strcmp(res->text, "true")==0) {
                                return newBoolRes(r, 1);
                            } else if(strcmp(res->text, "false")==0) {
                                return newBoolRes(r, 0);
                            } else {
                                generateErrMsg("error: dynamic type conversion  string -> bool: the string is not in {true, false}", NODE_EXPR_POS(node), node->base, buf);
                                addRErrorMsg(errmsg, RE_DYNAMIC_COERCION_ERROR, buf);
                                return newErrorRes(r, RE_DYNAMIC_COERCION_ERROR);
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case T_CONS:
                    /* we can ignore the not top level type constructor and leave type checking to when it is derefed */
                    switch(TYPE(res)) {
                        case T_CONS:
                            return res;
                        case T_IRODS:
                            if(strcmp(res->exprType->text, IntArray_MS_T) == 0 ||
                               strcmp(res->exprType->text, StrArray_MS_T) == 0 ||
                               strcmp(res->exprType->text, GenQueryOut_MS_T) == 0) {
                                return res;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case T_DATETIME:
                    switch(TYPE(res)) {
                        case T_INT:
                            newDatetimeRes(r, (time_t) RES_INT_VAL(res));
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            char typeBuf1[128], typeBuf2[128];
            snprintf(buf, ERR_MSG_LEN, "error: coerce from type %s to type %s",
            		typeToString(res->exprType, tvarEnv, typeBuf1, 128),
            		typeToString(coercion, tvarEnv, typeBuf2, 128));
            generateErrMsg(buf, NODE_EXPR_POS(node), node->base, buf3);
            addRErrorMsg(errmsg, RE_TYPE_ERROR, buf3);
            return newErrorRes(r, RE_TYPE_ERROR);
        }
}

Res* evaluateActions(Node *expr, Node *reco, int applyAll, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r) {
/*
    printTree(expr, 0);
*/
    int i;
    int cutFlag = 0;
    Res* res = NULL;
    #ifndef DEBUG
    char tmpStr[1024];
    #endif
	switch(getNodeType(expr)) {
		case N_ACTIONS:
            for(i=0;i<expr->degree;i++) {
                Node *nodei = expr->subtrees[i];
                if(getNodeType(nodei) == N_APPLICATION && getNodeType(nodei->subtrees[0]) == TK_TEXT && strcmp(nodei->subtrees[0]->text, "cut") == 0) {
                    cutFlag = 1;
                    continue;
                }
                res = evaluateExpression3(nodei, applyAll, 0, rei, reiSaveFlag, env, errmsg,r);
                if(getNodeType(res) == N_ERROR) {
                    #ifndef DEBUG
                        sprintf(tmpStr,"executeRuleAction Failed for %s",N_APP_FUNC(nodei)->text); // JMC - backport 4826
                        rodsLogError(LOG_ERROR,RES_ERR_CODE(res),tmpStr);
                        rodsLog (LOG_NOTICE,"executeRuleBody: Micro-service or Action %s Failed with status %i",N_APP_FUNC(nodei)->text,RES_ERR_CODE(res)); // JMC - backport 4826
                    #endif
                    /* run recovery chain */
                    if(RES_ERR_CODE(res) != RETRY_WITHOUT_RECOVERY_ERR && reco!=NULL) {
                        int i2;
                        for(i2 = reco->degree-1<i?reco->degree-1:i;i2>=0;i2--) {
                            #ifndef DEBUG
                                if (reTestFlag > 0) {
                                        if (reTestFlag == COMMAND_TEST_1 || COMMAND_TEST_MSI)
                                                fprintf(stdout,"***RollingBack\n");
                                        else if (reTestFlag == HTML_TEST_1)
                                                fprintf(stdout,"<FONT COLOR=#FF0000>***RollingBack</FONT><BR>\n");
                                        else  if (reTestFlag == LOG_TEST_1)
                                                if (rei != NULL && rei->rsComm != NULL && &(rei->rsComm->rError) != NULL)
                                                        rodsLog (LOG_NOTICE,"***RollingBack\n");
                                }
                            #endif

                            Res *res2 = evaluateExpression3(reco->subtrees[i2], 0, 0, rei, reiSaveFlag, env, errmsg, r);
                            if(getNodeType(res2) == N_ERROR) {
                            #ifndef DEBUG
                                sprintf(tmpStr,"executeRuleRecovery Failed for %s",N_APP_FUNC(reco->subtrees[i2])->text); // JMC - backport 4826
                                rodsLogError(LOG_ERROR,RES_ERR_CODE(res2),tmpStr);
                            #endif
                            }
                        }
                    }
                    if(cutFlag) {
                        return newErrorRes(r, CUT_ACTION_PROCESSED_ERR);
                    } else {
                        return res;
                    }
                }
                else if(TYPE(res) == T_BREAK) {
                    return res;
                }
                else if(TYPE(res) == T_SUCCESS) {
                    return res;
                }

            }
            return res==NULL?newIntRes(r, 0):res;
        default:
            break;
	}
    char errbuf[ERR_MSG_LEN];
    generateErrMsg("error: unsupported ast node type.", NODE_EXPR_POS(expr), expr->base, errbuf);
	addRErrorMsg(errmsg, RE_UNSUPPORTED_AST_NODE_TYPE, errbuf);
	return newErrorRes(r, RE_UNSUPPORTED_AST_NODE_TYPE);
}

Res *evaluateFunctionApplication(Node *func, Node *arg, int applyAll, Node *node, ruleExecInfo_t* rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *res;
    char errbuf[ERR_MSG_LEN];
    switch(getNodeType(func)) {
        case N_SYM_LINK:
        case N_PARTIAL_APPLICATION:
            res = newPartialApplication(func, arg, RES_FUNC_N_ARGS(func) - 1, r);
            if(RES_FUNC_N_ARGS(res) == 0) {
                res = evaluateFunction3(res, applyAll, node, env, rei, reiSaveFlag, errmsg, r);
            }
            return res;
        default:
            generateErrMsg("unsupported function node type.", NODE_EXPR_POS(node), node->base, errbuf);
            addRErrorMsg(errmsg, RE_UNSUPPORTED_AST_NODE_TYPE, errbuf);
            return newErrorRes(r, RE_UNSUPPORTED_AST_NODE_TYPE);
    }
}

void printEnvIndent(Env *env) {
	Env *e = env->lower;
	int i =0;
		while(e!=NULL) {
			i++;
			e=e->lower;
		}
		printIndent(i);
}
/**
 * evaluate function
 * provide env and region isolation for rules and external microservices
 * precond n <= MAX_PARAMS_LEN
 */
Res* evaluateFunction3(Node *appRes, int applyAll, Node *node, Env *env, ruleExecInfo_t* rei, int reiSaveFlag, rError_t *errmsg, Region *r) {
    unsigned int i;
	unsigned int n;
	Node* subtrees0[MAX_FUNC_PARAMS];
	Node* args[MAX_FUNC_PARAMS];
	i = 0;
	Node *appFuncRes = appRes;
    while(getNodeType(appFuncRes) == N_PARTIAL_APPLICATION) {
        i++;
        subtrees0[MAX_FUNC_PARAMS - i] = appFuncRes->subtrees[1];
        appFuncRes = appFuncRes->subtrees[0];
    }
    /* app can only be N_FUNC_SYM_LINK */
    char* fn = appFuncRes->text;
	if(strcmp(fn, "nop") == 0) {
    	return newIntRes(r, 0);
    }
/*    printEnvIndent(env);
	printf("calling function %s\n", fn);
	char buf0[ERR_MSG_LEN];
	generateErrMsg("", NODE_EXPR_POS(node), node->base, buf0);
	printf("%s", buf0); */
    Res *appArgRes = appRes->subtrees[1];

    n = appArgRes->degree;
    Res** appArgs = appArgRes->subtrees;
    Node** nodeArgs = node->subtrees[1]->subtrees;
    ExprType *coercionType = NULL;
    #ifdef DEBUG
    char buf[ERR_MSG_LEN>1024?ERR_MSG_LEN:1024];
	sprintf(buf, "Action: %s\n", fn);
    writeToTmp("eval.log", buf);
    #endif
    /* char buf2[ERR_MSG_LEN]; */

    Res* res;
    Region *newRegion = make_region(0, NULL);
    Env *global = globalEnv(env);
    Env *nEnv = newEnv(newHashTable2(10, newRegion), global, env, newRegion);

    List *localTypingConstraints = NULL;
    FunctionDesc *fd = NULL;
    /* look up function descriptor */
    fd = (FunctionDesc *)lookupFromEnv(ruleEngineConfig.extFuncDescIndex, fn);
        /* find matching arity */
/*    if(fd!=NULL) {
        if((fd->exprType->vararg == ONCE && T_FUNC_ARITY(fd->exprType) == n) ||
            (fd->exprType->vararg == STAR && T_FUNC_ARITY(fd->exprType) - 2 <= n) ||
            (fd->exprType->vararg == PLUS && T_FUNC_ARITY(fd->exprType) - 1 <= n)) {
        } else {
            snprintf(buf, ERR_MSG_LEN, "error: arity mismatch for function %s", fn);
            generateErrMsg(buf, NODE_EXPR_POS(node), node->base, buf2);
            addRErrorMsg(errmsg, -1, buf2);
            res = newErrorRes(r, -1);
            RETURN;
        }
    }*/

    localTypingConstraints = newList(r);
    char ioParam[MAX_FUNC_PARAMS];
    /* evaluation parameters and try to resolve remaining tvars with unification */
    for(i=0;i<n;i++) {
        switch(getIOType(nodeArgs[i])) {
            case IO_TYPE_INPUT | IO_TYPE_OUTPUT: /* input/output */
                ioParam[i] = 'p';
                args[i] = evaluateExpression3(appArgs[i], applyAll > 1 ? applyAll : 0, 0, rei, reiSaveFlag, env, errmsg, newRegion);
                if(getNodeType(args[i])==N_ERROR) {
                    res = (Res *)args[i];
                    RETURN;
                }
                break;
            case IO_TYPE_INPUT: /* input */
                ioParam[i] = 'i';
                args[i] = appArgs[i];
                break;
            case IO_TYPE_DYNAMIC: /* dynamic */
            	if(isVariableNode(appArgs[i])) {
					args[i] = attemptToEvaluateVar3(appArgs[i]->text, appArgs[i], rei, reiSaveFlag, env, errmsg, newRegion);
					if(TYPE(args[i]) == T_UNSPECED) {
						ioParam[i] = 'o';
					} else {
						ioParam[i] = 'p';
						if(getNodeType(args[i])==N_ERROR) {
							res = (Res *)args[i];
							RETURN;
						}
					}
            	} else {
            		ioParam[i] = 'i';
            		args[i] = evaluateExpression3(appArgs[i], applyAll > 1 ? applyAll : 0, 1, rei, reiSaveFlag, env, errmsg, newRegion);
					if(getNodeType(args[i])==N_ERROR) {
						res = (Res *)args[i];
						RETURN;
					}
            	}
                break;
            case IO_TYPE_OUTPUT: /* output */
                ioParam[i] = 'o';
                args[i] = newUnspecifiedRes(r);
                break;
            case IO_TYPE_EXPRESSION: /* expression */
                ioParam[i] = 'e';
                args[i] = appArgs[i];
                break;
            case IO_TYPE_ACTIONS: /* actions */
                ioParam[i] = 'a';
                args[i] = appArgs[i];
                break;
        }
    }
    /* try to type all input parameters */
    coercionType = node->subtrees[1]->coercionType;
    if(coercionType!=NULL) {
    	/*for(i=0;i<n;i++) {
			 if((ioParam[i] == 'i' || ioParam[i] == 'p') && nodeArgs[i]->exprType != NULL) {
				if(unifyWith(args[i]->exprType, nodeArgs[i]->exprType, env->current, r) == NULL) {
					snprintf(buf, ERR_MSG_LEN, "error: dynamically typed parameter does not have expected type.");
					                    generateErrMsg(buf, nodeArgs[i]->expr, nodeArgs[i]->base, buf2);
					                    addRErrorMsg(errmsg, TYPE_ERROR, buf2);
					                    res = newErrorRes(r, TYPE_ERROR);
					                    RETURN;
				}
			}
		}*/


        ExprType *argType = newTupleRes(n, args, r)->exprType;
		if(typeFuncParam(node->subtrees[1], argType, coercionType, env->current, localTypingConstraints, errmsg, newRegion)!=0) {
			res = newErrorRes(r, RE_TYPE_ERROR);
			RETURN;
		}
		/* solve local typing constraints */
		/* typingConstraintsToString(localTypingConstraints, localTVarEnv, buf, 1024);
		printf("%s\n", buf); */
		Node *errnode;
		if(!solveConstraints(localTypingConstraints, env->current, errmsg, &errnode, r)) {
			res = newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);
			RETURN;
		}
		/*printVarTypeEnvToStdOut(localTVarEnv); */
		/* do the input value conversion */
		ExprType **coercionTypes = coercionType->subtrees;
		for(i=0;i<n;i++) {
			if((ioParam[i] == 'i' || ioParam[i] == 'p') && (nodeArgs[i]->option & OPTION_COERCE) != 0) {
				args[i] = processCoercion(nodeArgs[i], args[i], coercionTypes[i], env->current, errmsg, newRegion);
				if(getNodeType(args[i])==N_ERROR) {
					res = (Res *)args[i];
					RETURN;
				}
			}
		}
    }


	if (GlobalREAuditFlag > 0) {
		char tmpActStr[MAX_ACTION_SIZE];
		functionApplicationToString(tmpActStr,MAX_ACTION_SIZE, fn, args, n);
		reDebug("  ExecAction", -4, "", tmpActStr, node, env,rei);
    }

    if(fd!=NULL) {
        switch(getNodeType(fd)) {
            case N_FD_DECONSTRUCTOR:
                res = deconstruct(fn, args, n, FD_PROJ(fd), errmsg, r);
                break;
            case N_FD_CONSTRUCTOR:
                res = construct(fn, args, n, instantiate(node->exprType, env->current, 1, r), r);
                break;
            case N_FD_FUNCTION:
                res = (Res *) FD_SMSI_FUNC_PTR(fd)(args, n, node, rei, reiSaveFlag,  env, errmsg, newRegion);
                break;
            case N_FD_EXTERNAL:
                res = execMicroService3(fn, args, n, node, nEnv, rei, errmsg, newRegion);
                break;
            case N_FD_RULE_INDEX_LIST:
                res = execAction3(fn, args, n, applyAll, node, nEnv, rei, reiSaveFlag, errmsg, newRegion);
                break;
            default:
            	res = newErrorRes(r, RE_UNSUPPORTED_AST_NODE_TYPE);
                generateAndAddErrMsg("unsupported function descriptor type", node, RE_UNSUPPORTED_AST_NODE_TYPE, errmsg);
                RETURN;
        }
    } else {
        res = execMicroService3(fn, args, n, node, nEnv, rei, errmsg, newRegion);
    }

	if (GlobalREAuditFlag > 0) {
		char tmpActStr[MAX_ACTION_SIZE];
		functionApplicationToString(tmpActStr,MAX_ACTION_SIZE, fn, args, n);
		reDebug("  ExecAction", -4, "Done", tmpActStr, node, env,rei);
    }

    if(getNodeType(res)==N_ERROR) {
        RETURN;
    }

    for(i=0;i<n;i++) {
        Res *resp = NULL;

        if(ioParam[i] == 'o' || ioParam[i] == 'p') {
            if((appArgs[i]->option & OPTION_COERCE) != 0) {
                args[i] = processCoercion(nodeArgs[i], args[i], appArgs[i]->exprType, env->current, errmsg, newRegion);
            }
            if(getNodeType(args[i])==N_ERROR) {
                res = (Res *)args[i];
                RETURN ;
            }
            resp = setVariableValue(appArgs[i]->text, args[i],rei,env,errmsg,r);
            /*char *buf = convertResToString(args[i]);
            printEnvIndent(env);
            printf("setting variable %s to %s\n", appArgs[i]->text, buf);
            free(buf);*/
        }
        if(resp!=NULL && getNodeType(resp)==N_ERROR) {
            res = resp;
            RETURN;
        }
    }
    /*printEnvIndent(env);
	printf("exiting function %s\n", fn);
*/
    /*return res;*/
ret:
    /*deleteEnv(nEnv, 2);*/
    cpEnv(env,r);
    res = cpRes(res,r);
    region_free(newRegion);
    return res;

}

Res* attemptToEvaluateVar3(char* vn, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	if(vn[0]=='*') { /* local variable */
		/* try to get local var from env */
		Res* res0 = (Res *)lookupFromEnv(env, vn);
		if(res0==NULL) {
			return newUnspecifiedRes(r);
		} else {
			return res0;
		}
	} else if(vn[0]=='$') { /* session variable */
        Res *res = getSessionVar("",vn,rei, env, errmsg,r);
		if(res==NULL) {
			return newUnspecifiedRes(r);
		} else {
			return res;
		}
	} else {
        return NULL;
	}
}

Res* evaluateVar3(char* vn, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    char buf[ERR_MSG_LEN>1024?ERR_MSG_LEN:1024];
    char buf2[ERR_MSG_LEN];
    Res *res = attemptToEvaluateVar3(vn, node, rei, reiSaveFlag, env, errmsg, r);
    if(res == NULL || TYPE(res) == T_UNSPECED) {
        if(vn[0]=='*') { /* local variable */
		    snprintf(buf, ERR_MSG_LEN, "error: unable to read local variable %s.", vn);
                    generateErrMsg(buf, NODE_EXPR_POS(node), node->base, buf2);
                    addRErrorMsg(errmsg, RE_UNABLE_TO_READ_LOCAL_VAR, buf2);
                    /*printEnvToStdOut(env); */
                    return newErrorRes(r, RE_UNABLE_TO_READ_LOCAL_VAR);
        } else if(vn[0]=='$') { /* session variable */
    	    snprintf(buf, ERR_MSG_LEN, "error: unable to read session variable %s.", vn);
            generateErrMsg(buf, NODE_EXPR_POS(node), node->base, buf2);
            addRErrorMsg(errmsg, RE_UNABLE_TO_READ_SESSION_VAR, buf2);
            return newErrorRes(r, RE_UNABLE_TO_READ_SESSION_VAR);
        } else {
            snprintf(buf, ERR_MSG_LEN, "error: unable to read variable %s.", vn);
            generateErrMsg(buf, NODE_EXPR_POS(node), node->base, buf2);
            addRErrorMsg(errmsg, RE_UNABLE_TO_READ_VAR, buf2);
            return newErrorRes(r, RE_UNABLE_TO_READ_VAR);
        }
	}
	return res;
}

/**
 * return NULL error
 *        otherwise success
 */
Res* getSessionVar(char *action,  char *varName,  ruleExecInfo_t *rei, Env *env, rError_t *errmsg, Region *r) {
  char *varMap;
  void *varValue = NULL;
  int i, vinx;
  varValue = NULL;

  /* Maps varName to the standard name and make varMap point to it. */
  /* It seems that for each pair of varName and standard name there is a list of actions that are supported. */
  /* vinx stores the index of the current pair so that we can start for the next pair if the current pair fails. */
  vinx = getVarMap(action,varName, &varMap, 0); /* reVariableMap.c */
  while (vinx >= 0) {
	/* Get the value of session variable referenced by varMap. */
      i = getVarValue(varMap, rei, (char **)&varValue); /* reVariableMap.c */
      /* convert to char * because getVarValue requires char * */
      if (i >= 0) {
            if (varValue != NULL) {
                Res *res = NULL;
                FunctionDesc *fd = (FunctionDesc *) lookupFromEnv(ruleEngineConfig.extFuncDescIndex, varMap);
                if (fd == NULL) {
                    /* default to string */
                    res = newStringRes(r, (char *) varValue);
                    free(varValue);
                } else {
                    ExprType *type = T_FUNC_RET_TYPE(fd->exprType); /* get var type from varMap */
                    switch (getNodeType(type)) {
                        case T_STRING:
                            res = newStringRes(r, (char *) varValue);
                            free(varValue);
                            break;
                        case T_INT:
                            res = newIntRes(r, atoi((char *)varValue));
                            free(varValue);
                            break;
                        case T_DOUBLE:
                            res = newDoubleRes(r, atof((char *)varValue));
                            free(varValue);
                            break;
                        case T_IRODS:
                            res = newUninterpretedRes(r, type->text, varValue, NULL);
                            break;
                        default:
                            /* unsupported type error */
                            res = newErrorRes(r, RE_UNSUPPORTED_SESSION_VAR_TYPE);
                            addRErrorMsg(errmsg, RE_UNSUPPORTED_SESSION_VAR_TYPE, "error: unsupported session variable type");
                            break;
                    }
                }
                free(varMap);
                return res;
            } else {
                return NULL;
            }
    } else if (i == NULL_VALUE_ERR) { /* Try next varMap, starting from vinx+1. */
      free(varMap);
      vinx = getVarMap(action,varName, &varMap, vinx+1);
    } else { /* On error, return 0. */
      free(varMap);
      if (varValue != NULL) free (varValue);
      return NULL;
    }
  }
  /* varMap not found, return 0. */
  if (varValue != NULL) free (varValue);
  return NULL;
}

/*
 * execute an external microserive or a rule
 */
Res* execAction3(char *actionName, Res** args, unsigned int nargs, int applyAllRule, Node *node, Env *env, ruleExecInfo_t* rei, int reiSaveFlag, rError_t *errmsg, Region *r)
{
	char buf[ERR_MSG_LEN>1024?ERR_MSG_LEN:1024];
	char buf2[ERR_MSG_LEN];
	char action[MAX_NAME_LEN];
	strcpy(action, actionName);
	mapExternalFuncToInternalProc2(action);

#ifdef USE_EIRODS
//rodsLog( LOG_NOTICE, "calling pluggable MSVC from arithmetics.c:execAction3" );
	// =-=-=-=-=-=-=-
	// switch to using pluggable usvc
	eirods::ms_table_entry ms_entry;
    int actionInx = actionTableLookUp( ms_entry, action );
#else
	int actionInx = actionTableLookUp(action);
#endif
	if (actionInx < 0) { /* rule */
		/* no action (microservice) found, try to lookup a rule */
		Res *actionRet = execRule(actionName, args, nargs, applyAllRule, env, rei, reiSaveFlag, errmsg, r);
		if (getNodeType(actionRet) == N_ERROR && (
                        RES_ERR_CODE(actionRet) == NO_RULE_FOUND_ERR)) {
			snprintf(buf, ERR_MSG_LEN, "error: cannot find rule for action \"%s\" available: %d.", action, availableRules());
                        generateErrMsg(buf, NODE_EXPR_POS(node), node->base, buf2);
                        addRErrorMsg(errmsg, NO_RULE_OR_MSI_FUNCTION_FOUND_ERR, buf2);
                        /*dumpHashtableKeys(coreRules); */
			return newErrorRes(r, NO_RULE_OR_MSI_FUNCTION_FOUND_ERR);
		}
		else {
			return actionRet;
		}
	} else { /* microservice */
		return execMicroService3(action, args, nargs, node, env, rei, errmsg, r);
	}
}

/**
 * execute micro service msiName
 */
Res* execMicroService3 (char *msName, Res **args, unsigned int nargs, Node *node, Env *env, ruleExecInfo_t *rei, rError_t *errmsg, Region *r) {
	msParamArray_t *origMsParamArray = rei->msParamArray;
	funcPtr myFunc = NULL;
	int actionInx;
	unsigned int numOfStrArgs;
	unsigned int i;
	int ii;
	msParam_t *myArgv[MAX_PARAMS_LEN];
    Res *res;
#ifdef USE_EIRODS
//rodsLog( LOG_NOTICE, "calling pluggable MSVC from arithmetics.c:execMicroService3" );
	// =-=-=-=-=-=-=-
	// switch to using pluggable usvc
	eirods::ms_table_entry ms_entry;
    actionInx = actionTableLookUp( ms_entry, msName );
#else
	/* look up the micro service */
	actionInx = actionTableLookUp(msName);
#endif
	char errbuf[ERR_MSG_LEN];
	if (actionInx < 0) {
            int ret = NO_MICROSERVICE_FOUND_ERR;
            generateErrMsg("execMicroService3: no micro service found", NODE_EXPR_POS(node), node->base, errbuf);
            addRErrorMsg(errmsg, ret, errbuf);
            return newErrorRes(r, ret);

	}

#ifdef USE_EIRODS
	myFunc       = ms_entry.callAction_;
	numOfStrArgs = ms_entry.numberOfStringArgs_;
#else
	myFunc =  MicrosTable[actionInx].callAction;
	numOfStrArgs = MicrosTable[actionInx].numberOfStringArgs;
#endif

	if (nargs != numOfStrArgs) {
            int ret = ACTION_ARG_COUNT_MISMATCH;
            generateErrMsg("execMicroService3: wrong number of arguments", NODE_EXPR_POS(node), node->base, errbuf);
            addRErrorMsg(errmsg, ret, errbuf);
            return newErrorRes(r, ret);
        }

	/* convert arguments from Res to msParam_t */
    /* char buf[1024]; */
    int fillInParamLabel = node->degree == 2 && node->subtrees[1]->degree == (int) numOfStrArgs;
	for (i = 0; i < numOfStrArgs; i++) {
            myArgv[i] = (msParam_t *)malloc(sizeof(msParam_t));
            Res *res = args[i];
            if(res != NULL) {
                int ret =
                    convertResToMsParam(myArgv[i], res, errmsg);
				myArgv[i]->label = fillInParamLabel && isVariableNode(node->subtrees[1]->subtrees[i]) ? strdup(node->subtrees[1]->subtrees[i]->text) : NULL;
                if(ret!=0) {
                    generateErrMsg("execMicroService3: error converting arguments to MsParam", NODE_EXPR_POS(node->subtrees[i]), node->subtrees[i]->base, errbuf);
                    addRErrorMsg(errmsg, ret, errbuf);
                    int j = i;
                    for(;j>=0;j--) {
                        if(TYPE(args[j])!=T_IRODS) {
                            free(myArgv[j]->inOutStruct);
                            myArgv[j]->inOutStruct = NULL;
                        }
                        free(myArgv[j]->label);
                        free(myArgv[j]->type);
                        free(myArgv[j]);
                    }
                    return newErrorRes(r, ret);
                }
            } else {
                myArgv[i]->inOutStruct = NULL;
                myArgv[i]->inpOutBuf = NULL;
                myArgv[i]->type = strdup(STR_MS_T);
            }
            /* sprintf(buf,"**%d",i); */
            /* myArgv[i]->label = strdup(buf); */
	}
        /* convert env to msparam array */
        rei->msParamArray = newMsParamArray();
        int ret = convertEnvToMsParamArray(rei->msParamArray, env, errmsg, r);
        if(ret!=0) {
            generateErrMsg("execMicroService3: error converting env to MsParamArray", NODE_EXPR_POS(node), node->base, errbuf);
            addRErrorMsg(errmsg, ret, errbuf);
            res = newErrorRes(r,ret);
            RETURN;
        }

	if (GlobalREAuditFlag > 0) {
		char tmpActStr[MAX_ACTION_SIZE];
		functionApplicationToString(tmpActStr,MAX_ACTION_SIZE, msName, args, nargs);
		reDebug("    ExecMicroSrvc", -4, "", tmpActStr, node, env,rei);
	}

	if (numOfStrArgs == 0)
		ii = (*(int (*)(ruleExecInfo_t *))myFunc) (rei) ;
	else if (numOfStrArgs == 1)
		ii = (*(int (*)(msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],rei);
	else if (numOfStrArgs == 2)
		ii = (*(int (*)(msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],rei);
	else if (numOfStrArgs == 3)
		ii = (*(int (*)(msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],myArgv[2],rei);
	else if (numOfStrArgs == 4)
		ii = (*(int (*)(msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],rei);
	else if (numOfStrArgs == 5)
		ii = (*(int (*)(msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],rei);
	else if (numOfStrArgs == 6)
		ii = (*(int (*)(msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],rei);
	else if (numOfStrArgs == 7)
		ii = (*(int (*)(msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],rei);
	else if (numOfStrArgs == 8)
		ii = (*(int (*)(msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],rei);
	else if (numOfStrArgs == 9)
		ii = (*(int (*)(msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],myArgv[8],rei);
	else if (numOfStrArgs == 10)
		ii = (*(int (*)(msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t *))myFunc) (myArgv[0],myArgv[1],myArgv[2],myArgv[3],myArgv[4],myArgv[5],myArgv[6],myArgv[7],
		                myArgv[8],myArgv [9],rei);
    if(ii<0) {
        res = newErrorRes(r, ii);
        RETURN;
    }
    /* converts back env */
	ret = updateMsParamArrayToEnvAndFreeNonIRODSType(rei->msParamArray, env, errmsg, r);
    if(ret!=0) {
        generateErrMsg("execMicroService3: error env from MsParamArray", NODE_EXPR_POS(node), node->base, errbuf);
        addRErrorMsg(errmsg, ret, errbuf);
        res = newErrorRes(r, ret);
        RETURN;
    }
    /* params */
	for (i = 0; i < numOfStrArgs; i++) {
        if(myArgv[i] != NULL) {
            int ret =
                convertMsParamToResAndFreeNonIRODSType(myArgv[i], args[i], errmsg, r);
            if(ret!=0) {
                generateErrMsg("execMicroService3: error converting arguments from MsParam", NODE_EXPR_POS(node), node->base, errbuf);
                addRErrorMsg(errmsg, ret, errbuf);
                res= newErrorRes(r, ret);
                RETURN;
            }
        } else {
            args[i] = NULL;
        }
	}

/*
        char vn[100];
	for(i=0;i<numOfStrArgs;i++) {
            sprintf(vn,"**%d",i);
            largs[i] = lookupFromHashTable(env, vn);
	}
*/
	res = newIntRes(r, ii);
ret:
    if(rei->msParamArray!=NULL) {
        deleteMsParamArray(rei->msParamArray);
    }
    rei->msParamArray = origMsParamArray;
    for(i=0;i<numOfStrArgs;i++) {
        if(TYPE(args[i])!=T_IRODS && myArgv[i]->inOutStruct!=NULL) {
            free(myArgv[i]->inOutStruct);
        }
        free(myArgv[i]->label);
        free(myArgv[i]->type);
        free(myArgv[i]);
    }
    if(getNodeType(res)==N_ERROR) {
        generateErrMsg("execMicroService3: error when executing microservice", NODE_EXPR_POS(node), node->base, errbuf);
        addRErrorMsg(errmsg, RES_ERR_CODE(res), errbuf);
    }
    return res;
}

Res* execRuleFromCondIndex(char *ruleName, Res **args, int argc, CondIndexVal *civ, int applyAll, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r) {
            /*printTree(civ->condExp, 0); */
        Res *status;
        Env *envNew = newEnv(newHashTable2(10, r), globalEnv(env), env, r);
        RuleDesc *rd;
        Node* rule = NULL;
        RuleIndexListNode *indexNode = NULL;
        Res* res = NULL;


        if(civ->params->degree != argc) {
            char buf[ERR_MSG_LEN];
            snprintf(buf, ERR_MSG_LEN, "error: cannot apply rule %s from rule conditional index because of wrong number of arguments, declared %d, supplied %d.", ruleName, civ->params->degree, argc);
            addRErrorMsg(errmsg, ACTION_ARG_COUNT_MISMATCH, buf);
            return newErrorRes(r, ACTION_ARG_COUNT_MISMATCH);
        }

        int i = initializeEnv(civ->params,args,argc, envNew->current,r);
        if (i != 0) {
            status = newErrorRes(r, i);
            RETURN;
        }

        res = evaluateExpression3(civ->condExp, 0, 0, rei, reiSaveFlag, envNew,  errmsg, r);
        if(getNodeType(res) == N_ERROR) {
            status = res;
            RETURN;
        }
        if(TYPE(res) != T_STRING) {
            /* todo try coercion */
            addRErrorMsg(errmsg, RE_DYNAMIC_TYPE_ERROR, "error: the lhs of indexed rule condition does not evaluate to a string");
            status = newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);
            RETURN;
        }

        indexNode = (RuleIndexListNode *)lookupFromHashTable(civ->valIndex, res->text);
        if(indexNode == NULL) {
#ifndef DEBUG
            rodsLog (LOG_NOTICE,"applyRule Failed for action 1: %s with status %i",ruleName, NO_MORE_RULES_ERR);
#endif
            status = newErrorRes(r, NO_MORE_RULES_ERR);
            RETURN;
        }

        rd = getRuleDesc(indexNode->ruleIndex); /* increase the index to move it to core rules index below MAX_NUM_APP_RULES are app rules */

        if(rd->ruleType != RK_REL && rd->ruleType != RK_FUNC) {
#ifndef DEBUG
            rodsLog (LOG_NOTICE,"applyRule Failed for action 1: %s with status %i",ruleName, NO_MORE_RULES_ERR);
#endif
            status = newErrorRes(r, NO_MORE_RULES_ERR);
            RETURN;
        }

        rule = rd->node;

        status = execRuleNodeRes(rule, args, argc,  applyAll > 1? applyAll : 0, env, rei, reiSaveFlag, errmsg, r);

        if (getNodeType(status) == N_ERROR) {
            rodsLog (LOG_NOTICE,"applyRule Failed for action : %s with status %i",ruleName, RES_ERR_CODE(status));
        }

    ret:
        /* deleteEnv(envNew, 2); */
        return status;


}
/*
 * look up rule node by rulename from index
 * apply rule condition index if possilbe
 * call execRuleNodeRes until success or no more rules
 */
Res *execRule(char *ruleNameInp, Res** args, unsigned int argc, int applyAllRuleInp, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r)
{
    int ruleInx = 0, statusI;
    Res *statusRes = NULL;
    int  inited = 0;
    ruleExecInfo_t  *saveRei;
    int reTryWithoutRecovery = 0;
    char ruleName[MAX_NAME_LEN];
    int applyAllRule = applyAllRuleInp;

    #ifdef DEBUG
    char buf[1024];
    snprintf(buf, 1024, "execRule: %s\n", ruleNameInp);
    writeToTmp("entry.log", buf);
    #endif

    ruleInx = 0; /* new rule */
    strcpy(ruleName, ruleNameInp);
    mapExternalFuncToInternalProc2(ruleName);

    RuleIndexListNode *ruleIndexListNode;
    int success = 0;
    int first = 1;
    while (1) {
        statusI = findNextRule2(ruleName, ruleInx, &ruleIndexListNode);

        if (statusI != 0) {
			if(applyAllRule == 0) {
	#ifndef DEBUG
				rodsLog (LOG_NOTICE,"applyRule Failed for action 1: %s with status %i",ruleName, statusI);
	#endif
				statusRes = statusRes == NULL ? newErrorRes(r, NO_RULE_FOUND_ERR) : statusRes;
			} else { // apply all rules succeeds even when 0 rule is applied
				success = 1;
			}
            break;
        }
		if (reiSaveFlag == SAVE_REI) {
			int statusCopy = 0;
			if (inited == 0) {
				saveRei = (ruleExecInfo_t *) mallocAndZero(sizeof (ruleExecInfo_t));
				statusCopy = copyRuleExecInfo(rei, saveRei);
				inited = 1;
			} else if (reTryWithoutRecovery == 0) {
				statusCopy = copyRuleExecInfo(saveRei, rei);
			}
			if(statusCopy != 0) {
				statusRes = newErrorRes(r, statusCopy);
				break;
			}
		}

		if(!first) {
			addRErrorMsg(errmsg, statusI, ERR_MSG_SEP);
		} else {
			first = 0;
		}

		if(ruleIndexListNode->secondaryIndex) {
        	statusRes = execRuleFromCondIndex(ruleName, args, argc, ruleIndexListNode->condIndex, applyAllRule, env, rei, reiSaveFlag, errmsg, r);
        } else {

        	RuleDesc *rd = getRuleDesc(ruleIndexListNode->ruleIndex);
			if(rd->ruleType == RK_REL || rd->ruleType == RK_FUNC) {

				Node* rule = rd->node;
				unsigned int inParamsCount = RULE_NODE_NUM_PARAMS(rule);
				if (inParamsCount != argc) {
					ruleInx++;
					continue;
				}

				if (GlobalREAuditFlag > 0)
					reDebug("  GotRule", ruleInx, "", ruleName, NULL, env, rei); /* pass in NULL for inMsParamArray for now */
		#ifndef DEBUG
				if (reTestFlag > 0) {
					if (reTestFlag == COMMAND_TEST_1)
						fprintf(stdout, "+Testing Rule Number:%i for Action:%s\n", ruleInx, ruleName);
					else if (reTestFlag == HTML_TEST_1)
						fprintf(stdout, "+Testing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n", ruleInx, ruleName);
					else if (rei != 0 && rei->rsComm != 0 && &(rei->rsComm->rError) != 0)
						rodsLog(LOG_NOTICE, "+Testing Rule Number:%i for Action:%s\n", ruleInx, ruleName);
				}
		#endif
				/* printTree(rule, 0); */

				statusRes = execRuleNodeRes(rule, args, argc,  applyAllRule > 1? applyAllRule : 0, env, rei, reiSaveFlag, errmsg, r);

			}
        }
		if(getNodeType(statusRes)!=N_ERROR) {
			success = 1;
			if (applyAllRule == 0) { /* apply first rule */
				break;
			} else { /* apply all rules */
				if (reiSaveFlag == SAVE_REI) {
					freeRuleExecInfoStruct(saveRei, 0);
					inited = 0;
				}
			}
		} else if(RES_ERR_CODE(statusRes)== RETRY_WITHOUT_RECOVERY_ERR) {
			reTryWithoutRecovery = 1;
		} else if(RES_ERR_CODE(statusRes)== CUT_ACTION_PROCESSED_ERR) {
			break;
		}

        ruleInx++;
    }

    if (inited == 1) {
        freeRuleExecInfoStruct(saveRei, 0);
    }

    if(success) {
        if(applyAllRule) {
            /* if we apply all rules, then it succeeds even if some of the rules fail */
            return newIntRes(r, 0);
        } else if(TYPE(statusRes) == T_SUCCESS) {
            return newIntRes(r, 0);
        } else {
            return statusRes;
        }
    } else {
#ifndef DEBUG
            rodsLog (LOG_NOTICE,"applyRule Failed for action 2: %s with status %i",ruleName, RES_ERR_CODE(statusRes));
#endif
        return statusRes;
    }
}
void copyFromEnv(Res **args, char **inParams, int inParamsCount, Hashtable *env, Region *r) {
	int i;
	for(i=0;i<inParamsCount;i++) {
		args[i]= cpRes((Res *)lookupFromHashTable(env, inParams[i]),r);
	}
}
/*
 * execute a rule given by an AST node
 * create a new environment and intialize it with parameters
 */
Res* execRuleNodeRes(Node *rule, Res** args, unsigned int argc, int applyAll, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r)
{
	if (GlobalREAuditFlag > 0)
		reDebug("  ExecRule", -4, "", RULE_NAME(rule), NULL, env, rei); /* pass in NULL for inMsParamArray for now */
	Node* ruleCondition = rule->subtrees[1];
	Node* ruleAction = rule->subtrees[2];
	Node* ruleRecovery = rule->subtrees[3];
	Node* ruleHead = rule->subtrees[0];
    Node** paramsNodes = ruleHead->subtrees[0]->subtrees;
	char* paramsNames[MAX_NUM_OF_ARGS_IN_ACTION];
        unsigned int inParamsCount = RULE_NODE_NUM_PARAMS(rule);
        Res *statusRes;

        if(inParamsCount != argc) {
            generateAndAddErrMsg("error: action argument count mismatch", rule, ACTION_ARG_COUNT_MISMATCH, errmsg);
            return newErrorRes(r, ACTION_ARG_COUNT_MISMATCH);

        }
        unsigned int k;
        for (k = 0; k < inParamsCount ; k++) {
            paramsNames[k] =  paramsNodes[k]->text;
        }

        Env *global = globalEnv(env);
        Region *rNew = make_region(0, NULL);
        Env *envNew = newEnv(newHashTable2(10, rNew), global, env, rNew);

	int statusInitEnv;
	/* printEnvToStdOut(envNew->current); */
        statusInitEnv = initializeEnv(ruleHead->subtrees[0],args,argc, envNew->current,rNew);
        if (statusInitEnv != 0)
            return newErrorRes(r, statusInitEnv);
        /* printEnvToStdOut(envNew->current); */
        Res *res = evaluateExpression3(ruleCondition, 0, 0, rei, reiSaveFlag,  envNew, errmsg, rNew);
        /* todo consolidate every error into T_ERROR except OOM */
        if (getNodeType(res) != N_ERROR && TYPE(res)==T_BOOL && RES_BOOL_VAL(res)!=0) {
#ifndef DEBUG
#if 0
            if (reTestFlag > 0) {
                    if (reTestFlag == COMMAND_TEST_1)
                            fprintf(stdout,"+Executing Rule Number:%i for Action:%s\n",ruleInx,ruleName);
                    else if (reTestFlag == HTML_TEST_1)
                            fprintf(stdout,"+Executing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n",ruleInx,ruleName);
                    else
                            rodsLog (LOG_NOTICE,"+Executing Rule Number:%i for Action:%s\n",ruleInx,ruleName);
            }
#endif
#endif
            if(getNodeType(ruleAction) == N_ACTIONS) {
                statusRes = evaluateActions(ruleAction, ruleRecovery, applyAll, rei, reiSaveFlag,  envNew, errmsg,rNew);
            } else {
                statusRes = evaluateExpression3(ruleAction, applyAll, 0, rei, reiSaveFlag, envNew, errmsg, rNew);
            }
            /* copy parameters */
            copyFromEnv(args, paramsNames, inParamsCount, envNew->current, r);
            /* copy return values */
            statusRes = cpRes(statusRes, r);

            if (getNodeType(statusRes) == N_ERROR) {
                    #ifndef DEBUG
                    rodsLog (LOG_NOTICE,"applyRule Failed for action 2: %s with status %i",ruleHead->text, RES_ERR_CODE(statusRes));
                    #endif
            }
        } else {
            if(getNodeType(res)!=N_ERROR && TYPE(res)!=T_BOOL) {
                char buf[ERR_MSG_LEN];
                generateErrMsg("error: the rule condition does not evaluate to a boolean value", NODE_EXPR_POS(ruleCondition), ruleCondition->base, buf);
                addRErrorMsg(errmsg, RE_TYPE_ERROR, buf);
            }
            statusRes = newErrorRes(r, RULE_FAILED_ERR);
        }
        /* copy global variables */
        cpEnv(global, r);
        /* deleteEnv(envNew, 2); */
        region_free(rNew);
    	if (GlobalREAuditFlag > 0)
    		reDebug("  ExecRule", -4, "Done", RULE_NAME(rule), NULL, env, rei); /* pass in NULL for inMsParamArray for now */
        return statusRes;

}

Res* matchPattern(Node *pattern, Node *val, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r) {
    char errbuf[ERR_MSG_LEN];
    char *localErrorMsg = NULL;
    Node *p = pattern, *v = val;
    Res *res;
    char *varName;
    char matcherName[MAX_NAME_LEN];
    switch (getNodeType(pattern)) {
    case N_APPLICATION:
        matcherName[0]='~';
        strcpy(matcherName+1, pattern->subtrees[0]->text);
        RuleIndexListNode *node;
        if(findNextRule2(matcherName, 0, &node) == 0) {
            v = execRule(matcherName, &val, 1, 0, env, rei, reiSaveFlag, errmsg, r);
            RE_ERROR2(getNodeType(v) == N_ERROR, "user defined pattern function error");
            if(getNodeType(v)!=N_TUPLE) {
            	Res **tupleComp = (Res **)region_alloc(r, sizeof(Res *));
            	*tupleComp = v;
            	v = newTupleRes(1, tupleComp ,r);
            }
        } else {
            RE_ERROR2(v->text == NULL || strcmp(pattern->subtrees[0]->text, v->text)!=0, "pattern mismatch constructor");
            Res **tupleComp = (Res **)region_alloc(r, sizeof(Res *) * v->degree);
			memcpy(tupleComp, v->subtrees, sizeof(Res *) * v->degree);
			v = newTupleRes(v->degree, tupleComp ,r);
        }
		res = matchPattern(p->subtrees[1], v, env, rei, reiSaveFlag, errmsg, r);
        return res;
    case TK_VAR:
        varName = pattern->text;
        if(varName[0] == '*') {
        	if(lookupFromEnv(env, varName)==NULL) {
        		/* new variable */
				if(insertIntoHashTable(env->current, varName, val) == 0) {
					snprintf(errbuf, ERR_MSG_LEN, "error: unable to write to local variable \"%s\".",varName);
					addRErrorMsg(errmsg, RE_UNABLE_TO_WRITE_LOCAL_VAR, errbuf);
					return newErrorRes(r, RE_UNABLE_TO_WRITE_LOCAL_VAR);
				}
			} else {
				updateInEnv(env, varName, val);
			}
        } else if(varName[0] == '$') {
        	return setVariableValue(varName, val, rei, env, errmsg, r);
        }
        return newIntRes(r, 0);

    case N_TUPLE:
    	RE_ERROR2(getNodeType(v) != N_TUPLE, "pattern mismatch value is not a tuple.");
    	RE_ERROR2(p->degree != v->degree, "pattern mismatch arity");
		int i;
		for(i=0;i<p->degree;i++) {
			Res *res = matchPattern(p->subtrees[i], v->subtrees[i], env, rei, reiSaveFlag, errmsg, r);
			if(getNodeType(res) == N_ERROR) {
				return res;
			}
		}
		return newIntRes(r, 0);
    case TK_STRING:
    	RE_ERROR2(getNodeType(v) != N_VAL || TYPE(v) != T_STRING, "pattern mismatch value is not a string.");
    	RE_ERROR2(strcmp(pattern->text, v->text)!=0 , "pattern mismatch string.");
		return newIntRes(r, 0);
    case TK_BOOL:
    	RE_ERROR2(getNodeType(v) != N_VAL || TYPE(v) != T_BOOL, "pattern mismatch value is not a boolean.");
    	res = evaluateExpression3(pattern, 0, 0, rei, reiSaveFlag, env, errmsg, r);
    	CASCADE_N_ERROR(res);
    	RE_ERROR2(RES_BOOL_VAL(res) != RES_BOOL_VAL(v) , "pattern mismatch boolean.");
		return newIntRes(r, 0);
    case TK_INT:
    	RE_ERROR2(getNodeType(v) != N_VAL || (TYPE(v) != T_INT && TYPE(v) != T_DOUBLE), "pattern mismatch value is not an integer.");
    	res = evaluateExpression3(pattern, 0, 0, rei, reiSaveFlag, env, errmsg, r);
    	CASCADE_N_ERROR(res);
    	RE_ERROR2(atoi(res->text) != (TYPE(v) == T_INT ? RES_INT_VAL(v) : RES_DOUBLE_VAL(v)) , "pattern mismatch integer.");
		return newIntRes(r, 0);
    case TK_DOUBLE:
    	RE_ERROR2(getNodeType(v) != N_VAL || (TYPE(v) != T_DOUBLE && TYPE(v) != T_INT), "pattern mismatch value is not a double.");
    	res = evaluateExpression3(pattern, 0, 0, rei, reiSaveFlag, env, errmsg, r);
    	CASCADE_N_ERROR(res);
    	RE_ERROR2(atof(res->text) != (TYPE(v) == T_DOUBLE ? RES_DOUBLE_VAL(v) : RES_INT_VAL(v)), "pattern mismatch integer.");
		return newIntRes(r, 0);
    default:
    	RE_ERROR2(1, "malformatted pattern error");
    	break;
    }
    error:
        generateErrMsg(localErrorMsg,NODE_EXPR_POS(pattern), pattern->base, errbuf);
        addRErrorMsg(errmsg, RE_PATTERN_NOT_MATCHED, errbuf);
        return newErrorRes(r, RE_PATTERN_NOT_MATCHED);

}

            /** this allows use of cut as the last msi in the body so that other rules will not be processed
                even when the current rule succeeds **/
/*            if (cutFlag == 1)
       		return(CUT_ACTION_ON_SUCCESS_PROCESSED_ERR);
*/

