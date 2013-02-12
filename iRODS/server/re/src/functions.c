/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "index.h"
#include "functions.h"
#include "arithmetics.h"
#include "datetime.h"
#include "cache.h"
#include "configuration.h"
#ifndef DEBUG
#include "apiHeaderAll.h"
#include "rsApiHandler.h"
#endif


#if defined(USE_BOOST)
#include <boost/regex.h>
#elif defined(_POSIX_VERSION)
#include <regex.h>
#endif


#ifndef DEBUG
#include "execMyRule.h"
#include "msParam.h"
#include "reFuncDefs.h"
#include "rsMisc.h"
#include "stringOpr.h"
#include "miscServerFunct.h"
#endif

#define GC_BEGIN Region *_rnew = make_region(0, NULL), *_rnew2 = NULL;
#define GC_REGION _rnew
#define GC_ON(env) \
if(region_size(_rnew) > DEFAULT_BLOCK_SIZE) {\
_rnew2 = make_region(0, NULL); \
cpEnv2((env), _rnew, _rnew2); \
region_free(_rnew); \
_rnew = _rnew2;}
#define GC_END region_free(_rnew);

#define RE_BACKWARD_COMPATIBLE

int fileConcatenate(char *file1, char *file2, char *file3);

Node *wrapToActions(Node *node, Region *r) {
    if(getNodeType(node)!=N_ACTIONS) {
        Node *actions[1];
        actions[0] = node;
        Label expr;
        expr.base = node->base;
        expr.exprloc = NODE_EXPR_POS(node);
        return createActionsNode(actions, 1, &expr, r);
    }
    return node;
}
Res *smsi_ifExec(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r) {
    Res *res = evaluateExpression3((Node *)params[0], 0, 1, rei,reiSaveFlag,env,errmsg,r);
    if(getNodeType(res) == N_ERROR) {
        return res;
    }
    if(RES_BOOL_VAL(res) == 0) {
        return evaluateActions(wrapToActions(params[2],r), wrapToActions(params[4],r),0,rei, reiSaveFlag, env, errmsg, r);
    } else {
        return evaluateActions(wrapToActions(params[1],r), wrapToActions(params[3],r),0,rei, reiSaveFlag, env, errmsg, r);
    }
}

Res *smsi_if2Exec(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r) {
    Res *res = evaluateExpression3((Node *)params[0], 0,1,rei,reiSaveFlag,env,errmsg,r);
    if(getNodeType(res) == N_ERROR) {
        return res;
    }
    if(RES_BOOL_VAL(res) == 0) {
        return evaluateExpression3((Node *)params[2], 0, 1, rei,reiSaveFlag,env,errmsg,r);
    } else {
        return evaluateExpression3((Node *)params[1], 0, 1, rei,reiSaveFlag,env,errmsg,r);
    }
}

Res *smsi_do(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        switch(getNodeType(params[0])) {
            case N_ACTIONS:
                return evaluateActions((Node *)params[0], NULL,0, rei, reiSaveFlag,env, errmsg, r);
            default:
                return evaluateExpression3((Node *)params[0], 0, 1, rei,reiSaveFlag,env,errmsg,r);
        }

}
Res *smsi_letExec(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r) {
    Res *res = evaluateExpression3(params[1], 0, 1,rei,reiSaveFlag,env,errmsg,r);
    if(getNodeType(res) == N_ERROR) {
            return res;
    }
    Env *nEnv = newEnv(newHashTable2(100, r), env, NULL, r);
    Res *pres = matchPattern(params[0], res, nEnv, rei, reiSaveFlag, errmsg, r);
    if(getNodeType(pres) == N_ERROR) {
        /*deleteEnv(nEnv, 1); */
        return pres;
    }
/*                printTree(params[2], 0); */
    res = evaluateExpression3(params[2], 0,1, rei,reiSaveFlag,nEnv,errmsg,r);
    /* deleteEnv(nEnv, 1); */
    return res;
}
Res *smsi_matchExec(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r) {
    Res *res = evaluateExpression3(params[0], 0, 1,rei,reiSaveFlag,env,errmsg,r);
    if(getNodeType(res) == N_ERROR) {
            return res;
    }
    int i;
    for(i=1;i<n;i++) {
		Env *nEnv = newEnv(newHashTable2(100, r), env, NULL, r);
		Res *pres = matchPattern(params[i]->subtrees[0], res, nEnv, rei, reiSaveFlag, errmsg, r);
		if(getNodeType(pres) == N_ERROR) {
			/*deleteEnv(nEnv, 1); */
			addRErrorMsg(errmsg, RE_PATTERN_NOT_MATCHED, ERR_MSG_SEP);
			continue;
		}
		res = evaluateExpression3(params[i]->subtrees[1], 0,0, rei,reiSaveFlag,nEnv,errmsg,r);
		/*deleteEnv(nEnv, 1); */
		return res;
    }
    generateAndAddErrMsg("pattern not matched", node, RE_PATTERN_NOT_MATCHED, errmsg);
    return newErrorRes(r, RE_PATTERN_NOT_MATCHED);
}


Res *smsi_whileExec(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

    Res *cond, *res;
    GC_BEGIN
    while(1) {

        cond = evaluateExpression3((Node *)params[0], 0,1, rei,reiSaveFlag, env,errmsg,GC_REGION);
        if(getNodeType(cond) == N_ERROR) {
            res = cond;
            break;
        }
        if(RES_BOOL_VAL(cond) == 0) {
            res = newIntRes(r, 0);
            break;
        }
        res = evaluateActions((Node *)params[1],(Node *)params[2],0, rei,reiSaveFlag, env,errmsg,GC_REGION);
        if(getNodeType(res) == N_ERROR) {
            break;
        } else
        if(TYPE(res) == T_BREAK) {
            res = newIntRes(r, 0);
            break;
        } else
        if(TYPE(res) == T_SUCCESS) {
            break;
        }
        GC_ON(env);

    }
    cpEnv(env, r);
    res = cpRes(res, r);
    GC_END
    return res;

}

Res *smsi_forExec(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

    Res *init, *cond, *res, *step;
    Region *rnew = make_region(0, NULL);
    init = evaluateExpression3((Node *)params[0], 0,1,rei,reiSaveFlag, env,errmsg,rnew);
    if(getNodeType(init) == N_ERROR) {
        res = init;
        cpEnv(env, r);
        res = cpRes2(res, rnew, r);
        region_free(rnew);
        return res;
    }
    GC_BEGIN
    while(1) {

        cond = evaluateExpression3((Node *)params[1], 0, 1,rei,reiSaveFlag, env,errmsg,GC_REGION);
        if(getNodeType(cond) == N_ERROR) {
            break;
        }
        if(RES_BOOL_VAL(cond) == 0) {
            res = newIntRes(r, 0);
            break;
        }
        res = evaluateActions((Node *)params[3],(Node *)params[4], 0, rei,reiSaveFlag, env,errmsg,GC_REGION);
        if(getNodeType(res) == N_ERROR) {
            break;
        } else
        if(TYPE(res) == T_BREAK) {
            res =
                newIntRes(r, 0);
            break;
        } else
        if(TYPE(res) == T_SUCCESS) {
            break;
        }
        step = evaluateExpression3((Node *)params[2], 0,1,rei,reiSaveFlag, env,errmsg,GC_REGION);
        if(getNodeType(step) == N_ERROR) {
            break;
        }
        GC_ON(env);
    }
    cpEnv(env, r);
    res = cpRes(res, r);
    GC_END
    region_free(rnew);
    return res;

}

Res *smsi_split(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r);

Res *collType(Res *coll, Node *node, rError_t *errmsg, Region *r) {
	if(TYPE(coll) != T_STRING &&
			TYPE(coll) != T_CONS &&
			(TYPE(coll) != T_TUPLE || coll->degree != 2 ||
			 TYPE(coll->subtrees[0]) != T_IRODS || strcmp(coll->subtrees[0]->exprType->text, GenQueryInp_MS_T) != 0 ||
			 TYPE(coll->subtrees[1]) != T_IRODS || strcmp(coll->subtrees[1]->exprType->text, GenQueryOut_MS_T) != 0) &&
		(TYPE(coll) != T_IRODS || (
		strcmp(coll->exprType->text, StrArray_MS_T) != 0 &&
		strcmp(coll->exprType->text, IntArray_MS_T) != 0 &&
		strcmp(coll->exprType->text, GenQueryOut_MS_T) != 0 &&
		strcmp(coll->exprType->text, CollInp_MS_T) != 0))) {
		char errbuf[ERR_MSG_LEN];
		snprintf(errbuf, ERR_MSG_LEN, "%s is not a collection type.", typeName_Res(coll));
		generateAndAddErrMsg(errbuf, node, RE_DYNAMIC_TYPE_ERROR, errmsg);
		return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);
	} else {
		return coll;
	}
}

Res *collTypeBackwardCompatible(Res *coll, Node *node, rError_t *errmsg, Region *r) {
	if(TYPE(coll) == T_STRING) { /* backward compatible mode only */
		Res *paramsr[2];
		paramsr[0] = coll;
		paramsr[1] = newStringRes(r, ",");
		return smsi_split(paramsr, 2, node, NULL, 0, NULL, errmsg, r);
	} else {
		return collType(coll, node, errmsg, r);
	}
}

int msiCloseGenQuery(msParam_t* genQueryInpParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei);
int msiGetMoreRows(msParam_t* genQueryInpParam, msParam_t* genQueryOutParam, msParam_t *contInxParam, ruleExecInfo_t *rei);
Res *smsiCollectionSpider(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r);

Res *smsi_forEach2Exec(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{
    Res *res = newRes(r);
    char* varName = ((Node *)subtrees[0])->text;
	Res* coll = evaluateExpression3(subtrees[1], 0, 1, rei, reiSaveFlag, env, errmsg, r);
	CASCADE_N_ERROR(coll = collType(coll, subtrees[1], errmsg, r));
	res = newIntRes(r, 0);
	if(TYPE(coll) == T_CONS && strcmp(coll->exprType->text, LIST) == 0) {
		int i;
		Res* elem;
		for(i=0;i<coll->degree;i++) {
				elem = coll->subtrees[i];
				setVariableValue(varName, elem, rei, env, errmsg, r);
				res = evaluateActions(subtrees[2], subtrees[3], 0, rei,reiSaveFlag, env,errmsg,r);
				if(getNodeType(res) == N_ERROR) {
					break;
				} else
				if(TYPE(res) == T_BREAK) {
					break;
				} else
				if(TYPE(res) == T_SUCCESS) {
					break;
				}
		}
		if(getNodeType(res) != N_ERROR) {
			res = newIntRes(r,0);
		}
	} else if (TYPE(coll) == T_TUPLE ) {
		int i;
		Res* elem;
		int cont = 1;
		msParam_t genQInpParam;
		msParam_t genQOutParam;
		msParam_t contInxParam;
		convertResToMsParam(&genQInpParam, coll->subtrees[0], errmsg);
		convertResToMsParam(&genQOutParam, coll->subtrees[1], errmsg);
		GC_BEGIN
		while(cont) {
			genQueryOut_t *genQueryOut = (genQueryOut_t*)RES_UNINTER_STRUCT(coll->subtrees[1]);
			cont = genQueryOut->continueInx > 0;
			int len = getCollectionSize(coll->subtrees[1]->exprType->text, RES_UNINTER_STRUCT(coll->subtrees[1]), r);
			for(i=0;i<len;i++) {
				GC_ON(env);
				elem = getValueFromCollection(coll->subtrees[1]->exprType->text, RES_UNINTER_STRUCT(coll->subtrees[1]), i, GC_REGION);
				setVariableValue(varName, elem, rei, env, errmsg, GC_REGION);
				res = evaluateActions((Node *)subtrees[2], (Node *)subtrees[3], 0, rei,reiSaveFlag,  env,errmsg,GC_REGION);
				clearKeyVal((keyValPair_t *)RES_UNINTER_STRUCT(elem));
				free(RES_UNINTER_STRUCT(elem));

				if(getNodeType(res) == N_ERROR) {
					cont = 0;
					break;
				}
				if(TYPE(res) == T_BREAK) {
					cont = 0;
					break;
				}
			}
			if(!cont) {
				break;
			}
			memset(&contInxParam, 0, sizeof(msParam_t));
			int status = msiGetMoreRows(&genQInpParam, &genQOutParam, &contInxParam, rei);
			clearMsParam(&contInxParam, 1);
			if(status < 0) {
				generateAndAddErrMsg("msiGetMoreRows error", node, status, errmsg);
				res = newErrorRes(GC_REGION, status);
				break;
			}
		}
		int status = msiCloseGenQuery(&genQInpParam, &genQOutParam, rei);
		clearMsParam(&genQInpParam, 0);
		clearMsParam(&genQOutParam, 0);
		if(status < 0) {
			generateAndAddErrMsg("msiCloseGenQuery error", node, status, errmsg);
		}

		cpEnv(env, r);
		res = cpRes(res, r);
		GC_END
		if(getNodeType(res) != N_ERROR) {
			res = newIntRes(r,0);
		}

	} else if (TYPE(coll) == T_STRING || (TYPE(coll) == T_IRODS && strcmp(RES_IRODS_TYPE(coll), CollInp_MS_T) == 0) ) {
		Node *subtreesNew[4];
		subtreesNew[0] = subtrees[0]; /* variable */
		subtreesNew[1] = coll; /* collection */
		subtreesNew[2] = subtrees[2]; /* actions */
		subtreesNew[3] = subtrees[3]; /* recovery actions */
		return smsiCollectionSpider(subtreesNew, n, node, rei, reiSaveFlag, env, errmsg, r);

	} else {
		int i;
		Res* elem;
		int len = getCollectionSize(coll->exprType->text, RES_UNINTER_STRUCT(coll), r);
		GC_BEGIN
		for(i=0;i<len;i++) {
			GC_ON(env);
				elem = getValueFromCollection(coll->exprType->text, RES_UNINTER_STRUCT(coll), i, GC_REGION);
				setVariableValue(varName, elem, rei, env, errmsg, GC_REGION);
				res = evaluateActions((Node *)subtrees[2], (Node *)subtrees[3], 0, rei,reiSaveFlag,  env,errmsg,GC_REGION);
                clearKeyVal((keyValPair_t *)RES_UNINTER_STRUCT(elem));
                free(RES_UNINTER_STRUCT(elem));

				if(getNodeType(res) == N_ERROR) {
						break;
				}
				if(TYPE(res) == T_BREAK) {
						break;
				}
		}
		cpEnv(env, r);
		res = cpRes(res, r);
		GC_END
		if(getNodeType(res) != N_ERROR) {
			res = newIntRes(r,0);
		}

	}
	return res;
}
Res *smsi_forEachExec(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{
    Res *res = newRes(r);
    char* varName = ((Node *)subtrees[0])->text;
        Res* orig = evaluateVar3(varName, ((Node *)subtrees[0]), rei, reiSaveFlag, env, errmsg, r);
        Res *coll;
        CASCADE_N_ERROR(coll = collTypeBackwardCompatible(orig, subtrees[0], errmsg, r));
        res = newIntRes(r, 0);
        if(TYPE(coll) == T_CONS && strcmp(coll->exprType->text, LIST) == 0) {
            int i;
            Res* elem;
            for(i=0;i<coll->degree;i++) {
                    elem = coll->subtrees[i];
                    setVariableValue(varName, elem, rei, env, errmsg, r);
                    res = evaluateActions(subtrees[1], subtrees[2], 0, rei,reiSaveFlag, env,errmsg,r);
                    if(getNodeType(res) == N_ERROR) {
                        break;
                    } else
                    if(TYPE(res) == T_BREAK) {
                        break;
                    } else
                    if(TYPE(res) == T_SUCCESS) {
                        break;
                    }
            }
            if(getNodeType(res) != N_ERROR) {
                res = newIntRes(r,0);
            }
        } else {
            int i;
            Res* elem;
            int len = getCollectionSize(coll->exprType->text, RES_UNINTER_STRUCT(coll), r);
            GC_BEGIN
            for(i=0;i<len;i++) {
            	GC_ON(env);
            		elem = getValueFromCollection(coll->exprType->text, RES_UNINTER_STRUCT(coll), i, GC_REGION);
                    setVariableValue(varName, elem, rei, env, errmsg, GC_REGION);
                    res = evaluateActions((Node *)subtrees[1], (Node *)subtrees[2], 0, rei,reiSaveFlag,  env,errmsg,GC_REGION);
                    clearKeyVal((keyValPair_t *)RES_UNINTER_STRUCT(elem));
                    free(RES_UNINTER_STRUCT(elem));

                    if(getNodeType(res) == N_ERROR) {
                            break;
                    }
                    if(TYPE(res) == T_BREAK) {
                            break;
                    }
            }
            cpEnv(env, r);
            res = cpRes(res, r);
            GC_END
            if(getNodeType(res) != N_ERROR) {
                res = newIntRes(r,0);
            }

        }
        setVariableValue(varName, orig, rei, env, errmsg, r);
        return res;
}

void columnToString(Node *n, char **queryStr, int *size) {
	if(strlen(n->text)==0) { /* no attribute function */
		PRINT(queryStr, size, "%s", n->subtrees[0]->text);
	} else {
		PRINT(queryStr, size, "%s", n->text);
		PRINT(queryStr, size, "(%s)", n->subtrees[0]->text);
	}


}

int
msiMakeGenQuery(msParam_t* selectListStr, msParam_t* condStr, msParam_t* genQueryInpParam, ruleExecInfo_t *rei);
int msiExecGenQuery(msParam_t* genQueryInParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei);

Res *smsi_query(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	char queryStr[1024];
	char condStr[1024];
	int size = 1024;
	char *p = queryStr;
	int where = 0;
	int i;
	for(i=0;i<n;i++) {
		switch(getNodeType(subtrees[i])) {
		case N_ATTR:
			columnToString(subtrees[i], &p, &size);
			break;
		case N_QUERY_COND:
			where = 1;
			break;
		default:
			generateAndAddErrMsg("unsupported node type", subtrees[i], RE_DYNAMIC_TYPE_ERROR, errmsg);
			return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);
		}
		if(where == 1) {
			break;
		}
		PRINT(&p, &size, "%s", ",");
	}
	if(p != queryStr) {
		p--;
		*p = '\0';
	}


	if(where == 1) {
		p = condStr;
		for(;i<n;i++) {
			switch(getNodeType(subtrees[i])) {
		case N_QUERY_COND:
			columnToString(subtrees[i]->subtrees[0], &p, &size);
			PRINT(&p, &size, " %s ", subtrees[i]->text);
			if(strcmp(subtrees[i]->text, "between")==0) {
				termToString(&p, &size, 0, MIN_PREC, subtrees[i]->subtrees[1], 1);
				PRINT(&p, &size, "%s", " ");
				termToString(&p, &size, 0, MIN_PREC, subtrees[i]->subtrees[2], 1);
			} else {
				termToString(&p, &size, 0, MIN_PREC, subtrees[i]->subtrees[1], 1);
			}
			break;
		default:
			generateAndAddErrMsg("unsupported node type", subtrees[i], RE_DYNAMIC_TYPE_ERROR, errmsg);
			return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);
		}
		if(i != n - 1) {
			PRINT(&p, &size, "%s", " AND ");
		}
		}
	}

	Region *rNew = make_region(0, NULL);
	msParam_t condParam;
	msParam_t selectListParam;
	msParam_t genQInpParam;
	msParam_t genQOutParam;
	memset(&genQInpParam, 0, sizeof(msParam_t));
	memset(&genQOutParam, 0, sizeof(msParam_t));
	convertResToMsParam(&condParam, newStringRes(rNew, condStr), errmsg);
	convertResToMsParam(&selectListParam, newStringRes(rNew, queryStr), errmsg);
	int status = msiMakeGenQuery(&selectListParam, &condParam, &genQInpParam, rei);
	clearMsParam(&condParam, 1);
	clearMsParam(&selectListParam, 1);
	if(status < 0) {
		region_free(rNew);
		generateAndAddErrMsg("msiMakeGenQuery error", node, status, errmsg);
		return newErrorRes(r, status);
	}
	status = msiExecGenQuery(&genQInpParam, &genQOutParam, rei);
	if(status < 0) {
		region_free(rNew);
		generateAndAddErrMsg("msiExecGenQuery error", node, status, errmsg);
		return newErrorRes(r, status);
	}
	Res *res = newRes(r);
	convertMsParamToResAndFreeNonIRODSType(&genQInpParam, res, errmsg, r);
	Res *res2 = newRes(r);
	convertMsParamToResAndFreeNonIRODSType(&genQOutParam, res2, errmsg, r);
	region_free(rNew);

	Res **comps = (Res **) region_alloc(r, sizeof(Res *) * 2);
	comps[0] = res;
	comps[1] = res2;

	return newTupleRes(2, comps, r);
}
Res *smsi_break(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

	Res *	res = newRes(r);
        res->exprType = newSimpType(T_BREAK, r);
        return res;
}
Res *smsi_succeed(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

	Res *	res = newRes(r);
        res->exprType = newSimpType(T_SUCCESS, r);
        return res;
}
Res *smsi_fail(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

	Res *	res = newErrorRes(r, n == 0 ?FAIL_ACTION_ENCOUNTERED_ERR:RES_INT_VAL(subtrees[0]));
	generateAndAddErrMsg("fail action encountered", node, RES_ERR_CODE(res), errmsg);
        return res;
}


Res *smsi_assign(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

    /* An smsi shares the same env as the enclosing rule. */
    /* Therefore, our modification to the env is reflected to the enclosing rule automatically. */
    Res *val = evaluateExpression3((Node *)subtrees[1], 0, 1, rei, reiSaveFlag,  env, errmsg,r);
    if(getNodeType(val) == N_ERROR) {
        return val;
    }
    Res *ret = matchPattern(subtrees[0], val, env, rei, reiSaveFlag, errmsg, r);

    return ret;
}

Res *smsi_listvars(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
/*
		char buf2[MAX_COND_LEN];
		char buf3[MAX_COND_LEN];
		sprintf(buf2, "len: %d\n", rei->msParamArray->len);
		for(i=0;i<rei->msParamArray->len;i++) {
			msParam_t *mP = rei->msParamArray->msParam[i];
			if(i!=0)strncat(buf2, ",", MAX_COND_LEN);
			strncat(buf2, mP->label, MAX_COND_LEN);
			strncat(buf2, "=", MAX_COND_LEN);
			if(mP->inOutStruct == NULL)
				strncat(buf2, "<null>", MAX_COND_LEN);
			else	{
				if (strcmp(mP->type, DOUBLE_MS_T) == 0) { // if the parameter is an integer
					snprintf(buf3, MAX_COND_LEN, "%f:",*(double *)mP->inOutStruct);
				} else if (strcmp(mP->type, INT_MS_T) == 0) { // if the parameter is an integer
					snprintf(buf3, MAX_COND_LEN, "%d:",*(int *)mP->inOutStruct);
				} else if (strcmp(mP->type, STR_MS_T) == 0) { // if the parameter is a string
					snprintf(buf3, MAX_COND_LEN, "%s:",(char *)mP->inOutStruct);
				} else if(strcmp(mP->type, DATETIME_MS_T) == 0) {
					snprintf(buf3, MAX_COND_LEN, "%ld:",*(time_t *)mP->inOutStruct);
				} else {
					snprintf(buf3, MAX_COND_LEN, "<value>:");
				}
				strncat(buf2, buf3, MAX_COND_LEN);
				strncat(buf2, mP->type, MAX_COND_LEN);
			}
		}
*/
        char buf[1024];
        printHashtable(env->current, buf);
        Res *res= newStringRes(r, buf);
        return res;
    }
Res *smsi_listcorerules(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *coll = newCollRes(ruleEngineConfig.coreRuleSet->len, newSimpType(T_STRING, r), r);
    int i;
    for(i=0;i<ruleEngineConfig.coreRuleSet->len;i++) {
        coll->subtrees[i] = newStringRes(r, ruleEngineConfig.coreRuleSet->rules[i]->node->subtrees[0]->text);
    }
    return coll;
}
Res *smsi_listapprules(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *coll = newCollRes(ruleEngineConfig.appRuleSet->len, newSimpType(T_STRING, r), r);
    int i;
    for(i=0;i<ruleEngineConfig.appRuleSet->len;i++) {
        coll->subtrees[i] = newStringRes(r, ruleEngineConfig.appRuleSet->rules[i]->node->subtrees[0]->text);
    }
    return coll;
}
Res *smsi_listextrules(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *coll = newCollRes(ruleEngineConfig.extRuleSet->len, newSimpType(T_STRING, r), r);
    int i;
    for(i=0;i<ruleEngineConfig.extRuleSet->len;i++) {
        coll->subtrees[i] = newStringRes(r, ruleEngineConfig.extRuleSet->rules[i]->node->subtrees[0]->text);
    }
    return coll;
}
Res *smsi_true(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        return newBoolRes(r, 1);
}
Res *smsi_false(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        return newBoolRes(r, 0);
}

Res *smsi_max(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
		int init=0;
		double max=0;
		int i;
		for(i=0;i<n;i++) {
			double x = params[i]->dval;
			if(init==0) {
				max = x;
				init = 1;
			} else {
				max = x > max? x: max;
			}
		}
		Res *res = newDoubleRes(r, max);
                return res;
        }
Res *smsi_min(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
		int init=0;
		double min=0;
		int i;
		for(i=0;i<n;i++) {
			double x = params[i]->dval;
			if(init==0) {
				min = x;
				init = 1;
			} else {
				min = x < min? x: min;
			}
		}
                Res *res = newDoubleRes(r, min);
                return res;
        }
        Res *smsi_average(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
		double sum=0;
		int i;
		for(i=0;i<n;i++) {
			double x = params[i]->dval;
			sum += x;
		}
		Res *res = newDoubleRes(r, sum/n);
                return res;
	}
        Res *smsi_hd(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
            if(params[0]->degree > 0) {
                return params[0]->subtrees[0];
            } else {
                generateAndAddErrMsg("error: hd: empty list", node, RE_RUNTIME_ERROR, errmsg);
                return newErrorRes(r, RE_RUNTIME_ERROR);
            }
	}
        Res *smsi_tl(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
            if(params[0]->degree > 0) {
		Res *res = newRes(r);
                ExprType *elemType = T_CONS_TYPE_ARG(((Res *)params[0])->exprType, 0);
		/* allocate memory for elements */
                res->exprType = newCollType(elemType, r);
                res->degree = params[0]->degree-1;
                res->subtrees = (Res **) region_alloc(r, sizeof(Res *)*res->degree);
		int i;
		for(i=0;i<res->degree;i++) {
			res->subtrees[i] = params[0]->subtrees[i+1];
		}
                return res;
            } else {
            	generateAndAddErrMsg("error: tl: empty list", node, RE_RUNTIME_ERROR, errmsg);
                return newErrorRes(r, RE_RUNTIME_ERROR);
            }
	}
        Res *smsi_cons(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
            Res *res = newRes(r);
            ExprType *elemType = params[0]->exprType;
            /* allocate memory for elements */
            res->exprType = newCollType(elemType, r);
            res->degree = params[1]->degree+1;
            res->subtrees = (Res **) region_alloc(r, sizeof(Res *)*res->degree);
            int i;
            res->subtrees[0] = params[0];
            for(i=1;i<res->degree;i++) {
                    res->subtrees[i] = params[1]->subtrees[i-1];
            }
                return res;
	}
        Res *smsi_setelem(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
            Res *res = newRes(r);
            Res *coll = params[0];
            Res *indexRes = params[1];
            Res *val = params[2];
            ExprType *elemType = coll->exprType->subtrees[0];
            int index = RES_INT_VAL(indexRes);
            if(0>index || index >= coll->degree) {
                char errbuf[ERR_MSG_LEN];
                snprintf(errbuf, ERR_MSG_LEN, "setelem: index out of bound %d", index);
                generateAndAddErrMsg(errbuf, node, RE_RUNTIME_ERROR, errmsg);
                return newErrorRes(r, RE_RUNTIME_ERROR);
            }

            /* allocate memory for elements */
            res->exprType = newCollType(elemType, r);
            res->degree = coll->degree;
            res->subtrees = (Res **) region_alloc(r, sizeof(Res *)*res->degree);
            memcpy(res->subtrees, coll->subtrees, sizeof(Res *)*res->degree);
            res->subtrees[index] = val;
            return res;
	}
        Res *smsi_list(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
            Res *res = newRes(r);
            ExprType *elemType =
                n == 0?newSimpType(T_UNSPECED, r):params[0]->exprType;
            /* allocate memory for elements */
            res->exprType = newCollType(elemType, r);
            res->degree = n;
            res->subtrees = (Res **) region_alloc(r, sizeof(Res *)*n);
            int i;
            for(i=0;i<n;i++) {
                    res->subtrees[i] = params[i];
            }
            return res;
	}
        Res *smsi_tuple(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
            Res *res = newRes(r);
            /* allocate memory for element types */
            ExprType **elemTypes = (ExprType **)region_alloc(r, n*sizeof(ExprType *));
            int i;
            for(i=0;i<n;i++) {
                elemTypes[i] = params[i]->exprType;
            }
            res->exprType = newConsType(n, cpStringExt(TUPLE, r), elemTypes, r);
            res->degree = n;
            res->text = cpStringExt(TUPLE, r);
            /* allocate memory for elements */
            res->subtrees = (Res **) region_alloc(r, sizeof(Res *)*n);
            for(i=0;i<n;i++) {
                res->subtrees[i] = params[i];
            }
            return res;
	}
        Res *smsi_elem(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
            char errbuf[ERR_MSG_LEN];
            int index = RES_INT_VAL(params[1]);
            if(TYPE(params[0]) == T_CONS) {
                if(index <0 || index >= params[0]->degree ) {
                    snprintf(errbuf, ERR_MSG_LEN, "error: index out of range %d.", index);
                    addRErrorMsg(errmsg, RE_RUNTIME_ERROR, errbuf);
                    return newErrorRes(r, RE_RUNTIME_ERROR);
                }
                Res *res = params[0]->subtrees[index];
                return res;
            } else {
                if(index <0 || index >= getCollectionSize(params[0]->exprType->text,
                        RES_UNINTER_STRUCT(params[0]), r) ) {
                    snprintf(errbuf, ERR_MSG_LEN, "error: index out of range %d. %s", index, ((Res *)params[0])->exprType->text);
                    addRErrorMsg(errmsg, RE_RUNTIME_ERROR, errbuf);
                    return newErrorRes(r, RE_RUNTIME_ERROR);
                }
                Res *res2 = getValueFromCollection(params[0]->exprType->text,
                        RES_UNINTER_STRUCT(params[0]),
                        index,r);

                return res2;
            }
	}
        Res *smsi_size(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
            Res * res = newRes(r);
            res->exprType = newSimpType(T_INT,r);
                if(TYPE(params[0]) == T_CONS) {
                    RES_INT_VAL_LVAL(res) = params[0]->degree;
                } else {
                    RES_INT_VAL_LVAL(res) = getCollectionSize(params[0]->exprType->text,
                            RES_UNINTER_STRUCT(params[0]),r);
                }
                return res;
	}
        Res *smsi_datetime(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
                char errbuf[ERR_MSG_LEN];
                Res *res = newRes(r);
		Res* timestr = params[0];
		char* format;
		if(TYPE(params[0])!=T_STRING ||
			(n == 2 && TYPE(params[1])!=T_STRING)) { /* error not a string */
                        res = newErrorRes(r, RE_UNSUPPORTED_OP_OR_TYPE);
                        snprintf(errbuf, ERR_MSG_LEN, "error: unsupported operator or type. can not apply datetime to type (%s[,%s]).", typeName_Res((Res *)params[0]), n==2?typeName_Res((Res *)params[1]):"null");
                        addRErrorMsg(errmsg, RE_UNSUPPORTED_OP_OR_TYPE, errbuf);
		} else {
			if(n == 2) {
				format = params[1]->text;
			} else {
				format="";
			}
			strttime(timestr->text, format, &(RES_TIME_VAL(res)));
                        res->exprType = newSimpType(T_DATETIME,r);
		}
                return res;
        }

Res *smsi_time(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        time_t t;
        time(&t);
        Res*res = newDatetimeRes(r, t);
        return res;
}
Res *smsi_timestr(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        char errbuf[ERR_MSG_LEN];
        Res *res = newRes(r);
        Res* dtime = params[0];
        char* format;
        if(TYPE(params[0])!=T_DATETIME ||
            (n == 2 && TYPE(params[1])!=T_STRING)) {
            res = newErrorRes(r, RE_UNSUPPORTED_OP_OR_TYPE);
            snprintf(errbuf, ERR_MSG_LEN, "error: unsupported operator or type. can not apply datetime to type (%s[,%s]).", typeName_Res((Res *)params[0]), n==2?typeName_Res((Res *)params[1]):"null");
            addRErrorMsg(errmsg, RE_UNSUPPORTED_OP_OR_TYPE, errbuf);
        } else {
            if(n == 2) {
                    format = params[1]->text;
            } else {
                    format = "";
            }
            char buf[1024];
            ttimestr(buf, 1024-1, format, &RES_TIME_VAL(dtime));
            res = newStringRes(r, buf);
        }
        return res;
}
Res *smsi_type(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        Res *val = params[0], *res;
        char typeName[128];
        typeToString(val->exprType, NULL, typeName, 128);
        res=newStringRes(r, typeName);
        return res;
}
Res *smsi_arity(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
		Res *val = params[0];
		RuleIndexListNode *ruleInxLstNode;
		if(findNextRule2(val->text, 0, &ruleInxLstNode)<0) {
			return newErrorRes(r, RE_RUNTIME_ERROR);
		}
		int ri;
		if(ruleInxLstNode->secondaryIndex) {
			return newErrorRes(r, RE_RUNTIME_ERROR);
		} else {
			ri = ruleInxLstNode->ruleIndex;
		}
		RuleDesc *rd = getRuleDesc(ri);
		return newIntRes(r, RULE_NODE_NUM_PARAMS(rd->node));
}
Res *smsi_str(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
                char errbuf[ERR_MSG_LEN];
                char errmsgbuf[ERR_MSG_LEN];
		Res *val = params[0], *res;
		if(TYPE(val) == T_INT
		|| TYPE(val) == T_DOUBLE
		|| TYPE(val) == T_BOOL
		|| TYPE(val) == T_CONS
		|| TYPE(val) == T_STRING
		|| TYPE(val) == T_DATETIME) {
                    char *buf = convertResToString(val);
                    if(buf != NULL) {
                        res = newStringRes(r, buf);
                        free(buf);
                    } else {
                        res = newErrorRes(r, RE_UNSUPPORTED_OP_OR_TYPE);
                        snprintf(errbuf, ERR_MSG_LEN, "error: converting value of type %s to string.", typeName_Res(val));
                        generateAndAddErrMsg(errbuf, node, RE_UNSUPPORTED_OP_OR_TYPE, errmsg);

                    }
		} else if(TYPE(val) == T_IRODS && strcmp(val->exprType->text, BUF_LEN_MS_T) == 0) {
			bytesBuf_t *buf = (bytesBuf_t *) RES_UNINTER_STRUCT(val);
			int len = buf->len;
			int i;
			for(i=0;i<len;i++) {
				if(((char *) buf->buf)[i] == '\0') {
					return newStringRes(r, (char *) buf->buf);
				}
			}
			char *tmp = (char *)malloc(len+1);
			memcpy(tmp, buf->buf, len);
			tmp[len] = '\0';
			return newStringRes(r, tmp);
		} else {
                    res = newErrorRes(r, RE_UNSUPPORTED_OP_OR_TYPE);
                    snprintf(errmsgbuf, ERR_MSG_LEN, "error: unsupported type. can not convert %s to string.", typeName_Res(val));
                    generateAndAddErrMsg(errbuf, node, RE_UNSUPPORTED_OP_OR_TYPE, errmsg);
		}
                return res;
}

Res *smsi_double(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
                char errbuf[ERR_MSG_LEN];
                Res *val = params[0], *res = newRes(r);
		if(TYPE(val) == T_STRING) {
                    res->exprType = newSimpType(T_DOUBLE,r);
                    RES_DOUBLE_VAL_LVAL(res) = atof(val->text);
		} else if(TYPE(val) == T_DATETIME) {
                    res->exprType = newSimpType(T_DOUBLE,r);
                    RES_DOUBLE_VAL_LVAL(res) = (double)RES_TIME_VAL(val);
                } else if(TYPE(val) == T_DOUBLE) {
                    res = val;
                } else {
                    res = newErrorRes(r, RE_UNSUPPORTED_OP_OR_TYPE);
                    snprintf(errbuf, ERR_MSG_LEN, "error: unsupported operator or type. can not convert %s to double.", typeName_Res(val));
                    generateAndAddErrMsg(errbuf, node, RE_UNSUPPORTED_OP_OR_TYPE, errmsg);
		}
                return res;
}
Res *smsi_int(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        char errbuf[ERR_MSG_LEN];
        Res *val = params[0], *res = newRes(r);
        if(TYPE(val) == T_STRING) {
            res->exprType = newSimpType(T_INT,r);
            RES_INT_VAL_LVAL(res) = atoi(val->text);
        } else if(TYPE(val) == T_DOUBLE) {
            res->exprType = newSimpType(T_INT, r);
            RES_INT_VAL_LVAL(res) = (int)RES_DOUBLE_VAL(val);
        } else if(TYPE(val) == T_INT) {
            res = val;
        } else {
            res = newErrorRes(r, RE_UNSUPPORTED_OP_OR_TYPE);
            snprintf(errbuf, ERR_MSG_LEN, "error: unsupported operator or type. can not convert %s to integer.", typeName_Res(val));
            generateAndAddErrMsg(errbuf, node, RE_UNSUPPORTED_OP_OR_TYPE, errmsg);
        }
        return res;
}
Res *smsi_bool(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        char errbuf[ERR_MSG_LEN];
        Res *val = params[0], *res = newRes(r);
        res->exprType = newSimpType(T_BOOL, r);
		if(TYPE(val) == T_BOOL) {
			res = val;
		} else if(TYPE(val) == T_STRING && (strcmp(val->text, "true")==0 || strcmp(val->text, "1") == 0)) {
        		RES_BOOL_VAL_LVAL(res) = 1;
		} else if(TYPE(val) == T_STRING && (strcmp(val->text, "false")==0 || strcmp(val->text, "0") == 0)) {
        		RES_BOOL_VAL_LVAL(res) = 0;
        } else if(TYPE(val) == T_DOUBLE) {
            RES_BOOL_VAL_LVAL(res) = (int)RES_DOUBLE_VAL(val)?1:0;
        } else if(TYPE(val) == T_INT) {
            RES_BOOL_VAL_LVAL(res) = (int)RES_INT_VAL(val)?1:0;
        } else {
            res = newErrorRes(r, RE_UNSUPPORTED_OP_OR_TYPE);
            snprintf(errbuf, ERR_MSG_LEN, "error: unsupported operator or type. can not convert %s to boolean.", typeName_Res(val));
            generateAndAddErrMsg(errbuf, node, RE_UNSUPPORTED_OP_OR_TYPE, errmsg);
        }
        return res;
}
Res *smsi_lmsg(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
        writeToTmp("re.log", params[0]->text);
        Res *res = newIntRes(r, 0);
        return res;
}
Res *smsi_not(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    return newBoolRes(r, !RES_BOOL_VAL(params[0])?1:0);

}
Res *smsi_negate(Node **args, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    if(TYPE(args[0])==T_INT) {
        return newIntRes(r, -RES_INT_VAL(args[0]));
    } else {
        return newDoubleRes(r, -RES_DOUBLE_VAL(args[0]));
    }
}
Res *smsi_abs(Node **args, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    if(TYPE(args[0])==T_INT) {
        int val = RES_INT_VAL(args[0]);
        return newIntRes(r, (int)(val < 0?-val:val));
    } else {
        double val = RES_DOUBLE_VAL(args[0]);
        return newDoubleRes(r, (val < 0?-val:val));
    }
}
Res *smsi_exp(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **args = (Res **)params;
    double val = RES_DOUBLE_VAL(args[0]);
    return newDoubleRes(r, exp(val));
}
Res *smsi_log(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **args = (Res **)params;
    double val = RES_DOUBLE_VAL(args[0]);
    return newDoubleRes(r, log(val));

}
Res *smsi_floor(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **args = (Res **)params;
    double val = RES_DOUBLE_VAL(args[0]);
    return newDoubleRes(r, floor(val));

}
Res *smsi_ceiling(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **args = (Res **)params;
    double val = RES_DOUBLE_VAL(args[0]);
    return newDoubleRes(r, ceil(val));

}

Res *smsi_and(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **args = (Res **)params;
    return newBoolRes(r, RES_BOOL_VAL(args[0])&&RES_BOOL_VAL(args[1])?1:0);

}

Res *smsi_or(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **args = (Res **)params;
    return newBoolRes(r, RES_BOOL_VAL(args[0])||RES_BOOL_VAL(args[1])?1:0);
}

Res *smsi_add(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    if(TYPE(params[0])==T_INT) {
        return newIntRes(r, RES_INT_VAL(params[0])+RES_INT_VAL(params[1]));
    } else {
    	return newDoubleRes(r, RES_DOUBLE_VAL(params[0])+RES_DOUBLE_VAL(params[1]));
    }
}
Res *smsi_subtract(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    if(TYPE(params[0])==T_INT) {
        return newIntRes(r, RES_INT_VAL(params[0])-RES_INT_VAL(params[1]));
    } else {
    	return newDoubleRes(r, RES_DOUBLE_VAL(params[0])-RES_DOUBLE_VAL(params[1]));
    }
}
Res *smsi_multiply(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    if(TYPE(params[0])==T_INT) {
        return newIntRes(r, RES_INT_VAL(params[0])*RES_INT_VAL(params[1]));
    } else {
    	return newDoubleRes(r, RES_DOUBLE_VAL(params[0])*RES_DOUBLE_VAL(params[1]));
    }
}
Res *smsi_divide(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    if(TYPE(params[0])==T_INT) {
        if(RES_INT_VAL(params[1]) != 0) {
            return newDoubleRes(r, RES_INT_VAL(params[0])/(double)RES_INT_VAL(params[1]));
        }
    } else {
        if(RES_DOUBLE_VAL(params[1]) != 0) {
        	return newDoubleRes(r, RES_DOUBLE_VAL(params[0])/RES_DOUBLE_VAL(params[1]));
        }
    }
	generateAndAddErrMsg("division by zero.", node, RE_DIVISION_BY_ZERO, errmsg);
	return newErrorRes(r, RE_DIVISION_BY_ZERO);
}

Res *smsi_modulo(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	if(RES_INT_VAL(params[1]) != 0) {
		return newDoubleRes(r, RES_INT_VAL(params[0])%RES_INT_VAL(params[1]));
	}
	generateAndAddErrMsg("division by zero.", node, RE_DIVISION_BY_ZERO, errmsg);
	return newErrorRes(r, RE_DIVISION_BY_ZERO);
}

Res *smsi_power(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	return newDoubleRes(r, pow(RES_DOUBLE_VAL(params[0]), RES_DOUBLE_VAL(params[1])));
}
Res *smsi_root(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	if(RES_DOUBLE_VAL(params[1]) != 0) {
		return newDoubleRes(r, pow(RES_DOUBLE_VAL(params[0]), 1/RES_DOUBLE_VAL(params[1])));
	}
	generateAndAddErrMsg("division by zero.", node, RE_DIVISION_BY_ZERO, errmsg);
	return newErrorRes(r, RE_DIVISION_BY_ZERO);
}

Res *smsi_concat(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **args = (Res **)params;
/*    if(args[0]->value.s.len+args[1]->value.s.len>=1024) {
        //error
        addRErrorMsg(errmsg, STRING_OVERFLOW, "error: string too long.");
        return newErrorRes(r, STRING_OVERFLOW);
    } else {*/
    char *newbuf = (char *)malloc((RES_STRING_STR_LEN(args[0]) + RES_STRING_STR_LEN(args[1])+1)*sizeof(char));

    strcpy(newbuf, args[0]->text);
    strcpy(newbuf+RES_STRING_STR_LEN(args[0]), args[1]->text);

    Res *res = newStringRes(r, newbuf);
    free(newbuf);
    return res;
    /*}*/
}

Res *smsi_lt(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    switch(TYPE(params[0])) {
        case T_INT:
            return newBoolRes(r, RES_INT_VAL(params[0]) < RES_INT_VAL(params[1])?1:0);
            break;
        case T_DOUBLE:
			return newBoolRes(r, RES_DOUBLE_VAL(params[0]) < RES_DOUBLE_VAL(params[1])?1:0);
            break;
        case T_DATETIME:
			return newBoolRes(r, difftime(RES_TIME_VAL(params[0]), RES_TIME_VAL(params[1]))<0?1:0);
            break;
        case T_STRING:
			return newBoolRes(r, strcmp(params[0]->text, params[1]->text) <0?1:0);
            break;
        default:
            break;
    }
    char errbuf[ERR_MSG_LEN], type0[128], type1[128];
    snprintf(errbuf, ERR_MSG_LEN, "type error: comparing between %s and %s", typeToString(params[0]->exprType, NULL, type0, 128), typeToString(params[1]->exprType, NULL, type1, 128));
    generateAndAddErrMsg(errbuf, node, RE_DYNAMIC_TYPE_ERROR, errmsg);
    return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);

}
Res *smsi_le(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    switch(TYPE(params[0])) {
        case T_INT:
            return newBoolRes(r, RES_INT_VAL(params[0]) <= RES_INT_VAL(params[1])?1:0);
            break;
        case T_DOUBLE:
			return newBoolRes(r, RES_DOUBLE_VAL(params[0]) <= RES_DOUBLE_VAL(params[1])?1:0);
            break;
        case T_DATETIME:
			return newBoolRes(r, difftime(RES_TIME_VAL(params[0]), RES_TIME_VAL(params[1]))<=0?1:0);
            break;
        case T_STRING:
			return newBoolRes(r, strcmp(params[0]->text, params[1]->text) <=0?1:0);
            break;
        default:
            break;
    }
    char errbuf[ERR_MSG_LEN], type0[128], type1[128];
    snprintf(errbuf, ERR_MSG_LEN, "type error: comparing between %s and %s", typeToString(params[0]->exprType, NULL, type0, 128), typeToString(params[1]->exprType, NULL, type1, 128));
    generateAndAddErrMsg(errbuf, node, RE_DYNAMIC_TYPE_ERROR, errmsg);
    return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);

}
Res *smsi_gt(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    switch(TYPE(params[0])) {
        case T_INT:
            return newBoolRes(r, RES_INT_VAL(params[0]) > RES_INT_VAL(params[1])?1:0);
            break;
        case T_DOUBLE:
			return newBoolRes(r, RES_DOUBLE_VAL(params[0]) > RES_DOUBLE_VAL(params[1])?1:0);
            break;
        case T_DATETIME:
			return newBoolRes(r, difftime(RES_TIME_VAL(params[0]), RES_TIME_VAL(params[1]))>0?1:0);
            break;
        case T_STRING:
			return newBoolRes(r, strcmp(params[0]->text, params[1]->text) >0?1:0);
            break;
        default:
            break;
    }
    char errbuf[ERR_MSG_LEN], type0[128], type1[128];
    snprintf(errbuf, ERR_MSG_LEN, "type error: comparing between %s and %s", typeToString(params[0]->exprType, NULL, type0, 128), typeToString(params[1]->exprType, NULL, type1, 128));
    generateAndAddErrMsg(errbuf, node, RE_DYNAMIC_TYPE_ERROR, errmsg);
    return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);

}
Res *smsi_ge(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    switch(TYPE(params[0])) {
        case T_INT:
            return newBoolRes(r, RES_INT_VAL(params[0]) >= RES_INT_VAL(params[1])?1:0);
            break;
        case T_DOUBLE:
			return newBoolRes(r, RES_DOUBLE_VAL(params[0]) >= RES_DOUBLE_VAL(params[1])?1:0);
            break;
        case T_DATETIME:
			return newBoolRes(r, difftime(RES_TIME_VAL(params[0]), RES_TIME_VAL(params[1]))>=0?1:0);
            break;
        case T_STRING:
			return newBoolRes(r, strcmp(params[0]->text, params[1]->text) >=0?1:0);
            break;
        default:
            break;
    }
    char errbuf[ERR_MSG_LEN], type0[128], type1[128];
    snprintf(errbuf, ERR_MSG_LEN, "type error: comparing between %s and %s", typeToString(params[0]->exprType, NULL, type0, 128), typeToString(params[1]->exprType, NULL, type1, 128));
    generateAndAddErrMsg(errbuf, node, RE_DYNAMIC_TYPE_ERROR, errmsg);
    return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);
}
Res *smsi_eq(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    switch(TYPE(params[0])) {
		case T_BOOL:
			return newBoolRes(r, RES_BOOL_VAL(params[0]) == RES_BOOL_VAL(params[1])?1:0);
			break;
        case T_INT:
            return newBoolRes(r, RES_INT_VAL(params[0]) == RES_INT_VAL(params[1])?1:0);
            break;
        case T_DOUBLE:
			return newBoolRes(r, RES_DOUBLE_VAL(params[0]) == RES_DOUBLE_VAL(params[1])?1:0);
            break;
        case T_DATETIME:
			return newBoolRes(r, difftime(RES_TIME_VAL(params[0]), RES_TIME_VAL(params[1]))==0?1:0);
            break;
        case T_STRING:
			return newBoolRes(r, strcmp(params[0]->text, params[1]->text) ==0?1:0);
            break;
        default:
            break;
    }
    char errbuf[ERR_MSG_LEN], type0[128], type1[128];
    snprintf(errbuf, ERR_MSG_LEN, "type error: comparing between %s and %s", typeToString(params[0]->exprType, NULL, type0, 128), typeToString(params[1]->exprType, NULL, type1, 128));
    generateAndAddErrMsg(errbuf, node, RE_DYNAMIC_TYPE_ERROR, errmsg);
    return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);
}
Res *smsi_neq(Node **params, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    switch(TYPE(params[0])) {
		case T_BOOL:
			return newBoolRes(r, RES_BOOL_VAL(params[0]) != RES_BOOL_VAL(params[1])?1:0);
			break;
        case T_INT:
            return newBoolRes(r, RES_INT_VAL(params[0]) != RES_INT_VAL(params[1])?1:0);
            break;
        case T_DOUBLE:
			return newBoolRes(r, RES_DOUBLE_VAL(params[0]) != RES_DOUBLE_VAL(params[1])?1:0);
            break;
        case T_DATETIME:
			return newBoolRes(r, difftime(RES_TIME_VAL(params[0]), RES_TIME_VAL(params[1]))!=0?1:0);
            break;
        case T_STRING:
			return newBoolRes(r, strcmp(params[0]->text, params[1]->text) !=0?1:0);
            break;
        default:
            break;
    }
    char errbuf[ERR_MSG_LEN], type0[128], type1[128];
    snprintf(errbuf, ERR_MSG_LEN, "type error: comparing between %s and %s", typeToString(params[0]->exprType, NULL, type0, 128), typeToString(params[1]->exprType, NULL, type1, 128));
    generateAndAddErrMsg(errbuf, node, RE_DYNAMIC_TYPE_ERROR, errmsg);
    return newErrorRes(r, RE_DYNAMIC_TYPE_ERROR);
}
Res *smsi_like(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **params = paramsr;
	char *pattern;
	char *bufstr;
	pattern = params[1]->text;
	Res *res;
#if defined(_POSIX_VERSION) || defined(USE_BOOST)
	bufstr = strdup(params[0]->text);
	/* make the regexp match whole strings */
	char *buf2;
	buf2 = wildCardToRegex(pattern);
	regex_t regbuf;
	regcomp(&regbuf,buf2,REG_EXTENDED);
	res = newBoolRes(r, regexec(&regbuf,bufstr, 0,0,0)==0?1:0);
	regfree(&regbuf);
	free(buf2);
	free(bufstr);
#else
	res = newBoolRes(r, match(pattern, param[0]->text)==TRUE?1:0);
#endif
	return res;
}
Res *smsi_not_like(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *res = smsi_like(paramsr, n, node, rei, reiSaveFlag, env, errmsg, r);
    if(TYPE(res) != N_ERROR) {
    	return newBoolRes(r, RES_BOOL_VAL(res)?0:1);
    }
    return res;
}
Res *smsi_like_regex(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **params = paramsr;
	char *pattern;
	char *bufstr;
	pattern = params[1]->text;
	Res *res;
#if defined(_POSIX_VERSION) || defined(USE_BOOST)
	bufstr = strdup(params[0]->text);
	/* make the regexp match whole strings */
	pattern = matchWholeString(params[1]->text);
	regex_t regbuf;
	regcomp(&regbuf,pattern,REG_EXTENDED);
	res = newBoolRes(r, regexec(&regbuf,bufstr, 0,0,0)==0?1:0);
	regfree(&regbuf);
	free(bufstr);
	free(pattern);
#else
	res = newBoolRes(r, match(pattern, param[0]->text)==TRUE?1:0);
#endif
	return res;
}
Res *smsi_not_like_regex(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *res = smsi_like_regex(paramsr, n, node, rei, reiSaveFlag, env, errmsg, r);
    if(TYPE(res) != N_ERROR) {
    	return newBoolRes(r, RES_BOOL_VAL(res)?0:1);
    }
    return res;
}

Res *smsi_eval(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **params = (Res **)paramsr;
    /*printf("\neval: %s\n", params[0]->text); */
    return eval(params[0]->text, env, rei, reiSaveFlag, errmsg, r);
}

Res *smsi_evalrule(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res **params = (Res **)paramsr;
    /*printf("\neval: %s\n", params[0]->text); */
    return newIntRes(r, parseAndComputeRule(params[0]->text, env, rei, reiSaveFlag, errmsg, r));
}
/**
 * Run node and return the errorcode.
 * If the execution is successful, the returned errorcode is 0.
 */
Res *smsi_errorcode(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *res;
    switch(getNodeType(paramsr[0])) {
            case N_ACTIONS:
                res = evaluateActions((Node *)paramsr[0], (Node *)paramsr[1], 0, rei, reiSaveFlag,  env, errmsg, r);
                break;
            default:
                res = evaluateExpression3((Node *)paramsr[0], 0,1, rei,reiSaveFlag,env,errmsg,r);
                break;
        }
        switch(getNodeType(res)) {
            case N_ERROR:
                return newIntRes(r, RES_ERR_CODE(res));
            default:
                return newIntRes(r, 0);
        }
}

/**
 * Run node and return the errorcode.
 * If the execution is successful, the returned errorcode is 0.
 */
Res *smsi_errormsg(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    char *errbuf = (char *)malloc(ERR_MSG_LEN*1024*sizeof(char));
    Res *res;
    switch(getNodeType(paramsr[0])) {
            case N_ACTIONS:
                res = evaluateActions((Node *)paramsr[0], (Node *)paramsr[1], 0, rei, reiSaveFlag,  env, errmsg, r);
                paramsr[2] = newStringRes(r, errMsgToString(errmsg, errbuf, ERR_MSG_LEN*1024));
                break;
            default:
                res = evaluateExpression3((Node *)paramsr[0], 0,1,rei,reiSaveFlag,env,errmsg,r);
                paramsr[1] = newStringRes(r, errMsgToString(errmsg, errbuf, ERR_MSG_LEN*1024));
                break;
    }
    freeRErrorContent(errmsg);
    free(errbuf);
    switch(getNodeType(res)) {
        case N_ERROR:
            return newIntRes(r, RES_ERR_CODE(res));
        default:
            return newIntRes(r, 0);
    }
}

Res *smsi_delayExec(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{
  int i;
  char actionCall[MAX_ACTION_SIZE];
  char recoveryActionCall[MAX_ACTION_SIZE];
  char delayCondition[MAX_ACTION_SIZE];

  Res **params = (Res **)paramsr;

  rstrcpy(delayCondition, params[0]->text, MAX_ACTION_SIZE);
  rstrcpy(actionCall, params[1]->text, MAX_ACTION_SIZE);
  rstrcpy(recoveryActionCall, params[2]->text, MAX_ACTION_SIZE);

  msParamArray_t *tmp = rei->msParamArray;
  rei->msParamArray = newMsParamArray();

  int ret = convertEnvToMsParamArray(rei->msParamArray, env, errmsg, r);
  if(ret!=0) {
	  generateAndAddErrMsg("error converting Env to MsParamArray", node, ret, errmsg);
      return newErrorRes(r, ret);
  }

  i = _delayExec(actionCall, recoveryActionCall, delayCondition, rei);

  deleteMsParamArray(rei->msParamArray);
  rei->msParamArray = tmp;

  if(i < 0) {
      return newErrorRes(r, i);
  } else {
      return newIntRes(r, i);
  }
}

Res *smsi_remoteExec(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{
  int i;
  execMyRuleInp_t execMyRuleInp;
  msParamArray_t *outParamArray = NULL;
  char tmpStr[LONG_NAME_LEN];

  Res **params = (Res **)paramsr;

  memset (&execMyRuleInp, 0, sizeof (execMyRuleInp));
  execMyRuleInp.condInput.len=0;
  rstrcpy (execMyRuleInp.outParamDesc, ALL_MS_PARAM_KW, LONG_NAME_LEN);

  rstrcpy (tmpStr, params[0]->text, LONG_NAME_LEN);
  parseHostAddrStr (tmpStr, &execMyRuleInp.addr);

  if(strlen(params[3]->text) == 0) {
      snprintf(execMyRuleInp.myRule, META_STR_LEN, "remExec{%s}", params[2]->text);
  } else {
      snprintf(execMyRuleInp.myRule, META_STR_LEN, "remExec||%s|%s", params[2]->text, params[3]->text);
  }
  addKeyVal(&execMyRuleInp.condInput,"execCondition",params[1]->text);

  rei->msParamArray = newMsParamArray();
  int ret = convertEnvToMsParamArray(rei->msParamArray, env, errmsg, r);
  if(ret!=0) {
	  generateAndAddErrMsg("error converting Env to MsParamArray", node, ret, errmsg);
      return newErrorRes(r, ret);
  }
  execMyRuleInp.inpParamArray = rei->msParamArray;

  i = rsExecMyRule (rei->rsComm, &execMyRuleInp,  &outParamArray);

  if(outParamArray != NULL) {
	  rei->msParamArray = outParamArray;
  }
  updateMsParamArrayToEnvAndFreeNonIRODSType(rei->msParamArray, env, errmsg, r);
  deleteMsParamArray(rei->msParamArray);
  rei->msParamArray = NULL;
  if(i<0) {
      return newErrorRes(r, i);
  } else {
      return newIntRes(r, i);
  }
}
int writeStringNew(char *writeId, char *writeStr, Env *env, Region *r, ruleExecInfo_t *rei) {
  execCmdOut_t *myExecCmdOut;
  Res *execOutRes;
#ifndef DEBUG
  dataObjInp_t dataObjInp;
  openedDataObjInp_t openedDataObjInp;
  bytesBuf_t tmpBBuf;
  fileLseekOut_t *dataObjLseekOut = NULL;
  int fd,i;
#endif

  if (writeId != NULL && strcmp (writeId, "serverLog") == 0) {
    rodsLog (LOG_NOTICE, "writeString: inString = %s", writeStr);
    return 0;
  }
#ifndef DEBUG
  /* inserted by Raja Dec 2, 2011 */
    if (writeId != NULL && writeId[0] == '/') {
    /* writing to an existing iRODS file */

    if (rei == NULL || rei->rsComm == NULL) {
      rodsLog (LOG_ERROR, "_writeString: input rei or rsComm is NULL");
      return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    bzero (&dataObjInp, sizeof (dataObjInp));
    dataObjInp.openFlags = O_RDWR;
    snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s",writeId);
    fd = rsDataObjOpen (rei->rsComm, &dataObjInp);
    if (fd < 0) {
      rodsLog (LOG_ERROR, "_writeString: rsDataObjOpen failed. status = %d", fd);
      return(fd);
    }

    bzero(&openedDataObjInp, sizeof(openedDataObjInp));
    openedDataObjInp.l1descInx = fd;
    openedDataObjInp.offset = 0;
    openedDataObjInp.whence = SEEK_END;
    i = rsDataObjLseek (rei->rsComm, &openedDataObjInp, &dataObjLseekOut);
    if (i < 0) {
      rodsLog (LOG_ERROR, "_writeString: rsDataObjLseek failed. status = %d", i);
      return(i);
    }

    bzero(&openedDataObjInp, sizeof(openedDataObjInp));
    openedDataObjInp.l1descInx = fd;
    tmpBBuf.len = openedDataObjInp.len = strlen(writeStr) + 1;
    tmpBBuf.buf =  writeStr;
    i = rsDataObjWrite (rei->rsComm, &openedDataObjInp, &tmpBBuf);
    if (i < 0) {
      rodsLog (LOG_ERROR, "_writeString: rsDataObjWrite failed. status = %d", i);
      return(i);
    }

    bzero(&openedDataObjInp, sizeof(openedDataObjInp));
    openedDataObjInp.l1descInx = fd;
    i = rsDataObjClose (rei->rsComm, &openedDataObjInp);
    return(i);
  }

  /* inserted by Raja Dec 2, 2011 */
#endif


  if ((execOutRes = (Res *)lookupFromEnv(env, "ruleExecOut")) != NULL) {
    myExecCmdOut = (execCmdOut_t *)RES_UNINTER_STRUCT(execOutRes);
  } else {
      Env *global = env;
      while(global->previous != NULL) {
          global = global->previous;
      }
    myExecCmdOut = (execCmdOut_t *)malloc (sizeof (execCmdOut_t));
    memset (myExecCmdOut, 0, sizeof (execCmdOut_t));
    execOutRes = newUninterpretedRes(r, ExecCmdOut_MS_T, myExecCmdOut, NULL);
    insertIntoHashTable(global->current, "ruleExecOut", execOutRes);
  }

  if (!strcmp(writeId,"stdout"))
    appendToByteBufNew(&(myExecCmdOut->stdoutBuf),(char *) writeStr);
  else if (!strcmp(writeId,"stderr"))
    appendToByteBufNew(&(myExecCmdOut->stderrBuf),(char *) writeStr);
  return 0;
}

Res *smsi_writeLine(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
  char *inString = convertResToString(paramsr[1]);
  Res *where = (Res *)paramsr[0];
  char *whereId = where->text;

  if (strcmp (whereId, "serverLog") == 0) {
      rodsLog (LOG_NOTICE, "writeLine: inString = %s\n", inString);
      free(inString);
      return newIntRes(r, 0);
  }
  int i = writeStringNew(whereId, inString, env, r, rei);
#ifdef DEBUG
  printf("%s\n", inString);
#endif

  free(inString);
  if (i < 0) {
      return newErrorRes(r, i);
  }
  i = writeStringNew(whereId, "\n", env, r,rei);

  if (i < 0)
    return newErrorRes(r, i);
  else
    return newIntRes(r, i);
}
Res *smsi_writeString(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

  char *inString = convertResToString(paramsr[1]);
  Res *where = (Res *)paramsr[0];
  char *whereId = where->text;

  int i = writeStringNew(whereId, inString, env, r, rei);

  free(inString);
  if (i < 0)
    return newErrorRes(r, i);
  else
    return newIntRes(r, i);
}

Res *smsi_triml(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    /* if the length of delim is 0, strstr should return str */
    Res *strres = (Res *)paramsr[0];
    Res *delimres = (Res *)paramsr[1];

    char *str = strres->text;
    char *delim = delimres->text;

    char *p = strstr(str, delim);
    if(p!=NULL) {
        /* found */
        return newStringRes(r, p+strlen(delim));
    } else {
        /* not found return the original string */
        return strres;
    }



}
Res *smsi_strlen(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *strres = (Res *)paramsr[0];
    return newIntRes(r, strlen(strres->text));
}

Res *smsi_substr(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *strres = (Res *)paramsr[0];
    Res *startres = (Res *)paramsr[1];
    Res *finishres = (Res *)paramsr[2];

    char *buf = strdup(strres->text + RES_INT_VAL(startres));
    buf[RES_INT_VAL(finishres) - RES_INT_VAL(startres)] = '\0';

    Res *retres = newStringRes(r, buf);
    free(buf);
    return retres;
}

Res *smsi_split(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *strres = (Res *)paramsr[0];
    Res *delimres = (Res *)paramsr[1];

    char *buf = strdup(strres->text);
    int len = strlen(buf);
    int count = 0;
    int trim = 1;
    int i;
    for(i=0;i<len;i++) {
    	if(strchr(delimres->text, buf[i]) != NULL) {
    		i++;
    		while(i<len && strchr(delimres->text, buf[i]) != NULL) {
    			i++;
    		}
    		if(!trim && i<len) {
    			count++;
    		}
    	} else {
    		trim = 0;
    	}
    }
    if(!trim) {
    	count ++;
    }

    Res *coll = newCollRes(count, newSimpType(T_STRING, r), r);

    int j=0;
    trim = 1;
    char *bufStart = buf;
    for(i=0;i<len;i++) {
    	if(strchr(delimres->text, buf[i]) != NULL) {
    		buf[i] = '\0';
    		if(!trim) {
    			coll->subtrees[j++] = newStringRes(r, bufStart);
    		}
    		i++;
    		while(i<len && strchr(delimres->text, buf[i]) != NULL) {
    			i++;
    		}
    		bufStart = buf + i;
    	} else {
    		trim = 0;
    	}
    }
    if(j!=count) {
		coll->subtrees[j++] = newStringRes(r, bufStart);
    }

    return coll;

}

Res *smsi_undefined(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	return newUnspecifiedRes(r);

}
Res *smsi_trimr(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *strres = (Res *)paramsr[0];
    Res *delimres = (Res *)paramsr[1];

    char *str = strres->text;
    char *delim = delimres->text;

    if(strlen(delim)==0) {
        return strres;
    }

    char *p = strstr(str, delim);
    char *newp = NULL;
    while(p!=NULL) {
        newp = p;
        p = strstr(p+1, delim);
    }
    if(newp == NULL) {
        /* not found */
        return strres;
    } else {
        /* found set where newp points to to \0 */
        char temp = *newp;
        *newp = '\0';

        Res *res = newStringRes(r, str);
        /* restore */
        *newp = temp;
        return res;
    }

}

Res *smsi_msiAdmShowIRB(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	char buf[1024*16];
	int i;
	if(isComponentInitialized(ruleEngineConfig.extRuleSetStatus)) {
		for(i=0;i<ruleEngineConfig.extRuleSet->len;i++) {
			ruleToString(buf, 1024*16, ruleEngineConfig.extRuleSet->rules[i]);
#ifdef DEBUG_VERBOSE
			printf("%s", buf);
#endif
			writeStringNew("stdout", buf, env, r, rei);
		}
	}
	if(isComponentInitialized(ruleEngineConfig.appRuleSetStatus)) {
		for(i=0;i<ruleEngineConfig.appRuleSet->len;i++) {
			ruleToString(buf, 1024*16, ruleEngineConfig.appRuleSet->rules[i]);
#ifdef DEBUG_VERBOSE
			printf("%s", buf);
#endif
			writeStringNew("stdout", buf, env, r, rei);
		}
	}
	if(isComponentInitialized(ruleEngineConfig.coreRuleSetStatus)) {
		for(i=0;i<ruleEngineConfig.coreRuleSet->len;i++) {
			ruleToString(buf, 1024*16, ruleEngineConfig.coreRuleSet->rules[i]);
#ifdef DEBUG_VERBOSE
			printf("%s", buf);
#endif
			writeStringNew("stdout", buf, env, r, rei);
		}
	}
	return newIntRes(r, 0);
}
Res *smsi_msiAdmShowCoreRE(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	char buf[1024];
    char *conDir = getConfigDir ();
    char file2[1024];
    snprintf(file2, 1024, "%s/reConfigs/core.re",
	    conDir);
	FILE *f2 = fopen(file2, "r");

	while(!feof(f2) && ferror(f2) == 0) {
		if(fgets(buf, 1024, f2)!=NULL) {
#ifdef DEBUG_VERBOSE
			printf("%s", buf);
#endif
			writeStringNew("stdout", buf, env, r, rei);
		}
	}
	fclose(f2);
	return newIntRes(r, 0);
}
Res *smsi_msiAdmClearAppRuleStruct(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

	  int i;
#ifndef DEBUG
	  if ((i = isUserPrivileged(rei->rsComm)) != 0)
		  return newErrorRes(r, i);
	  i = unlinkFuncDescIndex();
	  if (i < 0)
		  return newErrorRes(r, i);
	  i = clearResources(RESC_APP_RULE_SET | RESC_APP_FUNC_DESC_INDEX);
	  if (i < 0)
		  return newErrorRes(r, i);
	  i = generateFunctionDescriptionTables();
	  if (i < 0)
		  return newErrorRes(r, i);
	  i = clearDVarStruct(&appRuleVarDef);
	  if (i < 0)
		  return newErrorRes(r, i);
	  i = clearFuncMapStruct(&appRuleFuncMapDef);
	  return newIntRes(r, i);
#else
	  i = unlinkFuncDescIndex();
	  i = clearResources(RESC_APP_RULE_SET | RESC_APP_FUNC_DESC_INDEX);
	  i = generateFunctionDescriptionTables();
	  return newIntRes(r, 0);
#endif

}
Res *smsi_msiAdmAddAppRuleStruct(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	  int i;

#ifndef DEBUG
	  if ((i = isUserPrivileged(rei->rsComm)) != 0)
		  return newErrorRes(r, i);

	  if (strlen(paramsr[0]->text) > 0) {
	    i = loadRuleFromCacheOrFile(RULE_ENGINE_TRY_CACHE, paramsr[0]->text, &appRuleStrct);
	    if (i < 0)
	  	  return newErrorRes(r, i);
	  }
	  if (strlen(paramsr[1]->text) > 0) {
	    i = readDVarStructFromFile(paramsr[1]->text, &appRuleVarDef);
	    if (i < 0)
	  	  return newErrorRes(r, i);
	  }
	  if (strlen(paramsr[2]->text) > 0) {
	    i = readFuncMapStructFromFile(paramsr[2]->text, &appRuleFuncMapDef);
	    if (i < 0)
	  	  return newErrorRes(r, i);
	  }
	  return newIntRes(r, 0);
#else
	  i = loadRuleFromCacheOrFile(RULE_ENGINE_TRY_CACHE, paramsr[0]->text, &appRuleStrct);
	  if(i<0) {
		  return newErrorRes(r, i);
	  } else {
		  return newIntRes(r, i);
	  }

#endif

}

Res *smsi_msiAdmAppendToTopOfCoreRE(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
#ifndef DEBUG
	  int i;
	  if ((i = isUserPrivileged(rei->rsComm)) != 0)
	    return newErrorRes(r, i);
#endif
	  char *conDir = getConfigDir ();
	  char file1[1024];
	  char file2[1024];
	  char file3[1024];
	  snprintf(file1, 1024, "%s/reConfigs/%s.re",
		   conDir, paramsr[0]->text);
	  snprintf(file2, 1024, "%s/reConfigs/core.re",
		   conDir);
	  snprintf(file3, 1024, "%s/reConfigs/core.tmp", conDir);
	  int errcode;
	  if((errcode = fileConcatenate(file1, file2, file3))!=0 || (errcode = remove(file2))!=0 || (errcode = rename(file3, file2))!=0) {
		  generateAndAddErrMsg("error appending to top of core.re", node, errcode, errmsg);
		  return newErrorRes(r, errcode);
	  }
	  return newIntRes(r, 0);

}
Res *smsi_msiAdmChangeCoreRE(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
#ifndef DEBUG
	  int i;
	  if ((i = isUserPrivileged(rei->rsComm)) != 0)
	    return newErrorRes(r, i);
#endif
	  char *conDir = getConfigDir ();
	  char file1[1024];
	  char file2[1024];
	  snprintf(file1, 1024, "%s/reConfigs/%s.re",
		   conDir, paramsr[0]->text);
	  snprintf(file2, 1024, "%s/reConfigs/core.re",
		   conDir);
	  int errcode;
	  if((errcode = fileConcatenate(file1, NULL, file2))!=0) {
		  generateAndAddErrMsg("error changing core.re", node, errcode, errmsg);
		  return newErrorRes(r, errcode);
	  }
	  return newIntRes(r, 0);

}

int insertRulesIntoDBNew(char * baseName, RuleSet *ruleSet, ruleExecInfo_t *rei);

Res * smsi_msiAdmInsertRulesFromStructIntoDB(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{

  /* ruleStruct_t *coreRuleStruct; */
  int i;
#ifndef DEBUG
  if ((i = isUserPrivileged(rei->rsComm)) != 0) {
	  generateAndAddErrMsg("error inserting rules into database", node, i, errmsg);
      return newErrorRes(r, i);
  }
#endif

  if (paramsr[0]->text == NULL ||
      strlen(paramsr[0]->text) == 0 ) {
	  generateAndAddErrMsg("empty input struct", node, PARAOPR_EMPTY_IN_STRUCT_ERR, errmsg);
	  return newErrorRes(r, PARAOPR_EMPTY_IN_STRUCT_ERR);
  }

  RuleSet *rs = (RuleSet *)RES_UNINTER_STRUCT(paramsr[1]);
  i = insertRulesIntoDBNew(paramsr[0]->text, rs, rei);
  if(i<0) {
	  generateAndAddErrMsg("error inserting rules into database", node, PARAOPR_EMPTY_IN_STRUCT_ERR, errmsg);
	  return newErrorRes(r, i);
  } else {
	  return newIntRes(r, i);
  }

}
Res * smsi_msiAdmReadRulesFromFileIntoStruct(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{

  int i;
  RuleSet *ruleSet;

#ifndef DEBUG
  if ((i = isUserPrivileged(rei->rsComm)) != 0) {
	  generateAndAddErrMsg("error inserting rules into database", node, i, errmsg);
      return newErrorRes(r, i);
  }
#endif

/*  RE_TEST_MACRO ("Loopback on msiAdmReadRulesFromFileIntoStruct");*/


  if (paramsr[0]->text == NULL ||
      strlen(paramsr[0]->text) == 0 ) {
	  generateAndAddErrMsg("empty input struct", node, PARAOPR_EMPTY_IN_STRUCT_ERR, errmsg);
	  return newErrorRes(r, PARAOPR_EMPTY_IN_STRUCT_ERR);
  }
  Region *rsr = make_region(0, NULL);

  ruleSet = newRuleSet(rsr);

  Env *rsEnv = newEnv(newHashTable2(100, rsr), NULL, NULL, rsr);

  int errloc = 0;
  i = readRuleSetFromFile(paramsr[0]->text, ruleSet, rsEnv, &errloc, errmsg, rsr);
  /* deleteEnv(rsEnv, 3); */
  if (i != 0) {
	  region_free(rsr);
	  generateAndAddErrMsg("error reading rules from file.", node, i, errmsg);
	  return newErrorRes(r, i);
  }

  Hashtable *objectMap = newHashTable2(100, rsr);
  RuleSet *buf = memCpRuleSet(ruleSet, objectMap);
  if(buf == NULL) {
	  return newErrorRes(r, RE_OUT_OF_MEMORY);
  }

  paramsr[1] = newUninterpretedRes(r, RuleSet_MS_T, (void *) buf, NULL);

  region_free(rsr);
  return newIntRes(r, 0);
}

Res *smsi_msiAdmWriteRulesFromStructIntoFile(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
	int i;
	  FILE *file;
	  char fileName[MAX_NAME_LEN];
	  char *configDir;

	  char *inFileName = paramsr[0]->text;
	  if (inFileName[0] == '/' || inFileName[0] == '\\' ||
	      inFileName[1] == ':') {
	      snprintf (fileName,MAX_NAME_LEN, "%s",inFileName);
	  }
	  else {
	      configDir = getConfigDir ();
	      snprintf (fileName,MAX_NAME_LEN, "%s/reConfigs/%s.re", configDir,inFileName);
	  }


	  file = fopen(fileName, "w");
	  if (file == NULL) {
	    rodsLog(LOG_NOTICE,
		    "msiAdmWriteRulesFromStructToFile could not open rules file %s for writing\n",
		    fileName);
	    generateAndAddErrMsg("error opening file for writing.", node, FILE_OPEN_ERR, errmsg);
	    return newErrorRes(r, FILE_OPEN_ERR);
	  }

    RuleSet *ruleSet = (RuleSet *) RES_UNINTER_STRUCT(paramsr[1]);
	char buf[1024*16];
		for(i=0;i<ruleSet->len;i++) {
			ruleToString(buf, 1024*16, ruleSet->rules[i]);
			fprintf(file, "%s", buf);
		}
	  fclose (file);
	return newIntRes(r, 0);
}




int readRuleSetFromDB(char *ruleBaseName, char *versionStr, RuleSet *rs, ruleExecInfo_t *rei, rError_t *errmsg, Region *r);

Res * smsi_msiAdmRetrieveRulesFromDBIntoStruct(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{

  int i;
  RuleSet *ruleSet;
/*
#ifndef DEBUG
  if ((i = isUserPrivileged(rei->rsComm)) != 0) {
	  generateAndAddErrMsg("error reading rules from database", node, i, errmsg);
      return newIntRes(r, i);
  }
#endif
*/
  /* RE_TEST_MACRO ("Loopback on msiGetRulesFromDBIntoStruct"); */

  if (paramsr[0]->text == NULL ||
      strlen(paramsr[0]->text) == 0 ) {
	  generateAndAddErrMsg("empty input struct", node, PARAOPR_EMPTY_IN_STRUCT_ERR, errmsg);
	  return newErrorRes(r, PARAOPR_EMPTY_IN_STRUCT_ERR);
  }
  if (paramsr[1]->text == NULL ||
      strlen(paramsr[1]->text) == 0 ) {
	  generateAndAddErrMsg("empty input struct", node, PARAOPR_EMPTY_IN_STRUCT_ERR, errmsg);
	  return newErrorRes(r, PARAOPR_EMPTY_IN_STRUCT_ERR);
  }
  Region *rsr = make_region(0, NULL);

  ruleSet = newRuleSet(rsr);

  /* Env *rsEnv = newEnv(newHashTable2(100, rsr), NULL, NULL, rsr); */

  i = readRuleSetFromDB(paramsr[0]->text, paramsr[1]->text, ruleSet, rei, errmsg, rsr);
  /*deleteEnv(rsEnv, 3); */
  if (i != 0) {
	  region_free(rsr);
	  generateAndAddErrMsg("error retrieving rules from database.", node, i, errmsg);
	  return newErrorRes(r, i);
  }

  Hashtable *objectMap = newHashTable2(100, rsr);
  RuleSet *buf = memCpRuleSet(ruleSet, objectMap);
  if(buf == NULL) {
	  return newErrorRes(r, RE_OUT_OF_MEMORY);
  }

  paramsr[2] = newUninterpretedRes(r, RuleSet_MS_T, (void *) buf, NULL);

  region_free(rsr);
  return newIntRes(r, 0);
}


Res *smsi_getstdout(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *res = (Res *)lookupFromEnv(env, "ruleExecOut");
    if(res == NULL) {
    	generateAndAddErrMsg("ruleExecOut not set", node, RE_RUNTIME_ERROR, errmsg);
    	return newErrorRes(r, RE_RUNTIME_ERROR);
    }

    execCmdOut_t *out = (execCmdOut_t *)RES_UNINTER_STRUCT(res);
    int start = strlen((char *)out->stdoutBuf.buf);
    Res *ret = smsi_do(paramsr, 1, node, rei, reiSaveFlag, env, errmsg, r);
    /* int fin = strlen((char *)out->stdoutBuf.buf); */
    paramsr[1] = newStringRes(r, ((char *)out->stdoutBuf.buf + start));
    return ret;
}

Res *smsi_getstderr(Node **paramsr, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *res = (Res *)lookupFromEnv(env, "ruleExecOut");
    if(res == NULL) {
    	generateAndAddErrMsg("ruleExecOut not set", node, RE_RUNTIME_ERROR, errmsg);
    	return newErrorRes(r, RE_RUNTIME_ERROR);
    }

    execCmdOut_t *out = (execCmdOut_t *)RES_UNINTER_STRUCT(res);
    int start = strlen((char *)out->stderrBuf.buf);
    Res *ret = smsi_do(paramsr, 1, node, rei, reiSaveFlag, env, errmsg, r);
    paramsr[1] = newStringRes(r, ((char *)out->stderrBuf.buf + start));
    return ret;
}

Res *smsi_assignStr(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {
    Res *val = evaluateExpression3((Node *)subtrees[1], 0, 1, rei, reiSaveFlag,  env, errmsg,r);
    if(getNodeType(val) == N_ERROR) {
        return val;
    }
    if(TYPE(val) == T_INT || TYPE(val) == T_DOUBLE || TYPE(val) == T_BOOL) {
    	CASCADE_N_ERROR(val = smsi_str(&val, 1, node, rei, reiSaveFlag, env, errmsg, r));
    }
    Res *ret = matchPattern(subtrees[0], val, env, rei, reiSaveFlag, errmsg, r);

    return ret;

}

extern int GlobalAllRuleExecFlag;
Res *smsi_applyAllRules(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{
  Res *res;
  Node *action;
  int reiSaveFlag2;
  int allRuleExecFlag;

  action = subtrees[0];
  reiSaveFlag2 = RES_INT_VAL(subtrees[1]);
  allRuleExecFlag = RES_INT_VAL(subtrees[2]);

  res = evaluateExpression3(action, allRuleExecFlag == 1 ? 2 : 1, 1, rei, reiSaveFlag2, env, errmsg, r);

  return res;

}

int
parseResForCollInp (Node *inpParam, collInp_t *collInpCache,
collInp_t **outCollInp, int outputToCache)
{
    *outCollInp = NULL;

    if (inpParam == NULL) {
        rodsLog (LOG_ERROR,
          "parseMspForCollInp: input inpParam is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (TYPE(inpParam) == T_STRING) {
        /* str input */
        if (collInpCache == NULL) {
            collInpCache = (collInp_t *)malloc (sizeof (collInp_t));
        }
        memset (collInpCache, 0, sizeof (collInp_t));
        *outCollInp = collInpCache;
        if (strcmp (inpParam->text, "null") != 0) {
            rstrcpy(collInpCache->collName, (char*)inpParam->text, MAX_NAME_LEN);
        }
        return (0);
    } else if (TYPE(inpParam) == T_IRODS && strcmp(RES_IRODS_TYPE(inpParam), CollInp_MS_T) == 0) {
    	if (outputToCache == 1) {
    		collInp_t *tmpCollInp;
    		tmpCollInp = (collInp_t *) RES_UNINTER_STRUCT(inpParam);
            if (collInpCache == NULL) {
                collInpCache = (collInp_t *)malloc (sizeof (collInp_t));
            }
            *collInpCache = *tmpCollInp;
	    /* zero out the condition of the original because it has been
	     * moved */
            memset (&tmpCollInp->condInput, 0, sizeof (keyValPair_t));
            *outCollInp = collInpCache;
    	} else {
            *outCollInp = (collInp_t *) RES_UNINTER_STRUCT(inpParam);
    	}
    	return (0);
    } else {
    	char buf[ERR_MSG_LEN];
    	Hashtable *varTypes = newHashTable(10);
    	typeToString(inpParam->exprType, varTypes, buf, ERR_MSG_LEN);
    	deleteHashTable(varTypes, NULL);
        rodsLog (LOG_ERROR,
          "parseMspForCollInp: Unsupported input Param1 type %s",
          buf);
        return (USER_PARAM_TYPE_ERR);
    }
}

int rsOpenCollection (rsComm_t *rsComm, collInp_t *openCollInp);
int rsCloseCollection (rsComm_t *rsComm, int *handleInxInp);
int rsReadCollection (rsComm_t *rsComm, int *handleInxInp, collEnt_t **collEnt);

Res *smsiCollectionSpider(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r)
{
	collInp_t collInpCache, *collInp;		/* input for rsOpenCollection */
	collEnt_t *collEnt;						/* input for rsReadCollection */
	int handleInx;							/* collection handler */
	dataObjInp_t *dataObjInp;				/* will contain pathnames for each object (one at a time) */

	/* Sanity test */
	if (rei == NULL || rei->rsComm == NULL) {
    	generateAndAddErrMsg("msiCollectionSpider: input rei or rsComm is NULL.", node, SYS_INTERNAL_NULL_INPUT_ERR, errmsg);
    	return newErrorRes(r, SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* Parse collection input */
	rei->status = parseResForCollInp (subtrees[1], &collInpCache, &collInp, 0);
	if (rei->status < 0) {
		char buf[ERR_MSG_LEN];
		snprintf(buf, ERR_MSG_LEN, "msiIsCollectionSpider: input collection error. status = %d", rei->status);
		generateAndAddErrMsg(buf, node, rei->status, errmsg);
		return newErrorRes(r, rei->status);
	}

	/* Check if "objects" input has proper form */
    if(getNodeType(subtrees[0])!=TK_VAR) {
		char buf[ERR_MSG_LEN];
		snprintf(buf, ERR_MSG_LEN, "msiIsCollectionSpider: input objects error. status = %d", rei->status);
    	generateAndAddErrMsg(buf, node, rei->status, errmsg);
		return newErrorRes(r, rei->status);
    }
    char* varname = subtrees[0]->text;

    /* Allocate memory for dataObjInp. Needs to be persistent since will be freed later along with other msParams */
    dataObjInp = (dataObjInp_t *)malloc(sizeof(dataObjInp_t));

	/* Open collection in recursive mode */
	collInp->flags = RECUR_QUERY_FG;
	handleInx = rsOpenCollection (rei->rsComm, collInp);
	if (handleInx < 0)
	{
		char buf[ERR_MSG_LEN];
		snprintf(buf, ERR_MSG_LEN, "msiCollectionSpider: rsOpenCollection of %s error. status = %d", collInp->collName, handleInx);
    	generateAndAddErrMsg(buf, node, handleInx, errmsg);
		return newErrorRes(r, handleInx);
	}

	GC_BEGIN
	/* save the old value of variable with name varname in the current env only */
	Res *oldVal = (Res *) lookupFromHashTable(env->current, varname);

    /* Read our collection one object at a time */
	while ((rei->status = rsReadCollection (rei->rsComm, &handleInx, &collEnt)) >= 0)
	{
		GC_ON(env);
		/* Skip collection entries */
		if (collEnt != NULL) {
			if(collEnt->objType == DATA_OBJ_T)
			{
				/* Write our current object's path in dataObjInp, where the inOutStruct in 'objects' points to */

				memset(dataObjInp, 0, sizeof(dataObjInp_t));
				snprintf(dataObjInp->objPath, MAX_NAME_LEN, "%s/%s", collEnt->collName, collEnt->dataName);

				/* Free collEnt only. Content will be freed by rsCloseCollection() */
				free(collEnt);

				/* Set var with name varname in the current environment */
				updateInEnv(env, varname, newUninterpretedRes(GC_REGION, DataObjInp_MS_T, (void *) dataObjInp, NULL));

				/* Run actionStr on our object */
				Res *ret = evaluateActions(subtrees[2], subtrees[3], 0, rei, reiSaveFlag, env, errmsg, GC_REGION);
				if (TYPE(ret) == T_ERROR)
				{
					/* If an error occurs, log incident but keep going */
					char buf[ERR_MSG_LEN];
					snprintf(buf, ERR_MSG_LEN, "msiCollectionSpider: execMyRule error. status = %d", RES_ERR_CODE(ret));
					generateAndAddErrMsg(buf, node, RES_ERR_CODE(ret), errmsg);
				}
				else if (TYPE(ret) == T_BREAK) {
					break;
				}

			} else {
				/* Free collEnt only. Content will be freed by rsCloseCollection() */
				free(collEnt);
			}
		}

	}

	if(oldVal == NULL) {
		deleteFromHashTable(env->current, varname);
	} else {
		updateInEnv(env, varname, oldVal);
	}
	cpEnv2(env, GC_REGION, r);
	GC_END

	free(dataObjInp);

	/* Close collection */
	rei->status = rsCloseCollection (rei->rsComm, &handleInx);

	/* Return operation status */
	if(rei->status < 0) {
		return newErrorRes(r, rei->status);
	} else {
		return newIntRes(r, rei->status);
	}

}


/* utilities */
int fileConcatenate(char *file1, char *file2, char *file3) {
	char buf[1024];
	FILE *f1 = fopen(file1, "r");
	if(f1 == NULL) {
		return USER_FILE_DOES_NOT_EXIST;
	}
	FILE *f2;
	if(file2 == NULL) {
		f2 = NULL;
	} else {
		f2 = fopen(file2, "r");
		if(f2 == NULL) {
			fclose(f1);
			return USER_FILE_DOES_NOT_EXIST;
		}
	}
	FILE *f3 = fopen(file3, "w");

	size_t len;
	int error = 0;
	while(!feof(f1) && ferror(f1) == 0) {
		len = fread(buf, 1, 1024, f1);
		fwrite(buf, 1, len, f3);
	}
	error = ferror(f1);
	if(error==0 && f2!=NULL) {
		while(!feof(f2) && ferror(f2) == 0) {
			len = fread(buf, 1, 1024, f2);
			fwrite(buf, 1, len, f3);
		}
		error = ferror(f2);
	}

	fclose(f1);
	if(f2!=NULL) {
		fclose(f2);
	}
	fclose(f3);
	return error;
}

Res* eval(char *expr, Env *env, ruleExecInfo_t *rei, int saveREI, rError_t *errmsg, Region *r) {
    Res *res = parseAndComputeExpression(expr, env, rei, saveREI, errmsg, r);
    return res;
}

/*FunctionDesc *getFuncDescFromChain(int n, FunctionDesc *fDesc) {
            ExprType *fTypeCopy = fDesc->type;

            while((T_FUNC_VARARG(fTypeCopy) == ONCE && n != T_FUNC_ARITY(fTypeCopy))
                    ||(T_FUNC_VARARG(fTypeCopy) == STAR && n < T_FUNC_ARITY(fTypeCopy) - 1)
                    ||(T_FUNC_VARARG(fTypeCopy) == PLUS && n < T_FUNC_ARITY(fTypeCopy))) {
                if(fDesc->next == NULL) {
                    return NULL;
                }
                fDesc = fDesc->next;
                fTypeCopy = fDesc->type;
            }
            return fDesc;
}

*/

Node *construct(char *fn, Node **args, int argc, Node *constype, Region *r) {
    Node *res = newRes(r);
    res->text = cpStringExt(fn, r);
    res->degree = argc;
    res->subtrees = (Node **)region_alloc(r, sizeof(Node *)*argc);
    memcpy(res->subtrees, args, sizeof(Node *)*argc);
    res->exprType = constype;
    return res;
}

Node *deconstruct(char *fn, Node **args, int argc, int proj, rError_t*errmsg, Region *r) {
    Node *res = args[0]->subtrees[proj];
    return res;
}

Res *smsi_segfault(Node **subtrees, int n, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r) {

	char *a = NULL;
	putchar(*a);
        return NULL;
}


void getSystemFunctions(Hashtable *ft, Region *r) {
    insertIntoHashTable(ft, "do", newFunctionFD("e ?->?", smsi_do, r));
    insertIntoHashTable(ft, "eval", newFunctionFD("string->?", smsi_eval, r));
    insertIntoHashTable(ft, "evalrule", newFunctionFD("string->?", smsi_evalrule, r));
    insertIntoHashTable(ft, "applyAllRules", newFunctionFD("e ? * f 0{integer string } => integer * f 1{integer string} => integer->?", smsi_applyAllRules, r));
    insertIntoHashTable(ft, "errorcodea", newFunctionFD("a ? * a ?->integer", smsi_errorcode, r));
    insertIntoHashTable(ft, "errorcode", newFunctionFD("e ?->integer", smsi_errorcode, r));
    insertIntoHashTable(ft, "errormsga", newFunctionFD("a ? * a ? * o string->integer", smsi_errormsg, r));
    insertIntoHashTable(ft, "errormsg", newFunctionFD("e ? * o string->integer", smsi_errormsg, r));
    insertIntoHashTable(ft, "getstdout", newFunctionFD("e ? * o string ->integer", smsi_getstdout, r));
    insertIntoHashTable(ft, "getstderr", newFunctionFD("e ? * o string ->integer", smsi_getstderr, r));
    insertIntoHashTable(ft, "let", newFunctionFD("e 0 * e f 0 * e 1->1", smsi_letExec, r));
    insertIntoHashTable(ft, "match", newFunctionFD("e 0 * e (0 * 1)*->1", smsi_matchExec, r));
    insertIntoHashTable(ft, "if2", newFunctionFD("e boolean * e 0 * e 0 * e ? * e ?->0", smsi_if2Exec, r));
    insertIntoHashTable(ft, "if", newFunctionFD("e boolean * a ? * a ? * a ? * a ?->?", smsi_ifExec, r));
    insertIntoHashTable(ft, "for", newFunctionFD("e ? * e boolean * e ? * a ? * a ?->?",smsi_forExec, r));
    insertIntoHashTable(ft, "while", newFunctionFD("e boolean * a ? * a ?->?",smsi_whileExec, r));
    insertIntoHashTable(ft, "foreach", newFunctionFD("e list 0 * a ? * a ?->?", smsi_forEachExec, r));
    insertIntoHashTable(ft, "foreach2", newFunctionFD("forall X, e X * e list X * a ? * a ?->?", smsi_forEach2Exec, r));
    insertIntoHashTable(ft, "break", newFunctionFD("->integer", smsi_break, r));
    insertIntoHashTable(ft, "succeed", newFunctionFD("->integer", smsi_succeed, r));
    insertIntoHashTable(ft, "fail", newFunctionFD("integer ?->integer", smsi_fail, r));
    insertIntoHashTable(ft, "assign", newFunctionFD("e 0 * e f 0->integer", smsi_assign, r));
    insertIntoHashTable(ft, "lmsg", newFunctionFD("string->integer", smsi_lmsg, r));
    insertIntoHashTable(ft, "listvars", newFunctionFD("->string", smsi_listvars, r));
    insertIntoHashTable(ft, "listcorerules", newFunctionFD("->list string", smsi_listcorerules, r));
    insertIntoHashTable(ft, "listapprules", newFunctionFD("->list string", smsi_listapprules, r));
    insertIntoHashTable(ft, "listextrules", newFunctionFD("->list string", smsi_listextrules, r));
    /*insertIntoHashTable(ft, "true", newFunctionFD("boolean", smsi_true, r));
    insertIntoHashTable(ft, "false", newFunctionFD("boolean", smsi_false, r));*/
    insertIntoHashTable(ft, "time", newFunctionFD("->time", smsi_time, r));
    insertIntoHashTable(ft, "timestr", newFunctionFD("time->string", smsi_timestr, r));
    insertIntoHashTable(ft, "str", newFunctionFD("?->string", smsi_str, r));
    insertIntoHashTable(ft, "datetime", newFunctionFD("string->time", smsi_datetime, r));
    insertIntoHashTable(ft, "timestrf", newFunctionFD("time * string->string", smsi_timestr, r));
    insertIntoHashTable(ft, "datetimef", newFunctionFD("string * string->time", smsi_datetime, r));
    insertIntoHashTable(ft, "double", newFunctionFD("f 0{string double time}->double", smsi_double, r));
    insertIntoHashTable(ft, "int", newFunctionFD("0{integer string double}->integer", smsi_int, r));
    insertIntoHashTable(ft, "bool", newFunctionFD("0{boolean integer string double}->boolean", smsi_bool, r));
    insertIntoHashTable(ft, "list", newFunctionFD("forall X, X*->list X", smsi_list, r));
    /*insertIntoHashTable(ft, "tuple",
            newFunctionDescChain(newConstructorDesc("-> <>", r),
            newFunctionDescChain(newConstructorDesc("A-> <A>", r),
            newFunctionDescChain(newConstructorDesc("A * B-> <A * B>", r),
            newFunctionDescChain(newConstructorDesc("A * B * C-> <A * B * C>", r),
            newFunctionDescChain(newConstructorDesc("A * B * C * D-> <A * B * C * D>", r),
            newFunctionDescChain(newConstructorDesc("A * B * C * D * E-> <A * B * C * D * E>", r),
            newFunctionDescChain(newConstructorDesc("A * B * C * D * E * F * G-> <A * B * C * D * E * F * G>", r),
            newFunctionDescChain(newConstructorDesc("A * B * C * D * E * F * G * H-> <A * B * C * D * E * F * G * H>", r),
            newFunctionDescChain(newConstructorDesc("A * B * C * D * E * F * G * H * I-> <A * B * C * D * E * F * G * H * I>", r),
            newConstructorDesc("A * B * C * D * E * F * G * H * I * J-> <A * B * C * D * E * F * G * H * I * J>", r)
            ))))))))));*/
    insertIntoHashTable(ft, "elem", newFunctionFD("forall X, list X * integer->X", smsi_elem, r));
    insertIntoHashTable(ft, "setelem", newFunctionFD("forall X, list X * integer * X->list X", smsi_setelem, r));
    insertIntoHashTable(ft, "hd", newFunctionFD("forall X, list X->X", smsi_hd, r));
    insertIntoHashTable(ft, "tl", newFunctionFD("forall X, list X->list X", smsi_tl, r));
    insertIntoHashTable(ft, "cons", newFunctionFD("forall X, X * list X->list X", smsi_cons, r));
    insertIntoHashTable(ft, "size", newFunctionFD("forall X, list X->integer", smsi_size, r));
    insertIntoHashTable(ft, "type", newFunctionFD("forall X, X->string",smsi_type, r));
    insertIntoHashTable(ft, "arity", newFunctionFD("string->integer",smsi_arity, r));
    insertIntoHashTable(ft, "+", newFunctionFD("forall X in {integer double}, f X * f X->X",smsi_add, r));
    insertIntoHashTable(ft, "++", newFunctionFD("f string * f string->string",smsi_concat, r));
    insertIntoHashTable(ft, "-", newFunctionFD("forall X in {integer double}, f X * f X->X",smsi_subtract, r));
    insertIntoHashTable(ft, "neg", newFunctionFD("forall X in {integer double}, X-> X", smsi_negate, r));
    insertIntoHashTable(ft, "*", newFunctionFD("forall X in {integer double}, f X * f X->X",smsi_multiply, r));
    insertIntoHashTable(ft, "/", newFunctionFD("forall X in {integer double}, f X * f X->?",smsi_divide, r));
    insertIntoHashTable(ft, "%", newFunctionFD("integer * integer->integer",smsi_modulo, r));
    insertIntoHashTable(ft, "^", newFunctionFD("f double * f double->double",smsi_power, r));
    insertIntoHashTable(ft, "^^", newFunctionFD("f double * f double->double",smsi_root, r));
    insertIntoHashTable(ft, "log", newFunctionFD("f double->double",smsi_log, r));
    insertIntoHashTable(ft, "exp", newFunctionFD("f double->double",smsi_exp, r));
    insertIntoHashTable(ft, "!", newFunctionFD("boolean->boolean",smsi_not, r));
    insertIntoHashTable(ft, "&&", newFunctionFD("boolean * boolean->boolean",smsi_and, r));
    insertIntoHashTable(ft, "||", newFunctionFD("boolean * boolean->boolean",smsi_or, r));
    insertIntoHashTable(ft, "%%", newFunctionFD("boolean * boolean->boolean",smsi_or, r));
    insertIntoHashTable(ft, "==", newFunctionFD("forall X in {integer double boolean string time}, f X * f X->boolean",smsi_eq, r));
    insertIntoHashTable(ft, "!=", newFunctionFD("forall X in {integer double boolean string time}, f X * f X->boolean",smsi_neq, r));
    insertIntoHashTable(ft, ">", newFunctionFD("forall X in {integer double string time}, f X * f X->boolean", smsi_gt, r));
    insertIntoHashTable(ft, "<", newFunctionFD("forall X in {integer double string time}, f X * f X->boolean", smsi_lt, r));
    insertIntoHashTable(ft, ">=", newFunctionFD("forall X in {integer double string time}, f X * f X->boolean", smsi_ge, r));
    insertIntoHashTable(ft, "<=", newFunctionFD("forall X in {integer double string time}, f X * f X->boolean", smsi_le, r));
    insertIntoHashTable(ft, "floor", newFunctionFD("f double->double", smsi_floor, r));
    insertIntoHashTable(ft, "ceiling", newFunctionFD("f double->double", smsi_ceiling, r));
    insertIntoHashTable(ft, "abs", newFunctionFD("f double->double", smsi_abs, r));
    insertIntoHashTable(ft, "max", newFunctionFD("f double+->double", smsi_max, r));
    insertIntoHashTable(ft, "min", newFunctionFD("f double+->double", smsi_min, r));
    insertIntoHashTable(ft, "average", newFunctionFD("f double+->double", smsi_average, r));
    insertIntoHashTable(ft, "like", newFunctionFD("string * string->boolean", smsi_like, r));
    insertIntoHashTable(ft, "not like", newFunctionFD("string * string->boolean", smsi_not_like, r));
    insertIntoHashTable(ft, "like regex", newFunctionFD("string * string->boolean", smsi_like_regex, r));
    insertIntoHashTable(ft, "not like regex", newFunctionFD("string * string->boolean", smsi_not_like_regex, r));
    insertIntoHashTable(ft, "delayExec", newFunctionFD("string * string * string->integer", smsi_delayExec, r));
    insertIntoHashTable(ft, "remoteExec", newFunctionFD("string * string * string * string->integer", smsi_remoteExec,r));
    insertIntoHashTable(ft, "writeLine", newFunctionFD("string * ?->integer", smsi_writeLine,r));
    insertIntoHashTable(ft, "writeString", newFunctionFD("string * ?->integer", smsi_writeString,r));
    insertIntoHashTable(ft, "triml", newFunctionFD("string * string->string", smsi_triml, r));
    insertIntoHashTable(ft, "trimr", newFunctionFD("string * string->string", smsi_trimr, r));
    insertIntoHashTable(ft, "strlen", newFunctionFD("string->integer", smsi_strlen, r));
    insertIntoHashTable(ft, "substr", newFunctionFD("string * integer * integer->string", smsi_substr, r));
    insertIntoHashTable(ft, "split", newFunctionFD("string * string -> list string", smsi_split, r));
    insertIntoHashTable(ft, "query", newFunctionFD("expression ? + -> `GenQueryInp_PI` * `GenQueryOut_PI`", smsi_query, r));
    insertIntoHashTable(ft, "unspeced", newFunctionFD("-> ?", smsi_undefined, r));
    insertIntoHashTable(ft, "msiAdmShowIRB", newFunctionFD("e ? ?->integer", smsi_msiAdmShowIRB, r));
    insertIntoHashTable(ft, "msiAdmShowCoreRE", newFunctionFD("e ? ?->integer", smsi_msiAdmShowCoreRE, r));
#ifdef DEBUG
    insertIntoHashTable(ft, "msiAdmAddAppRuleStruct", newFunctionFD("string->integer", smsi_msiAdmAddAppRuleStruct, r));
#else
    insertIntoHashTable(ft, "msiAdmAddAppRuleStruct", newFunctionFD("string * string * string->integer", smsi_msiAdmAddAppRuleStruct, r));
#endif
    insertIntoHashTable(ft, "msiAdmClearAppRuleStruct", newFunctionFD("->integer", smsi_msiAdmClearAppRuleStruct, r));
    insertIntoHashTable(ft, "msiAdmAppendToTopOfCoreRE", newFunctionFD("string->integer", smsi_msiAdmAppendToTopOfCoreRE, r));
    insertIntoHashTable(ft, "msiAdmChangeCoreRE", newFunctionFD("string->integer", smsi_msiAdmChangeCoreRE, r));
    insertIntoHashTable(ft, "msiAdmInsertRulesFromStructIntoDB", newFunctionFD("string * `RuleSet_PI` -> integer", smsi_msiAdmInsertRulesFromStructIntoDB, r));
    insertIntoHashTable(ft, "msiAdmReadRulesFromFileIntoStruct", newFunctionFD("string * d `RuleSet_PI` -> integer", smsi_msiAdmReadRulesFromFileIntoStruct, r));
    insertIntoHashTable(ft, "msiAdmWriteRulesFromStructIntoFile", newFunctionFD("string * `RuleSet_PI` -> integer", smsi_msiAdmWriteRulesFromStructIntoFile, r));
    insertIntoHashTable(ft, "msiAdmRetrieveRulesFromDBIntoStruct", newFunctionFD("string * string * d `RuleSet_PI` -> integer", smsi_msiAdmRetrieveRulesFromDBIntoStruct, r));
    insertIntoHashTable(ft, "collectionSpider", newFunctionFD("forall X in {string `CollInpNew_PI`}, expression ? * X * actions ? * actions ? -> integer", smsiCollectionSpider, r));
    insertIntoHashTable(ft, "rei->doi->dataSize", newFunctionFD("double", (SmsiFuncTypePtr) NULL, r));

#ifdef RE_BACKWARD_COMPATIBLE
    insertIntoHashTable(ft, "assignStr", newFunctionFD("e ? * e ?->integer", smsi_assignStr, r));
    insertIntoHashTable(ft, "ifExec", newFunctionFD("e boolean * a ? * a ? * a ? * a ?->?", smsi_ifExec, r));
    insertIntoHashTable(ft, "forExec", newFunctionFD("e ? * e boolean * a ? * a ? * a ?->?", smsi_forExec, r));
    insertIntoHashTable(ft, "whileExec", newFunctionFD("e boolean * a ? * a ?->?", smsi_whileExec, r));
    insertIntoHashTable(ft, "forEachExec", newFunctionFD("e list 0 * a ? * a ?->?", smsi_forEachExec, r));
    insertIntoHashTable(ft, "msiGetRulesFromDBIntoStruct", newFunctionFD("string * string * d `RuleSet_PI` -> integer", smsi_msiAdmRetrieveRulesFromDBIntoStruct, r));
#endif
    insertIntoHashTable(ft, "msiSegFault", newFunctionFD(" -> integer", smsi_segfault, r));


}
