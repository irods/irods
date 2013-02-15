/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include "typing.h"
#include "functions.h"
#define RE_ERROR(x) if(x) {goto error;}
#define RE_ERROR2(x,y) if(x) {localErrorMsg=(y);goto error;}
#define N_BASE_TYPES 6
NodeType baseTypes[N_BASE_TYPES] = {
		T_INT,
		T_BOOL,
		T_DOUBLE,
		T_DATETIME,
		T_STRING,
		T_IRODS,
};
void doNarrow(Node **l, Node **r, int ln, int rn, int flex, Node **nl, Node **nr, int *nln, int *nrn);
Satisfiability createSimpleConstraint(ExprType *a, ExprType *b, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r);
ExprType *createType(ExprType *t, Node **nc, int nn, Hashtable *typingEnv, Hashtable *equivalence, Region *r);
ExprType *getFullyBoundedVar(Region *r);

/**
 * return 0 to len-1 index of the parameter with type error
 *        -1 success
 */
int typeParameters(ExprType** paramTypes, int len, Node** subtrees, int dynamictyping, Env* funcDesc, Hashtable *symbol_type_table, List *typingConstraints, rError_t *errmsg, Node **errnode, Region *r) {
	int i;
	for(i=0;i<len;i++) {
		paramTypes[i] = dereference(typeExpression3(subtrees[i], dynamictyping, funcDesc, symbol_type_table, typingConstraints, errmsg, errnode, r), symbol_type_table, r);
		if(getNodeType(paramTypes[i]) == T_ERROR) {
			return i;
		}
	}
	return -1;
}

int tautologyLt(ExprType *type, ExprType *expected) {

    if(typeEqSyntatic(type, expected)) {
    	return 1;
	}
	if(getNodeType(type)==T_DYNAMIC) {
        return 0;
    }
    if(getNodeType(expected)==T_DYNAMIC) {
    	return 1;
    }
	int i;
	ExprType a, b;
	if (getNodeType(type) == T_VAR) {
		if(T_VAR_NUM_DISJUNCTS(type) > 0) {
			for(i=0;i<T_VAR_NUM_DISJUNCTS(type);i++) {
				setNodeType(&a, getNodeType(T_VAR_DISJUNCT(type,i)));
				if(!tautologyLt(&a, expected)) {
					return 0;
				}
			}
			return 1;
		} else {
			return 0;
		}

	} else if (getNodeType(expected) == T_VAR) {
		if(T_VAR_NUM_DISJUNCTS(expected) > 0) {
			for(i=0;i<T_VAR_NUM_DISJUNCTS(expected);i++) {
				setNodeType(&b, getNodeType(T_VAR_DISJUNCT(expected,i)));
				if(!tautologyLt(type, &b)) {
					return 0;
				}
			}
			return 1;
		} else {
			return 0;
		}
	} else if ((getNodeType(type) == T_CONS && getNodeType(expected) == T_CONS) || (getNodeType(type) == T_TUPLE && getNodeType(expected) == T_TUPLE) ) {
		if(getNodeType(type) == T_CONS && strcmp(T_CONS_TYPE_NAME(type), T_CONS_TYPE_NAME(expected))!=0) {
			return 0;
		}
		int i;
		for(i=0;i<T_CONS_ARITY(type);i++) {
			if(tautologyLt(T_CONS_TYPE_ARG(type, 0), T_CONS_TYPE_ARG(expected, 0))==0) {
				return 0;
			}
		}
		return 1;
	} else {
		return tautologyLtBase(type, expected);
	}
}
char *getBaseTypeOrTVarId(ExprType *a, char buf[128]) {
	if(isBaseType(a)) {
		snprintf(buf, 128, "%s", typeName_ExprType(a));
	} else {
		getTVarName(T_VAR_ID(a), buf);
	}
	return buf;
}
int tautologyLtBase(ExprType *a, ExprType *b) {
	if(getNodeType(a) == getNodeType(b)) {
		return 1;
	}
        switch(getNodeType(a)) {
            case T_INT:
                return getNodeType(b) == T_INT ||
                        getNodeType(b) == T_DOUBLE;
            case T_DOUBLE:
            case T_BOOL:
            case T_STRING:
                return getNodeType(a)==getNodeType(b);
            case T_IRODS:
            	return getNodeType(b)==T_IRODS && (a->text==NULL || b->text == NULL || strcmp(a->text, b->text) == 0) ? 1 : 0;
            default:
                return 0;
        }
}
int occursIn(ExprType *var, ExprType *type) {
	if(getNodeType(type) == T_VAR) {
		return T_VAR_ID(var) == T_VAR_ID(type);
	} else {
		int i;
		for(i=0;i<type->degree;i++) {
			if(occursIn(var, type->subtrees[i])) {
				return 1;
			}
		}
		return 0;
	}
}
ExprType* getEquivalenceClassRep(ExprType *varOrBase, Hashtable *equivalence) {
	ExprType *equiv1 = NULL, *equiv2;
	char name[128];
	equiv2 = varOrBase;
	int ref = 0;
	while(equiv2 != NULL) {
		equiv1 = equiv2;
		equiv2 = (ExprType *) lookupFromHashTable(equivalence, getBaseTypeOrTVarId(equiv1, name));
		ref ++;
	}
	if(ref > 1) {
		updateInHashTable(equivalence, getBaseTypeOrTVarId(varOrBase, name), equiv1);
	}
	return equiv1;

}


int occursInEquiv(ExprType *var, ExprType *type, Hashtable *equivalence) {
	if(getNodeType(type) == T_VAR && isBaseType(type)) {
		ExprType *varEquiv = getEquivalenceClassRep(var, equivalence);
		ExprType *typeEquiv = getEquivalenceClassRep(type, equivalence);
		return typeEqSyntatic(varEquiv, typeEquiv);
	} else {
		int i;
		for(i=0;i<type->degree;i++) {
			if(occursInEquiv(var, type->subtrees[i], equivalence)) {
				return 1;
			}
		}
		return 0;
	}
}

Satisfiability splitVarR(ExprType *consTuple, ExprType *var, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r) {
	if(occursInEquiv(var, consTuple, equivalence) || isBaseType(getEquivalenceClassRep(var, equivalence))) {
		return ABSURDITY;
	}
    char tvarname[128];
    ExprType **typeArgs = (ExprType **) region_alloc(r, sizeof(ExprType *) * T_CONS_ARITY(consTuple));
    int i;
    ExprType *type;
    for(i=0;i<T_CONS_ARITY(consTuple);i++) {
        typeArgs[i] = newTVar(r);
    }
    if(getNodeType(consTuple) == T_CONS) {
        type = newConsType(T_CONS_ARITY(consTuple), T_CONS_TYPE_NAME(consTuple), typeArgs, r);
    } else {
        type = newTupleType(T_CONS_ARITY(consTuple), typeArgs, r);
    }
    insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(var), tvarname), type);
    return splitConsOrTuple(consTuple, type, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);

}
Satisfiability splitVarL(ExprType *var, ExprType *consTuple, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r) {
	if(occursInEquiv(var, consTuple, equivalence) || isBaseType(getEquivalenceClassRep(var, equivalence))) {
		return ABSURDITY;
	}
    char tvarname[128];
    ExprType **typeArgs = (ExprType **) region_alloc(r, sizeof(ExprType *) * T_CONS_ARITY(consTuple));
    int i;
    ExprType *type;
    for(i=0;i<T_CONS_ARITY(consTuple);i++) {
        typeArgs[i] = newTVar(r);
    }
    if(getNodeType(consTuple) == T_CONS) {
        type = newConsType(T_CONS_ARITY(consTuple), T_CONS_TYPE_NAME(consTuple), typeArgs, r);
    } else {
        type = newTupleType(T_CONS_ARITY(consTuple), typeArgs, r);
    }
    insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(var), tvarname), type);
    return splitConsOrTuple(type, consTuple, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);

}

/*
 * simplify b which is a variable, bounded or unbounded, based on a which is a base type
 * returns 1 tautology
 *         0 contingency
 *         -1 absurdity
 */
Satisfiability simplifyR(ExprType *a, ExprType *b, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r) {
	ExprType *bm;
	if(T_VAR_NUM_DISJUNCTS(b) == 0) {
			bm = getFullyBoundedVar(r);
	} else {
		bm = b;
	}
	Node *cl[MAX_NUM_DISJUNCTS], *cr[MAX_NUM_DISJUNCTS];
	int nln, nrn;
	doNarrow(&a, T_VAR_DISJUNCTS(bm), 1, T_VAR_NUM_DISJUNCTS(bm), flex, cl, cr, &nln, &nrn);
	ExprType *bn;
	if(nrn == 0) {
		return ABSURDITY;
	} else {
		bn = createType(b, cr, nrn, typingEnv, equivalence, r);
		if(bn == b) {
			return TAUTOLOGY;
		}
		return createSimpleConstraint(a, bn, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);
	}
}

ExprType *getFullyBoundedVar(Region *r) {
	ExprType **ds = (ExprType **) region_alloc(r, sizeof(ExprType *) * N_BASE_TYPES);
	int i;
	for(i=0;i<N_BASE_TYPES;i++) {
		ds[i] = newSimpType(baseTypes[i], r);
	}
	ExprType *var = newTVar2(N_BASE_TYPES, ds, r);
	return var;
}

Satisfiability simplifyL(ExprType *a, ExprType *b, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r) {
	ExprType *am;
	if(T_VAR_NUM_DISJUNCTS(a) == 0) {
		am = getFullyBoundedVar(r);
	} else {
		am = a;
	}
	Node *cl[MAX_NUM_DISJUNCTS], *cr[MAX_NUM_DISJUNCTS];
	int nln, nrn;
	doNarrow(T_VAR_DISJUNCTS(am), &b, T_VAR_NUM_DISJUNCTS(am), 1, flex, cl, cr, &nln, &nrn);
	ExprType *an;
	if(nln == 0) {
		return ABSURDITY;
	} else {
		an = createType(a, cl, nln, typingEnv, equivalence, r);
		if(an == a) {
			return TAUTOLOGY;
		}
		return createSimpleConstraint(an, b, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);
	}

}

void addToEquivalenceClass(ExprType *a, ExprType *b, Hashtable *equivalence) {
	char name[128];
	ExprType *an = getEquivalenceClassRep(a, equivalence);
	ExprType *bn = getEquivalenceClassRep(b, equivalence);
	if(!typeEqSyntatic(an, bn)) {
		if(isBaseType(an)) {
			insertIntoHashTable(equivalence, getTVarName(T_VAR_ID(bn), name), an);
		} else {
			insertIntoHashTable(equivalence, getTVarName(T_VAR_ID(an), name), bn);
		}
	}
}
void doNarrow(Node **l, Node **r, int ln, int rn, int flex, Node **nl, Node **nr, int *nln, int *nrn) {
	Node *retl[MAX_NUM_DISJUNCTS], *retr[MAX_NUM_DISJUNCTS];
	int i,k;
	for(i=0;i<ln;i++) {
		retl[i] = NULL;
	}
	for(k=0;k<rn;k++) {
		retr[k] = NULL;
	}
	for(k=0;k<rn;k++) {
		for(i=0;i<ln;i++) {
			if(splitBaseType(l[i], r[k], flex) == TAUTOLOGY) {
				retl[i] = l[i];
				retr[k] = r[k];
				if(getNodeType(l[i]) == T_IRODS && l[i]->text == NULL) {
					retl[i] = retr[k];
				}
				if(getNodeType(r[k]) == T_IRODS && r[k]->text == NULL) {
					retr[k] = retl[i];
				}
			/*	break;*/
			}
		}
	}
	*nln = 0;
	for(i=0;i<ln;i++) {
		if(retl[i]!=NULL) {
			nl[(*nln)++] = retl[i];
		}
	}
	*nrn = 0;
	for(k=0;k<rn;k++) {
		if(retr[k]!=NULL) {
			nr[(*nrn)++] = retr[k];
		}
	}
}

Satisfiability createSimpleConstraint(ExprType *a, ExprType *b, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r) {
	char name[128];
	if(isBaseType(a) && isBaseType(b)) {
		return TAUTOLOGY;
	} else {
		addToEquivalenceClass(a, b, equivalence);
		if(flex) {
			listAppend(simpleTypingConstraints, newTypingConstraint(a, newUnaryType(T_FLEX, b, r), TC_LT, node, r), r);
			return CONTINGENCY;
		} else {
			if((getNodeType(a) == T_VAR && T_VAR_NUM_DISJUNCTS(a)==0) || isBaseType(b)) {
				insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(a), name), b);
			} else if((getNodeType(b) == T_VAR && T_VAR_NUM_DISJUNCTS(b)==0) || isBaseType(a)) {
				insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(b), name), a);
			} else { // T_VAR_NUM_DISJUNCTS(a) == T_VAR_NUM_DISJUNCTS(b)
				insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(a), name), b);
			}
			return TAUTOLOGY;
		}
	}
}
ExprType *createType(ExprType *t, Node **nc, int nn, Hashtable *typingEnv, Hashtable *equivalence, Region *r) {
	char name[128];
	ExprType *gcd;
	if (nn == T_VAR_NUM_DISJUNCTS(t)) {
		gcd = t;
	} else {
		if(nn==1) {
			gcd = *nc;
		} else {
			gcd = newTVar2(nn, nc, r);
		}
		insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(t), name), gcd);
		addToEquivalenceClass(t, gcd, equivalence);
	}
	return gcd;
}
Satisfiability narrow(ExprType *type, ExprType *expected, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r) {

	if(T_VAR_ID(type) == T_VAR_ID(expected)) {
		return TAUTOLOGY;
	} else if(T_VAR_NUM_DISJUNCTS(type) > 0 && T_VAR_NUM_DISJUNCTS(expected) > 0) {
		int nln, nrn;
		Node *cl[100], *cr[100];
		doNarrow(T_VAR_DISJUNCTS(type), T_VAR_DISJUNCTS(expected), T_VAR_NUM_DISJUNCTS(type), T_VAR_NUM_DISJUNCTS(expected), flex, cl, cr, &nln, &nrn);
		ExprType *an;
		ExprType *bn;
		if(nln == 0 || nrn == 0) {
			return ABSURDITY;
		} else {
			an = createType(type, cl, nln, typingEnv, equivalence, r);
			bn = createType(expected, cr, nrn, typingEnv, equivalence, r);
		}
		return createSimpleConstraint(an, bn, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);
	} else if(T_VAR_NUM_DISJUNCTS(type)==0) { /* free */
		return createSimpleConstraint(type, expected, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);
	} else /*if(T_VAR_NUM_DISJUNCTS(expected)==0)*/ { /* free */
		return createSimpleConstraint(type, expected, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);
	}
}
Satisfiability splitConsOrTuple(ExprType *a, ExprType *b, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r) {
/* split composite constraints with same top level type constructor */
	if((getNodeType(a) == T_CONS && strcmp(T_CONS_TYPE_NAME(a), T_CONS_TYPE_NAME(b)) != 0) ||
			T_CONS_ARITY(a) != T_CONS_ARITY(b)) {
		return ABSURDITY;
	} else {
		int i;
		Satisfiability ret = TAUTOLOGY;
		for(i=0;i<T_CONS_ARITY(a);i++) {
			ExprType *simpa = T_CONS_TYPE_ARG(a, i);
			ExprType *simpb = T_CONS_TYPE_ARG(b, i);
			Satisfiability sat = simplifyLocally(simpa, simpb, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);
			switch(sat) {
				case ABSURDITY:
					return ABSURDITY;
				case TAUTOLOGY:
					break;
				case CONTINGENCY:
					ret = CONTINGENCY;
					break;
			}

		}
		return ret;
	}
}


int isBaseType(ExprType *t) {
	int i;
	for(i=0;i<N_BASE_TYPES;i++) {
		if(getNodeType(t) == baseTypes[i]) {
			return 1;
		}
	}
	return 0;
}

Satisfiability splitBaseType(ExprType *tca, ExprType *tcb, int flex)
{
    return (flex && tautologyLtBase(tca, tcb)) ||
    		(!flex && ((getNodeType(tca) == T_IRODS && getNodeType(tcb) == T_IRODS && (tca->text == NULL || tcb->text == NULL)) ||
    				typeEqSyntatic(tca, tcb))) ? TAUTOLOGY : ABSURDITY;
}

Satisfiability simplifyLocally(ExprType *tca, ExprType *tcb, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r) {
/*
    char buf[1024], buf2[1024], buf3[1024], buf4[ERR_MSG_LEN];
    generateErrMsg("simplifyLocally: constraint generated from ", NODE_EXPR_POS(TC_NODE(tc)), TC_NODE(tc)->base, buf4);
    printf(buf4);
    snprintf(buf, 1024, "\t%s<%s\n",
                typeToString(TC_A(tc), NULL, buf2, 1024),
                typeToString(TC_B(tc), NULL, buf3, 1024));
    printf("%s", buf);
    snprintf(buf, 1024, "\tinstantiated: %s<%s\n",
                typeToString(TC_A(tc), typingEnv, buf2, 1024),
                typeToString(TC_B(tc), typingEnv, buf3, 1024));
    printf("%s", buf);
*/
    if(getNodeType(tcb) == T_FLEX) {
    	tcb = tcb->subtrees[0];
    	flex = 1;
    } else if(getNodeType(tcb) == T_FIXD) {
    	tcb = tcb->subtrees[0];
    }
    tca = dereference(tca, typingEnv, r);
    tcb = dereference(tcb, typingEnv, r);

	if(getNodeType(tca) == T_UNSPECED || getNodeType(tca) == T_DYNAMIC || getNodeType(tcb) == T_DYNAMIC) { /* is an undefined variable argument or a parameter with dynamic type */
		return TAUTOLOGY;
	}
	else if(isBaseType(tca) && isBaseType(tcb)) {
		return splitBaseType(tca, tcb, flex);
	} else if(getNodeType(tca) == T_VAR && getNodeType(tcb) == T_VAR) {
		return narrow(tca, tcb, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);

	} else if(getNodeType(tca)==T_VAR && isBaseType(tcb)) {
		return simplifyL(tca, tcb, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);

	} else if(getNodeType(tcb)==T_VAR && isBaseType(tca)) {
		return simplifyR(tca, tcb, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);

	} else if(getNodeType(tca)==T_VAR && (getNodeType(tcb) == T_CONS || getNodeType(tcb) == T_TUPLE)) {
		return splitVarL(tca, tcb, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);

	} else if(getNodeType(tcb)==T_VAR && (getNodeType(tca) == T_CONS || getNodeType(tca) == T_TUPLE)) {
		return splitVarR(tca, tcb, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);

	} else if((getNodeType(tca) == T_CONS && getNodeType(tcb) == T_CONS)
			  || (getNodeType(tca) == T_TUPLE && getNodeType(tcb) == T_TUPLE)) {
		return splitConsOrTuple(tca, tcb, flex, node, typingEnv, equivalence, simpleTypingConstraints, r);
	} else {
		return ABSURDITY;
	}
}
/*
 * not 0 solved
 * 0 not solved
 */
Satisfiability solveConstraints(List *typingConstraints, Hashtable *typingEnv, rError_t *errmsg, Node ** errnode, Region *r) {
/*
    char buf0[1024];
    typingConstraintsToString(typingConstraints, typingEnv, buf0, 1024);
    printf("solving constraints: %s\n", buf0);
*/
	ListNode *nextNode = NULL;
    do {
        Satisfiability sat = simplify(typingConstraints, typingEnv, errmsg, errnode, r);
        if(sat == ABSURDITY) {
            return ABSURDITY;
        }
        int changed = 0;
        nextNode = typingConstraints->head;
        while(nextNode!=NULL && !changed) {
            TypingConstraint *tc = (TypingConstraint *)nextNode->value;
/*
            char buf2[1024], buf3[1024];
*/
            /* printf("dereferencing %s and %s.\n", typeToString(TC_A(tc), typingEnv, buf2, 1024), typeToString(TC_B(tc), typingEnv, buf3, 1024)); */
            ExprType *a = dereference(TC_A(tc), typingEnv, r);
            ExprType *b = dereference(TC_B(tc), typingEnv, r);
            if(getNodeType(b) == T_FLEX || getNodeType(b) == T_FIXD) {
            	b = b->subtrees[0];
            }
/*
            printf("warning: collasping %s with %s.\n", typeToString(a, typingEnv, buf2, 1024), typeToString(b, typingEnv, buf3, 1024));
*/
                        /*printVarTypeEnvToStdOut(typingEnv); */
            if (getNodeType(a) == T_VAR && getNodeType(b) == T_VAR && T_VAR_ID(a) == T_VAR_ID(b)) {
            	listRemove(typingConstraints, nextNode);
            	nextNode = typingConstraints->head;
                changed = 1;
/*            } else if (getNodeType(a) == T_VAR && T_VAR_NUM_DISJUNCTS(a) == 0 &&
            		(getNodeType(b) != T_VAR || T_VAR_NUM_DISJUNCTS(b) != 0)) {
                insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(a), buf), b);
                listRemove(typingConstraints, nextNode);
            	nextNode = typingConstraints->head;
                changed = 1;
            } else if (getNodeType(b) == T_VAR && T_VAR_NUM_DISJUNCTS(b) == 0 &&
            		(getNodeType(a) != T_VAR || T_VAR_NUM_DISJUNCTS(a) != 0)) {
                insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(b), buf), a);
                listRemove(typingConstraints, nextNode);
            	nextNode = typingConstraints->head;
                changed = 1;
            } else if (getNodeType(a) == T_VAR && getNodeType(b) == T_VAR &&
            		T_VAR_NUM_DISJUNCTS(a) != 0 && T_VAR_NUM_DISJUNCTS(b) != 0) {
                if(T_VAR_NUM_DISJUNCTS(a) > T_VAR_NUM_DISJUNCTS(b)) {
                    insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(b), buf), a);
                } else {
                    insertIntoHashTable(typingEnv, getTVarName(T_VAR_ID(a), buf), b);
                }
                listRemove(typingConstraints, nextNode);
            	nextNode = typingConstraints->head;
                changed = 1;*/
            } else if (getNodeType(a) != T_VAR && getNodeType(b) != T_VAR) {
                printf("error: simplified type does not have variable on either side.\n");
            } else {
            	nextNode = nextNode->next;
            }
            /* printVarTypeEnvToStdOut(typingEnv); */
        }
    } while(nextNode != NULL);
    if(!consistent(typingConstraints, typingEnv, r)) {
    	return ABSURDITY;
    }
    return typingConstraints->head == NULL ? TAUTOLOGY : CONTINGENCY;
}

int consistent(List *typingConstraints, Hashtable *typingEnv, Region *r) {
	return 1;
}
Satisfiability simplify(List *typingConstraints, Hashtable *typingEnv, rError_t *errmsg, Node **errnode, Region *r) {
    ListNode *ln;
    int changed;
    Hashtable *equivalence = newHashTable2(10, r);
    List *simpleTypingConstraints = newList(r);
    /* printf("start\n"); */
    /*char buf[1024];
	typingConstraintsToString(typingConstraints, typingEnv, buf, 1024);*/
    Satisfiability ret = TAUTOLOGY;
    do {
        changed = typingEnv->len;
    	ln = typingConstraints->head;
    	/*typingConstraintsToString(typingConstraints, typingEnv, buf, 1024);
    	printf("constraints: \n%s\n\n", buf);*/
        while(ln!=NULL) {
        	TypingConstraint *tc = (TypingConstraint *)ln->value;
            switch(simplifyLocally(TC_A(tc), TC_B(tc), 0, TC_NODE(tc), typingEnv, equivalence, simpleTypingConstraints, r)) {
                case TAUTOLOGY:
                	break;
                case CONTINGENCY:
                    /* printf("contingency\n"); */
                    /* printf("tautology\n"); */
                    /*    TypingConstraint *tcp;
					tcp = tc;
					while(tcp!=NULL) {
						printf("simplified %s<%s to %s<%s.\n",
								typeToString(a, NULL, buf3, 1024), typeToString(b, NULL, buf4, 1024),
								typeToString(tcp->a, NULL, buf1, 1024), typeToString(tcp->b, NULL, buf2, 1024));
						tcp = tcp->next;
					}*/
                	ret = CONTINGENCY;
                    break;
                case ABSURDITY:
                    *errnode = TC_NODE(tc);
                    char errmsgbuf1[ERR_MSG_LEN], errmsgbuf2[ERR_MSG_LEN], buf2[1024], buf3[1024];
                    snprintf(errmsgbuf1, ERR_MSG_LEN, "simplify: unsolvable typing constraint %s < %s.\n", typeToString(TC_A(tc), typingEnv, buf2, 1024), typeToString(TC_B(tc), typingEnv, buf3, 1024));
                    generateErrMsg(errmsgbuf1, NODE_EXPR_POS((*errnode)), (*errnode)->base, errmsgbuf2);
                    addRErrorMsg(errmsg, RE_TYPE_ERROR, errmsgbuf2);
                    /*printVarTypeEnvToStdOut(typingEnv); */
                    /* printf("absurdity\n"); */
                    return ABSURDITY;
            }
            ln = ln->next;
        }
    	/*typingConstraintsToString(typingConstraints, typingEnv, buf, 1024);
    	printf("simplified constraints: \n%s\n\n", buf);
    	printHashtable(typingEnv, buf);
    	printf("env: \n%s\n", buf);*/
        typingConstraints->head = simpleTypingConstraints->head;
        typingConstraints->tail = simpleTypingConstraints->tail;
        simpleTypingConstraints->head = simpleTypingConstraints->tail = NULL;
    } while(changed < typingEnv->len);

    return ret;
}

ExprType* typeFunction3(Node* node, int dynamictyping, Env* funcDesc, Hashtable* var_type_table, List *typingConstraints, rError_t *errmsg, Node **errnode, Region *r) {
    /*printTree(node, 0); */
    int i;
    char *localErrorMsg;
    ExprType *res3 = NULL;
    /*char buf[1024];*/
    /*printf("typeing %s\n",fn); */
    /*printVarTypeEnvToStdOut(var_type_table); */
    Node *fn = node->subtrees[0];
    Node *arg = node->subtrees[1];


    if(getNodeType(fn) == TK_TEXT && strcmp(fn->text, "foreach") == 0) {
        RE_ERROR2(getNodeType(arg) != N_TUPLE || arg->degree != 3,"wrong number of arguments to microservice");
        RE_ERROR2(getNodeType(arg->subtrees[0])!=TK_VAR,"argument form error");
        char* varname = arg->subtrees[0]->text;
        ExprType *varType = (ExprType *)lookupFromHashTable(var_type_table, varname);
        ExprType *collType = varType == NULL? NULL:dereference(varType, var_type_table, r);
        if(collType!=NULL) {
            /* error if res is not a collection type, a type variable (primitive union type excluded), a pair, a CollInp_MS_T, or a string */
            switch(getNodeType(collType)) {
            case T_CONS:
                /* dereference element type as only top level vars are dereferenced by the dereference function and we are accessing a subtree of the type */
            	updateInHashTable(var_type_table, varname, dereference(T_CONS_TYPE_ARG(collType, 0), var_type_table, r));
            	break;
            case T_VAR:
            	if(T_VAR_NUM_DISJUNCTS(collType)!=0) {
            		Node *disjuncts[2] = {newSimpType(T_STRING, r), newIRODSType(CollInp_MS_T, r)};
            		Res *unified = unifyTVarL(collType, newTVar2(2, disjuncts, r), var_type_table, r);
            		RE_ERROR2(getNodeType(unified) == T_ERROR, "foreach is applied to a non collection type");
            		collType = dereference(collType, var_type_table, r);
            		updateInHashTable(var_type_table, varname, newIRODSType(DataObjInp_MS_T, r));
            		break;
            	} else {
                    /* overwrite type of collection variable */
            		/* if this has to be later unified with CollInp_MS_T, we need a special rule to handle it */
                    unifyTVarL(collType, newCollType(newTVar(r), r), var_type_table, r);
                    collType = dereference(collType, var_type_table, r);
                    updateInHashTable(var_type_table, varname, dereference(T_CONS_TYPE_ARG(collType, 0), var_type_table, r));
                    break;
            	}
            case T_TUPLE:
            	RE_ERROR2(T_CONS_ARITY(collType)!=2 ||
            			getNodeType(T_CONS_TYPE_ARG(collType, 0)) != T_IRODS ||
            			strcmp(T_CONS_TYPE_NAME(T_CONS_TYPE_ARG(collType, 0)), GenQueryInp_MS_T) !=0 ||
            			getNodeType(T_CONS_TYPE_ARG(collType, 1)) != T_IRODS ||
						strcmp(T_CONS_TYPE_NAME(T_CONS_TYPE_ARG(collType, 0)), GenQueryOut_MS_T) !=0, "foreach is applied to a non collection type");
            		updateInHashTable(var_type_table, varname, newIRODSType(KeyValPair_MS_T, r));
            		break;
            case T_IRODS:
            	RE_ERROR2(strcmp(T_CONS_TYPE_NAME(collType), CollInp_MS_T)!=0, "foreach is applied to a non collection type");
        		updateInHashTable(var_type_table, varname, newIRODSType(DataObjInp_MS_T, r));
        		break;
            case T_STRING:
        		updateInHashTable(var_type_table, varname, newIRODSType(DataObjInp_MS_T, r));
        		break;
            default:
            	RE_ERROR2(TRUE, "foreach is applied to a non collection type");
            	break;
            }


        } else {
            ExprType *elemType;
            insertIntoHashTable(var_type_table, varname, elemType = newTVar(r));
            collType = newCollType(elemType, r);
        }
        arg->subtrees[0]->exprType = collType;
        res3 = typeExpression3(arg->subtrees[1], dynamictyping, funcDesc, var_type_table,typingConstraints,errmsg,errnode,r);
        RE_ERROR2(getNodeType(res3) == T_ERROR, "foreach loop type error");
        res3 = typeExpression3(arg->subtrees[2], dynamictyping, funcDesc, var_type_table,typingConstraints,errmsg,errnode,r);
        RE_ERROR2(getNodeType(res3) == T_ERROR, "foreach recovery type error");
        setIOType(arg->subtrees[0], IO_TYPE_EXPRESSION);
        for(i = 1;i<3;i++) {
        	setIOType(arg->subtrees[i], IO_TYPE_ACTIONS);
        }
        ExprType **typeArgs = allocSubtrees(r, 3);
        typeArgs[0] = collType;
        typeArgs[1] = newTVar(r);
        typeArgs[2] = newTVar(r);
        arg->coercionType = newTupleType(3, typeArgs, r);

        updateInHashTable(var_type_table, varname, collType); /* restore type of collection variable */
        return res3;
    } else {
    	ExprType *fnType = typeExpression3(fn, dynamictyping, funcDesc, var_type_table,typingConstraints,errmsg,errnode,r);
    	if(getNodeType(fnType) == T_ERROR) return fnType;
    	N_TUPLE_CONSTRUCT_TUPLE(arg) = 1; /* arg must be a N_TUPLE node */
		ExprType *argType = typeExpression3(arg, dynamictyping, funcDesc, var_type_table,typingConstraints,errmsg,errnode,r);
		if(getNodeType(argType) == T_ERROR) return argType;

		ExprType *fType = getNodeType(fnType) == T_CONS && strcmp(fnType->text, FUNC) == 0 ? fnType : unifyWith(fnType, newFuncType(newTVar(r), newTVar(r), r), var_type_table, r);

		RE_ERROR2(getNodeType(fType) == T_ERROR, "the first component of a function application does not have a function type");
		ExprType *paramType = dereference(fType->subtrees[0], var_type_table, r);
		ExprType *retType = dereference(fType->subtrees[1], var_type_table, r);

        RE_ERROR2(getNodeType(fn) == TK_TEXT && strcmp(fn->text, "assign") == 0 &&
                arg->degree>0 &&
                !isPattern(arg->subtrees[0]), "the first argument of microservice assign is not a variable or a pattern");
        RE_ERROR2(getNodeType(fn) == TK_TEXT && strcmp(fn->text, "let") == 0 &&
                arg->degree>0 &&
                !isPattern(arg->subtrees[0]), "the first argument of microservice let is not a variable or a pattern");

/*
            printf("start typing %s\n", fn);
*/
/*
            printTreeDeref(node, 0, var_type_table, r);
*/
        char buf[ERR_MSG_LEN];
                    /* char errbuf[ERR_MSG_LEN]; */
                    char typebuf[ERR_MSG_LEN];
                    char typebuf2[ERR_MSG_LEN];ExprType *t = NULL;
		if(getVararg(fType) != OPTION_VARARG_ONCE) {
			/* generate instance of vararg tuple so that no vararg tuple goes into typing constraints */
			int fixParamN = paramType->degree - 1;
			int argN = node->subtrees[1] ->degree;
			int copyN = argN - fixParamN;
			ExprType **subtrees = paramType->subtrees;
			if(copyN < (getVararg(fType) == OPTION_VARARG_PLUS ? 1 : 0) || (getVararg(fType) == OPTION_VARARG_OPTIONAL && copyN > 1)) {
				snprintf(buf, 1024, "unsolvable vararg typing constraint %s < %s %s",
					typeToString(argType, var_type_table, typebuf, ERR_MSG_LEN),
					typeToString(paramType, var_type_table, typebuf2, ERR_MSG_LEN),
					getVararg(fType) == OPTION_VARARG_PLUS ? "*" : getVararg(fType) == OPTION_VARARG_OPTIONAL? "?" : "+");
				RE_ERROR2(1, buf);
			}
			ExprType **paramTypes = allocSubtrees(r, argN);
			int i;
			for(i = 0;i<fixParamN;i++) {
				paramTypes[i] = subtrees[i];
			}
			for(i=0;i<copyN;i++) {
				paramTypes[i+fixParamN] = subtrees[fixParamN];
			}
			t = newTupleType(argN, paramTypes, r);
		} else {
			t = paramType;
		}
		/*t = replaceDynamicWithNewTVar(t, r);
		argType = replaceDynamicWithNewTVar(argType, r);*/
        int ret = typeFuncParam(node->subtrees[1], argType, t, var_type_table, typingConstraints, errmsg, r);
        if(ret!=0) {
            *errnode = node->subtrees[1];
            RE_ERROR2(ret != 0, "parameter type error");
        }
		int i;
		for(i=0;i<node->subtrees[1]->degree;i++) {
			setIOType(node->subtrees[1]->subtrees[i], getIOType(t->subtrees[i]));
		}

        arg->coercionType = t; /* set coersion to parameter type */

/*
        printTreeDeref(node, 0, var_type_table, r);
        printf("finish typing %s\n", fn);
*/
        return instantiate(replaceDynamicWithNewTVar(retType, r), var_type_table, 0, r);
    }
    char errbuf[ERR_MSG_LEN];
    char errmsgbuf[ERR_MSG_LEN];
    error:
    *errnode = node;
    snprintf(errmsgbuf, ERR_MSG_LEN, "type error: %s in %s", localErrorMsg, fn->text);
    generateErrMsg(errmsgbuf, NODE_EXPR_POS((*errnode)), (*errnode)->base, errbuf);
    addRErrorMsg(errmsg, RE_TYPE_ERROR, errbuf);
    return newSimpType(T_ERROR,r);
}
ExprType *replaceDynamicWithNewTVar(ExprType *type, Region *r) {
		ExprType *newt = (ExprType *)region_alloc(r, sizeof(ExprType));
		*newt = *type;
		if(getNodeType(type) == T_DYNAMIC) {
			setNodeType(newt, T_VAR);
			T_VAR_ID(newt) = newTVarId();
		}
		int i;
		for(i=0;i<type->degree;i++) {
			newt->subtrees[i] = replaceDynamicWithNewTVar(type->subtrees[i], r);
		}
		return newt;
}
int typeFuncParam(Node *param, Node *paramType, Node *formalParamType, Hashtable *var_type_table, List *typingConstraints, rError_t *errmsg, Region *r) {
            /*char buf[ERR_MSG_LEN];
            char errbuf[ERR_MSG_LEN];
            char typebuf[ERR_MSG_LEN];
            char typebuf2[ERR_MSG_LEN]; */
            /* printf("typing param %s < %s\n",
                                typeToString(paramType, var_type_table, typebuf, ERR_MSG_LEN),
                                typeToString(formalParamType, var_type_table, typebuf2, ERR_MSG_LEN)); */


			TypingConstraint *tc = newTypingConstraint(paramType, formalParamType, TC_LT, param, r);
			listAppend(typingConstraints, tc, r);
			Node *errnode;
			Satisfiability tcons = simplify(typingConstraints, var_type_table, errmsg, &errnode, r);
			switch(tcons) {
				case TAUTOLOGY:
					break;
				case CONTINGENCY:
					break;
				case ABSURDITY:
				    return -1;
			}
            return 0;
}

ExprType* typeExpression3(Node *expr, int dynamictyping, Env *funcDesc, Hashtable *varTypes, List *typingConstraints, rError_t *errmsg, Node **errnode, Region *r) {
    ExprType *res;
    ExprType **components;
    ExprType* t = NULL;
	int i;
	expr->option |= OPTION_TYPED;
	if(dynamictyping) {
		switch(getNodeType(expr)) {
		case TK_TEXT:
			if(strcmp(expr->text,"nop")==0) {
				return expr->exprType = newFuncType(newTupleType(0, NULL, r), newSimpType(T_INT, r), r);
			} else {
				/* not a variable, evaluate as a function */
				FunctionDesc *fDesc;

				if(funcDesc != NULL && (fDesc = (FunctionDesc*)lookupFromEnv(funcDesc, expr->text))!=NULL && fDesc->exprType!=NULL) {
					return expr->exprType = dupType(fDesc->exprType, r);
				} else {
					ExprType *paramType = newSimpType(T_DYNAMIC, r);
					setIOType(paramType, IO_TYPE_DYNAMIC);
					ExprType *fType = newFuncType(newUnaryType(T_TUPLE, paramType, r), newSimpType(T_DYNAMIC, r), r);
					setVararg(fType, OPTION_VARARG_STAR);
					return expr->exprType = fType;
				}
			}
		case N_TUPLE:
			components = (ExprType **) region_alloc(r, sizeof(ExprType *) * expr->degree);
			for(i=0;i<expr->degree;i++) {
				components[i] = typeExpression3(expr->subtrees[i], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode,r);
				if(getNodeType(components[i]) == T_ERROR) {
					return expr->exprType = components[i];
				}
			}
			if(N_TUPLE_CONSTRUCT_TUPLE(expr) || expr->degree != 1) {
				return expr->exprType = newTupleType(expr->degree, components, r);
			} else {
				return expr->exprType = components[0];
			}

		case N_APPLICATION:
			/* try to type as a function */
			/* the exprType is used to store the type of the return value */
			return expr->exprType = typeFunction3(expr, dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode,r);
		case N_ACTIONS:
			if(expr->degree == 0) {
				/* type of empty action sequence == T_INT */
				return expr->exprType = newSimpType(T_INT, r);
			}
			for(i=0;i<expr->degree;i++) {
				/*printf("typing action in actions"); */
				res = typeExpression3(expr->subtrees[i], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode,r);
				/*printVarTypeEnvToStdOut(varTypes); */
				if(getNodeType(res) == T_ERROR) {
					return expr->exprType = res;
				}
			}
			return expr->exprType = res;
		case N_ACTIONS_RECOVERY:
			res = typeExpression3(expr->subtrees[0], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode, r);
			if(getNodeType(res) == T_ERROR) {
				return expr->exprType = res;
			}
			res = typeExpression3(expr->subtrees[1], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode, r);
			return expr->exprType = res;
		default:
			return expr->exprType = newSimpType(T_DYNAMIC, r);

		}
	} else {
		switch(getNodeType(expr)) {
		case TK_BOOL:
			return expr->exprType = newSimpType(T_BOOL, r);
		case TK_INT:
            return expr->exprType = newSimpType(T_INT,r);
        case TK_DOUBLE:
            return expr->exprType = newSimpType(T_DOUBLE,r);
		case TK_STRING:
			return expr->exprType = newSimpType(T_STRING,r);
		case TK_VAR:
			t = (ExprType *)lookupFromHashTable(varTypes, expr->text);
			if(t==NULL) {
				/* define new variable */
				t = newTVar(r);
				insertIntoHashTable(varTypes, expr->text, t);
			}
			t = dereference(t, varTypes, r);
			return expr->exprType = t;
		case TK_TEXT:
			if(strcmp(expr->text,"nop")==0) {
				return expr->exprType = newFuncType(newTupleType(0, NULL, r), newSimpType(T_INT, r), r);
			} else {
                /* not a variable, evaluate as a function */
			    FunctionDesc *fDesc = (FunctionDesc*)lookupFromEnv(funcDesc, expr->text);
                if(fDesc!=NULL && fDesc->exprType!=NULL) {
                    return expr->exprType = dupType(fDesc->exprType, r);
                } else {
                    ExprType *paramType = newSimpType(T_DYNAMIC, r);
                    setIOType(paramType, IO_TYPE_DYNAMIC);
                    ExprType *fType = newFuncType(newUnaryType(T_TUPLE, paramType, r), newSimpType(T_DYNAMIC, r), r);
                    setVararg(fType, OPTION_VARARG_STAR);
                    return expr->exprType = fType;
                }
			}
        case N_TUPLE:
            components = (ExprType **) region_alloc(r, sizeof(ExprType *) * expr->degree);
            for(i=0;i<expr->degree;i++) {
                components[i] = typeExpression3(expr->subtrees[i], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode,r);
                if(getNodeType(components[i]) == T_ERROR) {
                	return expr->exprType = components[i];
                }
            }
            if(N_TUPLE_CONSTRUCT_TUPLE(expr) || expr->degree != 1) {
            	return expr->exprType = newTupleType(expr->degree, components, r);
            } else {
            	return expr->exprType = components[0];
            }

		case N_APPLICATION:
			/* try to type as a function */
			/* the exprType is used to store the type of the return value */
			return expr->exprType = typeFunction3(expr, dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode,r);
		case N_ACTIONS:
			if(expr->degree == 0) {
				/* type of empty action sequence == T_INT */
				return expr->exprType = newSimpType(T_INT, r);
			}
			for(i=0;i<expr->degree;i++) {
				/*printf("typing action in actions"); */
				res = typeExpression3(expr->subtrees[i], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode,r);
				/*printVarTypeEnvToStdOut(varTypes); */
				if(getNodeType(res) == T_ERROR) {
					return expr->exprType = res;
				}
			}
			return expr->exprType = res;
		case N_ACTIONS_RECOVERY:
			res = typeExpression3(expr->subtrees[0], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode, r);
			if(getNodeType(res) == T_ERROR) {
				return expr->exprType = res;
			}
			res = typeExpression3(expr->subtrees[1], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode, r);
			return expr->exprType = res;
		case N_ATTR:
			/* todo type */
			for(i=0;i<expr->degree;i++) {
				res = typeExpression3(expr->subtrees[i], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode,r);
				if(getNodeType(res) == T_ERROR) {
					return expr->exprType = res;
				}
			}
			return expr->exprType = newSimpType(T_DYNAMIC, r);
		case N_QUERY_COND:
			/* todo type */
			for(i=0;i<expr->degree;i++) {
				res = typeExpression3(expr->subtrees[i], dynamictyping, funcDesc, varTypes, typingConstraints, errmsg, errnode,r);
				if(getNodeType(res) == T_ERROR) {
					return expr->exprType = res;
				}
			}
			return expr->exprType = newSimpType(T_DYNAMIC, r);
		case TK_COL:
			/* todo type */
			return expr->exprType = newSimpType(T_DYNAMIC, r);

        default:
			break;
		}
		*errnode = expr;
		char errbuf[ERR_MSG_LEN], errbuf0[ERR_MSG_LEN];
		snprintf(errbuf0, ERR_MSG_LEN, "error: unsupported ast node %d", getNodeType(expr));
		generateErrMsg(errbuf0, NODE_EXPR_POS(expr), expr->base, errbuf);
		addRErrorMsg(errmsg, RE_TYPE_ERROR, errbuf);
		return expr->exprType = newSimpType(T_ERROR,r);
	}
}
/*
 * This process is based on a few assumptions:
 * If the type coerced to cannot be inferred, i.e., an expression is to be coerced to some tvar, bounded to a union type or free,
 * then we leave further type checking to runtime, which can be done locally under the assumption of indistinguishable inclusion.
 *
 */
void postProcessCoercion(Node *expr, Hashtable *varTypes, rError_t *errmsg, Node **errnode, Region *r) {
    expr->coercionType = expr->coercionType==NULL?NULL:instantiate(expr->coercionType, varTypes, 0, r);
    expr->exprType = expr->exprType==NULL?NULL:instantiate(expr->exprType, varTypes, 0, r);
    int i;
    for(i=0;i<expr->degree;i++) {
        postProcessCoercion(expr->subtrees[i], varTypes, errmsg, errnode, r);
    }
    if(expr->coercionType!=NULL && expr->exprType!=NULL) {
                /*char buf[128];*/
                /*typeToString(expr->coercionType, NULL, buf, 128);
                printf("%s", buf);*/
            if(getNodeType(expr) == N_TUPLE) {
            	ExprType **csubtrees = expr->coercionType->subtrees;
            	int i;
            	for(i=0;i<expr->degree;i++) {
                    if(typeEqSyntatic(expr->subtrees[i]->exprType, csubtrees[i])) {
                		expr->subtrees[i]->option &= ~OPTION_COERCE;
                    } else {
                		expr->subtrees[i]->option |= OPTION_COERCE;
                    }
            	}
            }
    }
}
/*
 * convert single action to actions if the parameter is of type actions
 */
void postProcessActions(Node *expr, Env *systemFunctionTables, rError_t *errmsg, Node **errnode, Region *r) {
    int i;
    switch(getNodeType(expr)) {
        case N_TUPLE:
            for(i=0;i<expr->degree;i++) {
                if(getIOType(expr->subtrees[i]) == IO_TYPE_ACTIONS && getNodeType(expr->subtrees[i]) != N_ACTIONS) {
                    setIOType(expr->subtrees[i], IO_TYPE_INPUT);
                    Node **params = (Node **)region_alloc(r, sizeof(Node *)*1);
                    params[0] = expr->subtrees[i];
                    Label pos;
                    pos.base = expr->base;
                    pos.exprloc = NODE_EXPR_POS(expr);
                    expr->subtrees[i] = createActionsNode(params, 1, &pos, r);
                    setIOType(expr->subtrees[i], IO_TYPE_ACTIONS);
                    expr->subtrees[i]->exprType = params[0]->exprType;
                }
            }
            break;
        default:
            break;
    }
    for(i=0;i<expr->degree;i++) {
        postProcessActions(expr->subtrees[i], systemFunctionTables, errmsg, errnode, r);
    }
}
