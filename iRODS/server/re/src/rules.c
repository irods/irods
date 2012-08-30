/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "debug.h"
#include "rules.h"
#include "index.h"
#include "functions.h"
#include "arithmetics.h"
#include "configuration.h"
#include "filesystem.h"



#define RE_ERROR(cond) if(cond) { goto error; }

extern int GlobalAllRuleExecFlag;

#ifdef USE_EIRODS
	#include "reAction.h"
#else
	#ifndef DEBUG
		#include "reGlobalsExtern.h"
		#include "reHelpers1.h"
		typedef struct {
		  char action[MAX_ACTION_SIZE];
		  int numberOfStringArgs;
		  funcPtr callAction;
		} microsdef_t;
		extern int NumOfAction;
		extern microsdef_t MicrosTable[];
	#endif
#endif // ifdef USE_EIRODS

/**
 * Read a set of rules from files.
 * return 0 success
 *        otherwise error code
 */

int readRuleSetFromFile(char *ruleBaseName, RuleSet *ruleSet, Env *funcDesc, int* errloc, rError_t *errmsg, Region *r) {
	char rulesFileName[MAX_NAME_LEN];
	getRuleBasePath(ruleBaseName, rulesFileName);

	return readRuleSetFromLocalFile(ruleBaseName, rulesFileName, ruleSet, funcDesc, errloc, errmsg,r);
}
int readRuleSetFromLocalFile(char *ruleBaseName, char *rulesFileName, RuleSet *ruleSet, Env *funcDesc, int *errloc, rError_t *errmsg, Region *r) {

	FILE *file;
    char errbuf[ERR_MSG_LEN];
    file = fopen(rulesFileName, "r");
	if (file == NULL) {
            snprintf(errbuf, ERR_MSG_LEN,
                    "readRuleSetFromFile() could not open rules file %s\n",
                    rulesFileName);
            addRErrorMsg(errmsg, RULES_FILE_READ_ERROR, errbuf);
            return(RULES_FILE_READ_ERROR);
	}
    Pointer *e = newPointer(file, ruleBaseName);
    int ret = parseRuleSet(e, ruleSet, funcDesc, errloc, errmsg, r);
    deletePointer(e);
    if(ret < 0) {
        return ret;
    }

	Node *errnode;
	ExprType *restype = typeRuleSet(ruleSet, errmsg, &errnode, r);
	if(getNodeType(restype) == T_ERROR) {
	    *errloc = NODE_EXPR_POS(errnode);
	    return RE_TYPE_ERROR;
	}

	return 0;
}
execCmdOut_t *addCmdExecOutToEnv(Env *global, Region *r) {
    execCmdOut_t *ruleExecOut = (execCmdOut_t *)malloc (sizeof (execCmdOut_t));
    memset (ruleExecOut, 0, sizeof (execCmdOut_t));
    ruleExecOut->stdoutBuf.buf = strdup("");
    ruleExecOut->stdoutBuf.len = 0;
    ruleExecOut->stderrBuf.buf = strdup("");
    ruleExecOut->stderrBuf.len = 0;
    Res *execOutRes = newUninterpretedRes(r, ExecCmdOut_MS_T, ruleExecOut, NULL);
    insertIntoHashTable(global->current, "ruleExecOut", execOutRes);
    return ruleExecOut;

}

void freeCmdExecOut(execCmdOut_t *ruleExecOut) {
    if(ruleExecOut!=NULL) {
		if(ruleExecOut->stdoutBuf.buf!=0) {
			free(ruleExecOut->stdoutBuf.buf);
		}
		if(ruleExecOut->stderrBuf.buf!=0) {
			free(ruleExecOut->stderrBuf.buf);
		}
		free(ruleExecOut);
    }

}

int parseAndComputeMsParamArrayToEnv(msParamArray_t *var, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r) {
    int i;
	for(i=0;i<var->len;i++) {
		Res *res = newRes(r);
		int ret = convertMsParamToRes(var->msParam[i], res, errmsg, r);
        if(ret != 0) {
            return ret;
        }
		char *varName = var->msParam[i]->label;
        if(TYPE(res) == T_UNSPECED) {
            if(varName!=NULL) {
            	updateInEnv(env, varName, res);
            }
        	continue;
        }
        if(TYPE(res) != T_STRING) {
            return -1;
        }

		char *expr = res->text;
		res = parseAndComputeExpression(expr, env, rei, reiSaveFlag, errmsg, r);
		if(getNodeType(res)  ==  N_ERROR) {
		    return RES_ERR_CODE(res);
		}
        if(varName!=NULL) {
            updateInEnv(env, varName, res);
        }
	}
    return 0;

}
Env *defaultEnv(Region *r) {
    Env *global = newEnv(newHashTable2(10, r), NULL, NULL, r);
    Env *env = newEnv(newHashTable2(10, r), global, NULL, r);

    return env;
}

int parseAndComputeRuleAdapter(char *rule, msParamArray_t *msParamArray, ruleExecInfo_t *rei, int reiSaveFlag, Region *r) {
    /* set clearDelayed to 0 so that nested calls to this function do not call clearDelay() */
    int recclearDelayed = ruleEngineConfig.clearDelayed;
    ruleEngineConfig.clearDelayed = 0;

    rError_t errmsgBuf;
    errmsgBuf.errMsg = NULL;
    errmsgBuf.len = 0;

    Env *env = defaultEnv(r);

    rei->status = 0;

    msParamArray_t *orig = NULL;

    Res *execOutRes;

    int rescode = 0;
    if(msParamArray!=NULL) {
    	if(strncmp(rule, "@external\n", 10) == 0) {
            rescode = parseAndComputeMsParamArrayToEnv(msParamArray, globalEnv(env), rei, reiSaveFlag, &errmsgBuf, r);
            RE_ERROR(rescode < 0);
            rule = rule + 10;
    	} else {
    		rescode = convertMsParamArrayToEnv(msParamArray, globalEnv(env), &errmsgBuf, r);
            RE_ERROR(rescode < 0);
    	}
    }

    if((execOutRes = (Res *)lookupFromEnv(env, "ruleExecOut")) == NULL || TYPE(execOutRes) == T_UNSPECED) {
    	/* add cmdExecOut only if ruleExecOut is an output parameter */
    	deleteFromHashTable(globalEnv(env)->current, "ruleExecOut");
    	addCmdExecOutToEnv(globalEnv(env), r);
    }

    orig = rei->msParamArray;
    rei->msParamArray = NULL;

    rescode = parseAndComputeRule(rule, env, rei, reiSaveFlag, &errmsgBuf, r);

    if(orig==NULL) {
        rei->msParamArray = newMsParamArray();
    } else {
    	rei->msParamArray = orig;
    }
    convertEnvToMsParamArray(rei->msParamArray, env, &errmsgBuf, r);

    RE_ERROR(rescode < 0);
    freeRErrorContent(&errmsgBuf);
    /* deleteEnv(env, 3); */

    return rescode;
error:
    logErrMsg(&errmsgBuf, &rei->rsComm->rError);
    rei->status = rescode;
    freeRErrorContent(&errmsgBuf);
    /* deleteEnv(env, 3); */
    if(recclearDelayed) {
    clearDelayed();
    }
    ruleEngineConfig.clearDelayed = recclearDelayed;

    return rescode;


}

int parseAndComputeRuleNewEnv( char *rule, ruleExecInfo_t *rei, int reiSaveFlag, msParamArray_t *msParamArray, rError_t *errmsg, Region *r) {
    Env *env = defaultEnv(r);
    addCmdExecOutToEnv(globalEnv(env), r);

    int rescode = 0;
    msParamArray_t *orig = NULL;

    if(msParamArray!=NULL) {
        rescode = convertMsParamArrayToEnv(msParamArray, env->previous, errmsg, r);
        RE_ERROR(rescode < 0);
    }

    orig = rei->msParamArray;
    rei->msParamArray = NULL;

    rescode = parseAndComputeRule(rule, env, rei, reiSaveFlag, errmsg, r);
    RE_ERROR(rescode < 0);

    if(orig==NULL) {
        rei->msParamArray = newMsParamArray();
    } else {
    	rei->msParamArray = orig;
    }
    rescode = convertEnvToMsParamArray(rei->msParamArray, env, errmsg, r);
    RE_ERROR(rescode < 0);
    /* deleteEnv(env, 3); */
    return rescode;

error:

    /* deleteEnv(env, 3); */
    return rescode;
}

/* parse and compute a rule */
int parseAndComputeRule(char *rule, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r) {
	if(overflow(rule, MAX_RULE_LEN)) {
		addRErrorMsg(errmsg, RE_BUFFER_OVERFLOW, "error: potential buffer overflow");
		return RE_BUFFER_OVERFLOW;
	}
	Node *node;
    Pointer *e = newPointer2(rule);
    if(e == NULL) {
        addRErrorMsg(errmsg, RE_POINTER_ERROR, "error: can not create a Pointer.");
        return RE_POINTER_ERROR;
    }

    int tempLen = ruleEngineConfig.extRuleSet->len;

    int checkPoint=checkPointExtRuleSet(r);

    int rescode;

    int errloc;

    /* add rules into ext rule set */
    rescode = parseRuleSet(e, ruleEngineConfig.extRuleSet, ruleEngineConfig.extFuncDescIndex, &errloc, errmsg, r);
    deletePointer(e);
    if(rescode != 0) {
        return RE_PARSER_ERROR;
    }

    RuleDesc *rd = NULL;
    Res *res = NULL;
    /* add rules into rule index */
	int i;
	for(i=tempLen;i<ruleEngineConfig.extRuleSet->len;i++) {
		if(ruleEngineConfig.extRuleSet->rules[i]->ruleType == RK_FUNC || ruleEngineConfig.extRuleSet->rules[i]->ruleType == RK_REL) {
			appendRuleIntoExtIndex(ruleEngineConfig.extRuleSet->rules[i], i, r);
		}
	}

	for(i=tempLen;i<ruleEngineConfig.extRuleSet->len;i++) {
		if(ruleEngineConfig.extRuleSet->rules[i]->ruleType == RK_FUNC || ruleEngineConfig.extRuleSet->rules[i]->ruleType == RK_REL) {
			Hashtable *varTypes = newHashTable2(10, r);

			List *typingConstraints = newList(r);
			Node *errnode;
			ExprType *type = typeRule(ruleEngineConfig.extRuleSet->rules[i], ruleEngineConfig.extFuncDescIndex, varTypes, typingConstraints, errmsg, &errnode, r);

			if(getNodeType(type)==T_ERROR) {
/*				rescode = TYPE_ERROR;     #   TGR, since renamed to RE_TYPE_ERROR */
				rescode = RE_TYPE_ERROR;
				RETURN;
			}
		}
	}

    /* exec the first rule */
    rd = ruleEngineConfig.extRuleSet->rules[tempLen];
	node = rd->node;

	res = execRuleNodeRes(node, NULL, 0, GlobalAllRuleExecFlag, env, rei, reiSaveFlag, errmsg,r);
	rescode = getNodeType(res)  ==  N_ERROR? RES_ERR_CODE(res):0;
ret:
    /* remove rules from ext rule set */
    popExtRuleSet(checkPoint);

    return rescode;
}

/* call an action with actionName and string parameters */
Res *computeExpressionWithParams( char *actionName, char **params, int paramsCount, ruleExecInfo_t *rei, int reiSaveFlag, msParamArray_t *msParamArray, rError_t *errmsg, Region *r) {
#ifdef DEBUG
    char buf[ERR_MSG_LEN>1024?ERR_MSG_LEN:1024];
    snprintf(buf, 1024, "computExpressionWithParams: %s\n", actionName);
    writeToTmp("entry.log", buf);
#endif
    /* set clearDelayed to 0 so that nested calls to this function do not call clearDelay() */
    int recclearDelayed = ruleEngineConfig.clearDelayed;
    ruleEngineConfig.clearDelayed = 0;

    if(overflow(actionName, MAX_NAME_LEN)) {
            addRErrorMsg(errmsg, RE_BUFFER_OVERFLOW, "error: potential buffer overflow");
            return newErrorRes(r, RE_BUFFER_OVERFLOW);
    }
    int k;
    for(k=0;k<paramsCount;k++) {
        if(overflow(params[k], MAX_RULE_LEN)) {
            addRErrorMsg(errmsg, RE_BUFFER_OVERFLOW, "error: potential buffer overflow");
            return newErrorRes(r, RE_BUFFER_OVERFLOW);
        }
    }

    Node** paramNodes = (Node **)region_alloc(r, sizeof(Node *)*paramsCount);
    int i;
    for(i=0;i<paramsCount;i++) {

        Node *node;

        /*Pointer *e = newPointer2(params[i]);

        if(e == NULL) {
            addRErrorMsg(errmsg, -1, "error: can not create Pointer.");
            return newErrorRes(r, -1);
        }

        node = parseTermRuleGen(e, 1, errmsg, r);*/
        node = newNode(TK_STRING, params[i], 0, r);
        /*if(node==NULL) {
            addRErrorMsg(errmsg, OUT_OF_MEMORY, "error: out of memory.");
            return newErrorRes(r, OUT_OF_MEMORY);
        } else if (getNodeType(node) == N_ERROR) {
            return newErrorRes(r, RES_ERR_CODE(node));

        }*/

        paramNodes[i] = node;
    }

    Node *node = createFunctionNode(actionName, paramNodes, paramsCount, NULL, r);
    Env *global = newEnv(newHashTable2(10, r), NULL, NULL, r);
    Env *env = newEnv(newHashTable2(10, r), global, NULL, r);
    if(msParamArray!=NULL) {
        convertMsParamArrayToEnv(msParamArray, global, errmsg, r);
    }
    Res *res = computeNode(node, NULL, env, rei, reiSaveFlag, errmsg,r);
    /* deleteEnv(env, 3); */
    if(recclearDelayed) {
    	clearDelayed();
    }
    ruleEngineConfig.clearDelayed = recclearDelayed;
    return res;
}
ExprType *typeRule(RuleDesc *rule, Env *funcDesc, Hashtable *varTypes, List *typingConstraints, rError_t *errmsg, Node **errnode, Region *r) {
            /* printf("%s\n", node->subtrees[0]->text); */
			addRErrorMsg(errmsg, -1, ERR_MSG_SEP);
            char buf[ERR_MSG_LEN];
            Node *node = rule->node;
            int dynamictyping = rule->dynamictyping;
#if 0
            int arity = RULE_NODE_NUM_PARAMS(node); /* subtrees[0] = param list */
            Node *type = node->subtrees[0]->subtrees[1]; /* subtrees[1] = return type */
#endif

            ExprType *resType = typeExpression3(node->subtrees[1], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode, r);
            /*printf("Type %d\n",resType->t); */
            RE_ERROR(getNodeType(resType) == T_ERROR);
            if(getNodeType(resType) != T_BOOL && getNodeType(resType) != T_VAR && getNodeType(resType) != T_DYNAMIC) {
            	char buf2[1024], buf3[ERR_MSG_LEN];
            	typeToString(resType, varTypes, buf2, 1024);
            	snprintf(buf3, ERR_MSG_LEN, "error: the type %s of the rule condition is not supported", buf2);
                generateErrMsg(buf3, NODE_EXPR_POS(node->subtrees[1]), node->subtrees[1]->base, buf);
                addRErrorMsg(errmsg, RE_TYPE_ERROR, buf);
                RE_ERROR(1);
            }
            resType = typeExpression3(node->subtrees[2], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode, r);
            RE_ERROR(getNodeType(resType) == T_ERROR);
            resType = typeExpression3(node->subtrees[3], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode, r);
            RE_ERROR(getNodeType(resType) == T_ERROR);
            /* printVarTypeEnvToStdOut(varTypes); */
            RE_ERROR(solveConstraints(typingConstraints, varTypes, errmsg, errnode, r) == ABSURDITY);
            int i;
            for(i =1;i<=3;i++) { // 1 = cond, 2 = actions, 3 = recovery
                postProcessCoercion(node->subtrees[i], varTypes, errmsg, errnode, r);
                postProcessActions(node->subtrees[i], funcDesc, errmsg, errnode, r);
            }
            /*printTree(node, 0); */
            return newSimpType(T_INT, r);

        error:
            snprintf(buf, ERR_MSG_LEN, "type error: in rule %s", node->subtrees[0]->text);
            addRErrorMsg(errmsg, RE_TYPE_ERROR, buf);
            return resType;

}

ExprType *typeRuleSet(RuleSet *ruleset, rError_t *errmsg, Node **errnode, Region *r) {
    Env *funcDesc = ruleEngineConfig.extFuncDescIndex;
    Hashtable *ruleType = newHashTable2(MAX_NUM_RULES * 2, r);
    ExprType *res;
    int i;
    for(i=0;i<ruleset->len;i++) {
        RuleDesc *rule = ruleset->rules[i];
        if(rule->ruleType == RK_REL || rule->ruleType == RK_FUNC) {
        	List *typingConstraints = newList(r);
			Hashtable *varTypes = newHashTable2(100, r);
			ExprType *restype = typeRule(rule, funcDesc, varTypes, typingConstraints, errmsg, errnode, r);
        /*char buf[1024]; */
        /*typingConstraintsToString(typingConstraints, NULL, buf, 1024); */
        /*printf("rule %s, typing constraints: %s\n", ruleset->rules[i]->subtrees[0]->text, buf); */
			if(getNodeType(restype) == T_ERROR) {
				res = restype;
				char *errbuf = (char *) malloc(ERR_MSG_LEN*1024*sizeof(char));
				errMsgToString(errmsg, errbuf, ERR_MSG_LEN*1024);
#ifdef DEBUG
				writeToTmp("ruleerr.log", errbuf);
				writeToTmp("ruleerr.log", "\n");
#endif
				rodsLog (LOG_ERROR, "%s", errbuf);
				free(errbuf);
				freeRErrorContent(errmsg);
				RETURN;
			}
			/* check that function names are unique and do not conflict with system msis */
			char errbuf[ERR_MSG_LEN];
			char *ruleName = rule->node->subtrees[0]->text;
			FunctionDesc *fd;
			if((fd = (FunctionDesc *)lookupFromEnv(funcDesc, ruleName)) != NULL) {
				if(getNodeType(fd) != N_FD_EXTERNAL && getNodeType(fd) != N_FD_RULE_INDEX_LIST) {
					char *err;
					switch(getNodeType(fd)) {
					case N_FD_CONSTRUCTOR:
						err = "redefinition of constructor";
						break;
					case N_FD_DECONSTRUCTOR:
						err = "redefinition of deconstructor";
						break;
					case N_FD_FUNCTION:
						err = "redefinition of system microservice";
						break;
					default:
						err = "redefinition of system symbol";
						break;
					}

					generateErrMsg(err, NODE_EXPR_POS(rule->node), rule->node->base, errbuf);
					addRErrorMsg(errmsg, RE_FUNCTION_REDEFINITION, errbuf);
					res = newErrorType(RE_FUNCTION_REDEFINITION, r);
					*errnode = rule->node;
					RETURN;
				}
			}

			RuleDesc *rd = (RuleDesc *)lookupFromHashTable(ruleType, ruleName);
			if(rd!=NULL) {
				if(rule->ruleType == RK_FUNC || rd ->ruleType == RK_FUNC) {
					generateErrMsg("redefinition of function", NODE_EXPR_POS(rule->node), rule->node->base, errbuf);
					addRErrorMsg(errmsg, RE_FUNCTION_REDEFINITION, errbuf);
					generateErrMsg("previous definition", NODE_EXPR_POS(rd->node), rd->node->base, errbuf);
					addRErrorMsg(errmsg, RE_FUNCTION_REDEFINITION, errbuf);
					res = newErrorType(RE_FUNCTION_REDEFINITION, r);
					*errnode = rule->node;
					RETURN;
				}
            } else {
                insertIntoHashTable(ruleType, ruleName, rule);
            }
        }
    }
    res = newSimpType(T_INT, r); /* Although a rule set does not have type T_INT, return T_INT to indicate success. */

ret:
    return res;
}

int typeNode(Node *node, Hashtable *varTypes, rError_t *errmsg, Node **errnode, Region *r)
{
    if((node->option & OPTION_TYPED) == 0) {
        /*printTree(node, 0); */
        List *typingConstraints = newList(r);
        Res *resType = typeExpression3(node, 0, ruleEngineConfig.extFuncDescIndex, varTypes, typingConstraints, errmsg, errnode, r);
        /*printf("Type %d\n",resType->t); */
        if(getNodeType(resType) == T_ERROR) {
            addRErrorMsg(errmsg, RE_TYPE_ERROR, "type error: in rule");
            return RE_TYPE_ERROR;
        }
        postProcessCoercion(node, varTypes, errmsg, errnode, r);
        postProcessActions(node, ruleEngineConfig.extFuncDescIndex, errmsg, errnode, r);
        /*    printTree(node, 0); */
        varTypes = NULL;
        node->option |= OPTION_TYPED;
    }
    return 0;
}

/* compute an expression or action given by an AST node */
Res* computeNode(Node *node, Node *reco, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t* errmsg, Region *r) {
    Hashtable *varTypes = newHashTable2(10, r);
    Region *rNew = make_region(0, NULL);
    Node *en;
    Node **errnode = &en;
    Res* res;
    int errorcode;
    if((errorcode = typeNode(node, varTypes, errmsg, errnode, r))!=0) {
        res = newErrorRes(r,errorcode);
        RETURN;
    }
    if(reco != NULL && (errorcode = typeNode(reco, varTypes, errmsg, errnode, r))!=0) {
        res = newErrorRes(r,errorcode);
        RETURN;
    }
    if(getNodeType(node) == N_ACTIONS) {
    	res = evaluateActions(node, NULL, GlobalAllRuleExecFlag, rei, reiSaveFlag, env, errmsg, rNew);
    } else {
    	res = evaluateExpression3(node, GlobalAllRuleExecFlag, 0, rei, reiSaveFlag, env, errmsg, rNew);
    }

/*    switch (TYPE(res)) {
        case T_ERROR:
            addRErrorMsg(errmsg, -1, "error: in rule");
            break;
        default:
            break;
    }*/
    ret:
    res = cpRes(res, r);
    cpEnv(env, r);
    region_free(rNew);
    return res;
}

/* parse and compute an expression
 *
 */
Res *parseAndComputeExpression(char *expr, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r) {
    Res *res = NULL;
    char buf[ERR_MSG_LEN>1024?ERR_MSG_LEN:1024];
    int rulegen;
    Node *node = NULL, *recoNode = NULL;

#ifdef DEBUG
    snprintf(buf, 1024, "parseAndComputeExpression: %s\n", expr);
    writeToTmp("entry.log", buf);
#endif
    if(overflow(expr, MAX_RULE_LEN)) {
            addRErrorMsg(errmsg, RE_BUFFER_OVERFLOW, "error: potential buffer overflow");
            return newErrorRes(r, RE_BUFFER_OVERFLOW);
    }
    Pointer *e = newPointer2(expr);
    ParserContext *pc = newParserContext(errmsg, r);
    if(e == NULL) {
        addRErrorMsg(errmsg, RE_POINTER_ERROR, "error: can not create pointer.");
        res = newErrorRes(r, RE_POINTER_ERROR);
        RETURN;
    }
    rulegen = isRuleGenSyntax(expr);

    if(rulegen) {
    	node = parseTermRuleGen(e, rulegen, pc);
    } else {
    	node= parseActionsRuleGen(e, rulegen, 1, pc);
    }
    if(node==NULL) {
            addRErrorMsg(errmsg, RE_OUT_OF_MEMORY, "error: out of memory.");
            res = newErrorRes(r, RE_OUT_OF_MEMORY);
            RETURN;
    } else if (getNodeType(node) == N_ERROR) {
            generateErrMsg("error: syntax error",NODE_EXPR_POS(node), node->base, buf);
            addRErrorMsg(errmsg, RE_PARSER_ERROR, buf);
            res = newErrorRes(r, RE_PARSER_ERROR);
            RETURN;
    } else {
        Token *token;
        token = nextTokenRuleGen(e, pc, 0);
        if(strcmp(token->text, "|")==0) {
        	recoNode = parseActionsRuleGen(e, rulegen, 1, pc);
            if(node==NULL) {
				addRErrorMsg(errmsg, RE_OUT_OF_MEMORY, "error: out of memory.");
				res = newErrorRes(r, RE_OUT_OF_MEMORY);
				RETURN;
            } else if (getNodeType(node) == N_ERROR) {
				generateErrMsg("error: syntax error",NODE_EXPR_POS(node), node->base, buf);
				addRErrorMsg(errmsg, RE_PARSER_ERROR, buf);
				res = newErrorRes(r, RE_PARSER_ERROR);
				RETURN;
            }
            token = nextTokenRuleGen(e, pc, 0);
        }
        if(token->type!=TK_EOS) {
            Label pos;
            getFPos(&pos, e, pc);
            generateErrMsg("error: unparsed suffix",pos.exprloc, pos.base, buf);
            addRErrorMsg(errmsg, RE_UNPARSED_SUFFIX, buf);
            res = newErrorRes(r, RE_UNPARSED_SUFFIX);
            RETURN;
        }
    }
	res = computeNode(node, NULL, env, rei, reiSaveFlag, errmsg,r);
    ret:
    deleteParserContext(pc);
    deletePointer(e);
    return res;
}

int generateRuleTypes(RuleSet *inRuleSet, Hashtable *symbol_type_table, Region *r)
{
	int i;
	for (i=0;i<inRuleSet->len;i++) {
            Node *ruleNode = inRuleSet->rules[i]->node;
            if(ruleNode == NULL)
                continue;
            char *key = ruleNode->subtrees[0]->text;
            int arity = RULE_NODE_NUM_PARAMS(ruleNode);

            ExprType **paramTypes = (ExprType**) region_alloc(r, sizeof(ExprType *)*arity);
            int k;
            for(k=0;k<arity;k++) {
                paramTypes[k] = newTVar(r);
            }
            ExprType *ruleType = newFuncTypeVarArg(arity, OPTION_VARARG_ONCE, paramTypes, newSimpType(T_INT, r), r);

            if (insertIntoHashTable(symbol_type_table, key,ruleType) == 0) {
                    return 0;
            }
	}
	return 1;
}

RuleDesc* getRuleDesc(int ri)
{

	if(ri< APP_RULE_INDEX_OFF) {
		return ruleEngineConfig.extRuleSet->rules[ri];
	} else
	if (ri < CORE_RULE_INDEX_OFF) {
		ri = ri - APP_RULE_INDEX_OFF;
		return ruleEngineConfig.appRuleSet->rules[ri];
	} else {
		ri = ri - CORE_RULE_INDEX_OFF;
		return ruleEngineConfig.coreRuleSet->rules[ri];
	}
}

#ifdef USE_EIRODS
// =-=-=-=-=-=-=-
// function to look up and / or load a microservice for execution
int actionTableLookUp ( eirods::ms_table_entry& _entry, char* _action ) {

	std::string str_act( _action );

    if( str_act[0] == 'a' && str_act[1] == 'c' )
		return -1;
	
	// =-=-=-=-=-=-=
	// look up Action in microservice table.  If it returns
	// the end() iterator, is is not found so try to load it.
	if( !MicrosTable.has_entry( str_act ) ) {
		rodsLog( LOG_NOTICE, "actionTableLookUp - [%s] not found, load it.", _action );
		eirods::error ret = eirods::load_microservice_plugin( MicrosTable, str_act );
		if( !ret.ok() ) {
			return UNMATCHED_ACTION_ERR;
		} else { // if loaded
			rodsLog( LOG_NOTICE, "actionTableLookUp - loaded [%s]", _action );
		} // else
	}  // if not found

	_entry = *MicrosTable[ str_act ];

	return 0;

} // actionTableLookUp
#else
int actionTableLookUp (char *action)
{

	int i;

	for (i = 0; i < NumOfAction; i++) {
		if (!strcmp(MicrosTable[i].action,action))
			return (i);
	}

	return (UNMATCHED_ACTION_ERR);
}
#endif


/*
 * Set retOutParam to 1 if you need to retrieve the output parameters from inMsParamArray and 0 if not
 */
Res *parseAndComputeExpressionAdapter(char *inAction, msParamArray_t *inMsParamArray, int retOutParams, ruleExecInfo_t *rei, int reiSaveFlag, Region *r) { // JMC - backport 4540
    /* set clearDelayed to 0 so that nested calls to this function do not call clearDelay() */
    int recclearDelayed = ruleEngineConfig.clearDelayed;
    ruleEngineConfig.clearDelayed = 0;
    int freeRei = 0;
    if(rei == NULL) {
        rei = (ruleExecInfo_t *) malloc(sizeof(ruleExecInfo_t));
        memset(rei, 0, sizeof(ruleExecInfo_t));
        freeRei = 1;
    }
    rei->status = 0;
    Env *env = defaultEnv(r);
    /* retrieve generated data here as it may be overridden by convertMsParamArrayToEnv */
    execCmdOut_t *execOut = addCmdExecOutToEnv(globalEnv(env), r);

    Res *res;
    rError_t errmsgBuf;
    errmsgBuf.errMsg = NULL;
    errmsgBuf.len = 0;

    msParamArray_t *orig = rei->msParamArray;
    rei->msParamArray = NULL;

    if(inMsParamArray!=NULL) {
        convertMsParamArrayToEnv(inMsParamArray, env, &errmsgBuf, r);
    }

    res = parseAndComputeExpression(inAction, env, rei, reiSaveFlag, &errmsgBuf, r);
    if(retOutParams) { // JMC - backport 4540
       if(inMsParamArray != NULL) {
                       clearMsParamArray(inMsParamArray, 0);
                       convertEnvToMsParamArray(inMsParamArray, env, &errmsgBuf, r);
               }
    }
    rei->msParamArray = orig;

	freeCmdExecOut(execOut);
    /* deleteEnv(env, 3); */
    if(getNodeType(res) == N_ERROR && !freeRei) {
        logErrMsg(&errmsgBuf, &rei->rsComm->rError);
        rei->status = RES_ERR_CODE(res);
    }
    freeRErrorContent(&errmsgBuf);
    if(freeRei) {
        free(rei);
    }
    if(recclearDelayed) {
    	clearDelayed();
    }
    ruleEngineConfig.clearDelayed = recclearDelayed;
    return res;

}
int overflow(char* expr, int len) {
	int i;
	for(i = 0;i<len+1;i++) {
		if(expr[i]==0)
			return 0;
	}
	return 1;
}

