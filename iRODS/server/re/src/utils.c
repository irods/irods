/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "utils.h"
#include "restructs.h"
#include "conversion.h"
#include "configuration.h"
#include "reVariableMap.gen.h"
/* make a new type by substituting tvars with fresh tvars */
ExprType *dupType(ExprType *ty, Region *r) {
    Hashtable *varTable = newHashTable2(100, r);
    /* todo add oom handler here */
    ExprType *dup = dupTypeAux(ty, r, varTable);
    return dup;


}

int typeEqSyntatic(ExprType *a, ExprType *b) {
    if(getNodeType(a)!=getNodeType(b) || getVararg(a) != getVararg(b)) {
        return 0;
    }
    switch(getNodeType(a)) {
        case T_CONS:
        case T_TUPLE:
            if(T_CONS_ARITY(a) == T_CONS_ARITY(b) &&
                    (getNodeType(a) == T_TUPLE || strcmp(T_CONS_TYPE_NAME(a), T_CONS_TYPE_NAME(b)) == 0)) {
                int i;
                for(i=0;i<T_CONS_ARITY(a);i++) {
                    if(!typeEqSyntatic(T_CONS_TYPE_ARG(a, i),T_CONS_TYPE_ARG(b, i))) {
                        return 0;
                    }
                }
                return 1;
            }
            return 0;
        case T_VAR:
            return T_VAR_ID(a) == T_VAR_ID(b);
        case T_IRODS:
            return strcmp(a->text, b->text) == 0;
        default:
            return 1;
    }

}

ExprType *dupTypeAux(ExprType *ty, Region *r, Hashtable *varTable) {
    ExprType **paramTypes;
    int i;
    ExprType *newt;
    ExprType *exist;
    char *name;
    char buf[128];
    switch(getNodeType(ty)) {
        case T_CONS:
            paramTypes = (ExprType **) region_alloc(r,sizeof(ExprType *)*T_CONS_ARITY(ty));
            for(i=0;i<T_CONS_ARITY(ty);i++) {
                paramTypes[i] = dupTypeAux(T_CONS_TYPE_ARG(ty, i),r,varTable);
            }
            newt = newConsType(T_CONS_ARITY(ty), T_CONS_TYPE_NAME(ty), paramTypes, r);
            newt->option = ty->option;
            break;
        case T_TUPLE:
            paramTypes = (ExprType **) region_alloc(r,sizeof(ExprType *)*T_CONS_ARITY(ty));
            for(i=0;i<T_CONS_ARITY(ty);i++) {
                paramTypes[i] = dupTypeAux(T_CONS_TYPE_ARG(ty, i),r,varTable);
            }
            newt = newTupleType(T_CONS_ARITY(ty), paramTypes, r);
            newt->option = ty->option;
            break;
        case T_VAR:
            name = getTVarName(T_VAR_ID(ty), buf);
            exist = (ExprType *)lookupFromHashTable(varTable, name);
            if(exist != NULL)
                newt = exist;
            else {
                newt = newTVar2(T_VAR_NUM_DISJUNCTS(ty), T_VAR_DISJUNCTS(ty), r);
                insertIntoHashTable(varTable, name, newt);

            }
            newt->option = ty->option;
            break;
        case T_FLEX:
            paramTypes = (ExprType **) region_alloc(r,sizeof(ExprType *)*1);
            paramTypes[0] = dupTypeAux(ty->subtrees[0],r,varTable);
            newt = newExprType(T_FLEX, 1, paramTypes, r);
            newt->option = ty->option;
            break;

        default:
            newt = ty;
            break;
    }
	return newt;
}
int coercible(ExprType *a, ExprType *b) {
            return (getNodeType(a)!=T_CONS && getNodeType(a) == getNodeType(b)) ||
                   (getNodeType(b) == T_DOUBLE && getNodeType(a) == T_INT) ||
                   (getNodeType(b) == T_DOUBLE && getNodeType(a) == T_STRING) ||
                   (getNodeType(b) == T_INT && getNodeType(a) == T_DOUBLE) ||
                   (getNodeType(b) == T_INT && getNodeType(a) == T_STRING) ||
                   (getNodeType(b) == T_STRING && getNodeType(a) == T_INT) ||
                   (getNodeType(b) == T_STRING && getNodeType(a) == T_DOUBLE) ||
                   (getNodeType(b) == T_STRING && getNodeType(a) == T_BOOL) ||
                   (getNodeType(b) == T_BOOL && getNodeType(a) == T_STRING) ||
                   (getNodeType(b) == T_DATETIME && getNodeType(a) == T_INT) ||
                   (getNodeType(b) == T_DATETIME && getNodeType(a) == T_DOUBLE) ||
                   (getNodeType(b) == T_DYNAMIC) ||
                   (getNodeType(a) == T_DYNAMIC) ||
                   (getNodeType(a)==T_CONS && getNodeType(b)==T_CONS && coercible(T_CONS_TYPE_ARG(a, 0), T_CONS_TYPE_ARG(b, 0)));
}
/*
 * unify a free tvar or union type with some other type
 */
ExprType* unifyTVarL(ExprType *type, ExprType* expected, Hashtable *varTypes, Region *r) {
    char buf[128];
    if(T_VAR_NUM_DISJUNCTS(type)==0) { /* free */
    	if(occursIn(type, expected)) {
    		return NULL;
    	}
        insertIntoHashTable(varTypes, getTVarName(T_VAR_ID(type), buf), expected);
        return dereference(expected, varTypes, r);
    } else { /* union type */
        int i;
        ExprType *ty = NULL;
        for(i=0;i<T_VAR_NUM_DISJUNCTS(type);i++) {
                if(getNodeType(T_VAR_DISJUNCT(type,i)) == getNodeType(expected)) { /* union types can only include primitive types */
                    ty = expected;
                    break;
                }
        }
        if(ty != NULL) {
            insertIntoHashTable(varTypes, getTVarName(T_VAR_ID(type), buf), expected);
        }
        return ty;
    }

}
ExprType* unifyTVarR(ExprType *type, ExprType* expected, Hashtable *varTypes, Region *r) {
    char buf[128];
    if(T_VAR_NUM_DISJUNCTS(expected)==0) { /* free */
    	if(occursIn(expected, type)) {
    		return NULL;
    	}
        insertIntoHashTable(varTypes, getTVarName(T_VAR_ID(expected), buf), type);
        return dereference(expected, varTypes, r);
    } else { /* union type */
        int i;
        ExprType *ty = NULL;
        for(i=0;i<T_VAR_NUM_DISJUNCTS(expected);i++) {
            if(getNodeType(type) == getNodeType(T_VAR_DISJUNCT(expected,i))) { /* union types can only include primitive types */
                ty = type;
            }
        }
        if(ty != NULL) {
            insertIntoHashTable(varTypes, getTVarName(T_VAR_ID(expected), buf), ty);
            return dereference(expected, varTypes, r);
        }
        return ty;
    }

}
/**
 * return The most general common instance of type and expected if unifiable
 *        NULL if false
 */
ExprType* unifyWith(ExprType *type, ExprType* expected, Hashtable *varTypes, Region *r) {
	if(getVararg(type) != getVararg(expected)) {
		return NULL;
	}
    char buf[128];
    /* dereference types to get the most specific type */
    /* as dereference only deref top level types, it is necessary to call dereference again */
    /* when unification is needed for subexpressions of the types which can be performed by calling this function */
    type = dereference(type, varTypes, r);
    expected = dereference(expected, varTypes, r);
    if(getNodeType(type) == T_UNSPECED) {
        return expected;
    }
    if(getNodeType(expected) == T_DYNAMIC) {
        return type;
    }
    if(getNodeType(type) == T_VAR && getNodeType(expected) == T_VAR) {
        if(T_VAR_ID(type) == T_VAR_ID(expected)) {
            /* if both dereference to the same tvar then do not modify var types table */
            return type;
        } else if(T_VAR_NUM_DISJUNCTS(type) > 0 && T_VAR_NUM_DISJUNCTS(expected) > 0) {
            Node *c[10];
            Node** cp = c;
            int i,k;
            for(k=0;k<T_VAR_NUM_DISJUNCTS(expected);k++) {
                for(i=0;i<T_VAR_NUM_DISJUNCTS(type);i++) {
                    if(getNodeType(T_VAR_DISJUNCT(type,i)) == getNodeType(T_VAR_DISJUNCT(expected,k))) {
                        *(cp++)=T_VAR_DISJUNCT(expected,k);
                        break;
                    }
                }
            }
            if(cp == c) {
                return NULL;
            } else {
                ExprType *gcd;
                if(cp-c==1) {
                    gcd = *c;
                } else {
                    gcd = newTVar2(cp-c, c, r);
                }
                updateInHashTable(varTypes, getTVarName(T_VAR_ID(type), buf), gcd);
                updateInHashTable(varTypes, getTVarName(T_VAR_ID(expected), buf), gcd);
                return gcd;
            }
        } else {
            if(T_VAR_NUM_DISJUNCTS(type)==0) { /* free */
                insertIntoHashTable(varTypes, getTVarName(T_VAR_ID(type), buf), expected);
                return dereference(expected, varTypes, r);
            } else if(T_VAR_NUM_DISJUNCTS(expected)==0) { /* free */
                insertIntoHashTable(varTypes, getTVarName(T_VAR_ID(expected), buf), type);
                return dereference(expected, varTypes, r);
            } else {
                /* error unreachable */
                return NULL;
            }
        }
    } else
	if(getNodeType(type) == T_VAR) {
            return unifyTVarL(type, expected, varTypes, r);
	} else if(getNodeType(expected) == T_VAR) {
            return unifyTVarR(type, expected, varTypes, r);
	} else {
            return unifyNonTvars(type, expected, varTypes, r);
	}
}
/**
 * Unify non tvar or union types.
 * return The most general instance if unifiable
 *        NULL if not
 */
ExprType* unifyNonTvars(ExprType *type, ExprType *expected, Hashtable *varTypes, Region *r) {
	if(getNodeType(type) == T_CONS && getNodeType(expected) == T_CONS) {
		if(strcmp(T_CONS_TYPE_NAME(type), T_CONS_TYPE_NAME(expected)) == 0
			&& T_CONS_ARITY(type) == T_CONS_ARITY(expected)) {
            ExprType **subtrees = (ExprType **) region_alloc(r, sizeof(ExprType *) * T_CONS_ARITY(expected));

			int i;
			for(i=0;i<T_CONS_ARITY(type);i++) {
				ExprType *elemType = unifyWith(
                                        T_CONS_TYPE_ARG(type, i),
                                        T_CONS_TYPE_ARG(expected, i),
                                        varTypes,r); /* unifyWithCoercion performs dereference */
				if(elemType == NULL) {
					return NULL;
				}
				subtrees[i] = elemType;
			}
			return dereference(newConsType(T_CONS_ARITY(expected), T_CONS_TYPE_NAME(expected), subtrees, r), varTypes, r);
		} else {
			return NULL;
		}
        } else if(getNodeType(type) == T_IRODS || getNodeType(expected) == T_IRODS) {
                if(strcmp(type->text, expected->text)!=0) {
                        return NULL;
                }
                return expected;
	} else if(getNodeType(expected) == getNodeType(type)) { /* primitive types */
                return expected;
	} else {
            return newErrorType(RE_TYPE_ERROR, r);
	}
}
/*
int unifyPrim(ExprType *type, TypeConstructor prim, Hashtable *typeVars, Region *r) {
	ExprType primType;
	primType.t = prim;
	ExprType *unifiedType = unifyNonTvars(type, &primType, typeVars, r);
	if(unifiedType == NULL) {
		return 0;
	} else {
		return 1;
	}
}
*/

/* utility function */
char* getTVarName(int vid, char name[128]) {
    snprintf(name, 128, "?%d",vid);
    return name;
}
char* getTVarNameRegion(int vid, Region *r) {
    char *name = (char *) region_alloc(r, sizeof(char)*128);
    snprintf(name, 128, "?%d",vid);
    return name;
}
char* getTVarNameRegionFromExprType(ExprType *tvar, Region *r) {
    return getTVarNameRegion(T_VAR_ID(tvar), r);
}


int newTVarId() {
    return ruleEngineConfig.tvarNumber ++;
}

/* copy to new region
 * If the new region is the same as the old region then do not copy.
 */
/* Res *cpRes(Res *res0, Region *r) {
    Res *res;
    if(!IN_REGION(res0, r)) {
        res = newRes(r);
        *res = *res0;
    } else {
        res = res0;
    }
    if(res->exprType!=NULL) {
        res->exprType = cpType(res->exprType, r);
    }
    if(res->text != NULL) {
        res->text = cpString(res->text, r);
    }
    if(res->param!= NULL) {
    	res->param = newMsParam(cpString(res->param->type, r), res->param->inOutStruct, res->param->inpOutBuf, r);
    }
    int i;
    if(res->subtrees!=NULL) {
        if(!IN_REGION(res->subtrees, r)) {
            Node **temp = (Node **)region_alloc(r, sizeof(Node *) * res->degree);
            memcpy(temp, res->subtrees, sizeof(Node *) * res->degree);
            res->subtrees = temp;
        }
        for(i=0;i<res->degree;i++) {
            res->subtrees[i] = cpRes(res->subtrees[i], r);
        }
    }
    return res;
}*/

char *cpString(char *str, Region *r) {
    if(IN_REGION(str, r)) {
        return str;
    } else
    return cpStringExt(str, r);
}
char *cpStringExt(char *str, Region *r) {
    char *strCp = (char *)region_alloc(r, (strlen(str)+1) * sizeof(char) );
    strcpy(strCp, str);
    return strCp;
}
/* ExprType *cpType(ExprType *ty, Region *r) {
    if(IN_REGION(ty, r)) {
        return ty;
    }
    int i;
    ExprType *newt;
    newt = (ExprType *) region_alloc(r, sizeof(ExprType));
    memcpy(newt, ty, sizeof(ExprType));
    if(ty->subtrees != NULL) {
        newt->subtrees = (ExprType **) region_alloc(r,sizeof(ExprType *)*ty->degree);
        for(i=0;i<ty->degree;i++) {
            newt->subtrees[i] = cpType(ty->subtrees[i],r);
        }
    }
    if(ty->text != NULL) {
        newt->text = cpString(ty->text, r);
    }

    return newt;
}*/
/* copy res values from other region to r */
void cpHashtable(Hashtable *env, Region *r) {
	int i;

	for(i=0;i<env->size;i++) {
            struct bucket *b = env->buckets[i];
            while(b!=NULL) {
                b->value = cpRes((Res *)b->value, r);
                b= b->next;
            }
	}
}

void cpEnv(Env *env, Region *r) {
    cpHashtable(env->current, r);
    if(env->previous!=NULL) {
        cpEnv(env->previous, r);
    }
}

/* copy from old region to new region
 * If the new region is the same as the old region then do not copy.
 */
/* Res *cpRes2(Res *res0, Region *oldr, Region *r) {
    Res *res;
    if(IN_REGION(res0, oldr)) {
        res = newRes(r);
        *res = *res0;
    } else {
        res = res0;
    }
    if(res->exprType!=NULL) {
        res->exprType = cpType2(res->exprType, oldr, r);
    }
    if(res->text != NULL) {
        res->text = cpString2(res->text, oldr, r);
    }
    if(res->param!= NULL && !(IN_REGION(res->param->type, r) && IN_REGION(res->param, r))) {
    	res->param = newMsParam(cpString2(res->param->type, oldr, r), res->param->inOutStruct, res->param->inpOutBuf, r);
    }

    int i;
    if(res->subtrees!=NULL) {
        if(IN_REGION(res->subtrees, oldr)) {
            Node **temp = (Node **)region_alloc(r, sizeof(Node *) * res->degree);
            memcpy(temp, res->subtrees, sizeof(Node *) * res->degree);
            res->subtrees = temp;
        }
        for(i=0;i<res->degree;i++) {
            res->subtrees[i] = cpRes2(res->subtrees[i], oldr, r);
        }
    }
    return res;
} */

char *cpString2(char *str, Region *oldr, Region *r) {
    if(!IN_REGION(str, oldr)) {
        return str;
    } else
    return cpStringExt(str, r);
}
/* ExprType *cpType2(ExprType *ty, Region *oldr, Region *r) {
    if(!IN_REGION(ty, oldr)) {
        return ty;
    }
    int i;
    ExprType *newt;
    newt = (ExprType *) region_alloc(r, sizeof(ExprType));
    memcpy(newt, ty, sizeof(ExprType));
    if(ty->subtrees != NULL) {
        newt->subtrees = (ExprType **) region_alloc(r,sizeof(ExprType *)*ty->degree);
        for(i=0;i<ty->degree;i++) {
            newt->subtrees[i] = cpType2(ty->subtrees[i],oldr, r);
        }
    }
    if(ty->text != NULL) {
        newt->text = cpString2(ty->text, oldr, r);
    }

    return newt;
} */
/* copy res values from region oldr to r */
void cpHashtable2(Hashtable *env, Region *oldr, Region *r) {
	int i;

	for(i=0;i<env->size;i++) {
            struct bucket *b = env->buckets[i];
            while(b!=NULL) {
                b->value = cpRes2((Res *)b->value, oldr, r);
                b= b->next;
            }
	}
}

void cpEnv2(Env *env, Region *oldr, Region *r) {
    cpHashtable2(env->current, oldr, r);
    if(env->previous!=NULL) {
        cpEnv2(env->previous, oldr, r);
    }
}


void printIndent(int n) {
	int i;
	for(i=0;i<n;i++) {
		printf("\t");
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

void printTreeDeref(Node *n, int indent, Hashtable *var_types, Region *r) {
	printIndent(indent);
	printf("%s:%d->",n->text, getNodeType(n));
        printType(n->coercionType, var_types);
        printf("\n");
	int i;
	for(i=0;i<n->degree;i++) {
		printTreeDeref(n->subtrees[i],indent+1, var_types, r);
	}

}
void printType(ExprType *type, Hashtable *var_types) {
    char buf[1024];
    typeToString(type, var_types, buf, 1024);
    printf("%s", buf);
}

char* typeToString(ExprType *type, Hashtable *var_types, char *buf, int bufsize) {
    buf[0] = '\0';
    Region *r = make_region(0, NULL);
    if(getVararg(type) != OPTION_VARARG_ONCE) {
    	snprintf(buf+strlen(buf), bufsize-strlen(buf), "vararg ");
    }
        ExprType *etype = type;
        if(getNodeType(etype) == T_VAR && var_types != NULL) {
            /* dereference */
            etype = dereference(etype, var_types, r);
        }

        if(getNodeType(etype) == T_VAR) {
        	snprintf(buf+strlen(buf), bufsize-strlen(buf), "%s ", etype == NULL?"?":typeName_ExprType(etype));
            snprintf(buf+strlen(buf), bufsize-strlen(buf), "%d", T_VAR_ID(etype));
            if(T_VAR_NUM_DISJUNCTS(type)!=0) {
                snprintf(buf+strlen(buf), bufsize-strlen(buf), "{");
                int i;
                for(i=0;i<T_VAR_NUM_DISJUNCTS(type);i++) {
                    snprintf(buf+strlen(buf), bufsize-strlen(buf), "%s ", typeName_ExprType(T_VAR_DISJUNCT(type, i)));
                }
                snprintf(buf+strlen(buf), bufsize-strlen(buf), "}");
            }
        } else if(getNodeType(etype) == T_CONS) {
        	if(strcmp(etype->text, FUNC) == 0) {
        		snprintf(buf+strlen(buf), bufsize-strlen(buf), "(");
				typeToString(T_CONS_TYPE_ARG(etype, 0), var_types, buf+strlen(buf), bufsize-strlen(buf));
				snprintf(buf+strlen(buf), bufsize-strlen(buf), ")");
        		snprintf(buf+strlen(buf), bufsize-strlen(buf), "->");
        		typeToString(T_CONS_TYPE_ARG(etype, 1), var_types, buf+strlen(buf), bufsize-strlen(buf));
			} else {

        	snprintf(buf+strlen(buf), bufsize-strlen(buf), "%s ", T_CONS_TYPE_NAME(etype));
            int i;
            if(T_CONS_ARITY(etype) != 0) {
				snprintf(buf+strlen(buf), bufsize-strlen(buf), "(");
				for(i=0;i<T_CONS_ARITY(etype);i++) {
					if(i!=0) {
						snprintf(buf+strlen(buf), bufsize-strlen(buf), ", ");
					}
					typeToString(T_CONS_TYPE_ARG(etype, i), var_types, buf+strlen(buf), bufsize-strlen(buf));
				}
				snprintf(buf+strlen(buf), bufsize-strlen(buf), ")");
            }
			}
        } else if(getNodeType(etype) == T_FLEX) {
        	snprintf(buf+strlen(buf), bufsize-strlen(buf), "%s ", typeName_ExprType(etype));
            typeToString(etype->subtrees[0], var_types, buf+strlen(buf), bufsize-strlen(buf));
        } else if(getNodeType(etype) == T_FIXD) {
        	snprintf(buf+strlen(buf), bufsize-strlen(buf), "%s ", typeName_ExprType(etype));
            typeToString(etype->subtrees[0], var_types, buf+strlen(buf), bufsize-strlen(buf));
        	snprintf(buf+strlen(buf), bufsize-strlen(buf), "=> ");
        	typeToString(etype->subtrees[1], var_types, buf+strlen(buf), bufsize-strlen(buf));
        } else if(getNodeType(etype) == T_TUPLE) {
        	if(T_CONS_ARITY(etype) == 0) {
        		snprintf(buf+strlen(buf), bufsize-strlen(buf), "unit");
        	} else {
        		if(T_CONS_ARITY(etype) == 1) {
            		snprintf(buf+strlen(buf), bufsize-strlen(buf), "(");
        		}
				int i;
				for(i=0;i<T_CONS_ARITY(etype);i++) {
					if(i!=0) {
						snprintf(buf+strlen(buf), bufsize-strlen(buf), " * ");
					}
					typeToString(T_CONS_TYPE_ARG(etype, i), var_types, buf+strlen(buf), bufsize-strlen(buf));
				}
        		if(T_CONS_ARITY(etype) == 1) {
            		snprintf(buf+strlen(buf), bufsize-strlen(buf), ")");
        		}
        	}
        } else {
        	snprintf(buf+strlen(buf), bufsize-strlen(buf), "%s ", etype == NULL?"?":typeName_ExprType(etype));
        }

    int i = strlen(buf) - 1;
    while(buf[i]==' ') i--;
    buf[i+1]='\0';

    region_free(r);
    return buf;

}
void typingConstraintsToString(List *typingConstraints, Hashtable *var_types, char *buf, int bufsize) {
    char buf2[1024];
    char buf3[1024];
    ListNode *p = typingConstraints->head;
    buf[0] = '\0';
    while(p!=NULL) {
        snprintf(buf + strlen(buf), bufsize-strlen(buf), "%s<%s\n",
                typeToString(TC_A((TypingConstraint *)p->value), NULL, /*var_types,*/ buf2, 1024),
                typeToString(TC_B((TypingConstraint *)p->value), NULL, /*var_types,*/ buf3, 1024));
        p=p->next;
    }
}
ExprType *dereference(ExprType *type, Hashtable *type_table, Region *r) {
    if(getNodeType(type) == T_VAR) {
        char name[128];
        getTVarName(T_VAR_ID(type), name);
        /* printf("deref: %s\n", name); */
        ExprType *deref = (ExprType *)lookupFromHashTable(type_table, name);
        if(deref == NULL)
            return type;
        else
            return dereference(deref, type_table, r);
    }
    return type;
}

ExprType *instantiate(ExprType *type, Hashtable *type_table, int replaceFreeVars, Region *r) {
    ExprType **paramTypes;
    int i;
    ExprType *typeInst;
    int changed = 0;

    switch(getNodeType(type)) {
        case T_VAR:
            typeInst = dereference(type, type_table, r);
            if(typeInst == type) {
                return replaceFreeVars?newSimpType(T_UNSPECED, r): type;
            } else {
                return instantiate(typeInst, type_table, replaceFreeVars, r);
            }
        default:
            if(type->degree != 0) {
                paramTypes = (ExprType **) region_alloc(r,sizeof(ExprType *)*type->degree);
                for(i=0;i<type->degree;i++) {
                    paramTypes[i] = instantiate(type->subtrees[i], type_table, replaceFreeVars, r);
                    if(paramTypes[i]!=type->subtrees[i]) {
                        changed = 1;
                    }
                }
            }
            if(changed) {
                ExprType *inst = (ExprType *) region_alloc(r, sizeof(ExprType));
                memcpy(inst, type, sizeof(ExprType));
                inst->subtrees = paramTypes;
                return inst;
            } else {
                return type;
            }

    }
}

/** debuggging functions **/
int writeToTmp(char *fileName, char *text) {
    char buf[1024];
    strcpy(buf, "/tmp/");
    strcat(buf, fileName);
    FILE *fp = fopen(buf, "a");
    if(fp==NULL) {
        return 0;
    }
    fputs(text, fp);
    fclose(fp);
    return 1;
}
int writeIntToTmp(char *fileName, int text) {
    char buf[1024];
	snprintf(buf, 1024, "%d", text);
	writeToTmp(fileName, buf);
    return 1;
}

void printEnvToStdOut(Env *env) {
	Env *e = env;
    char buffer[1024];
    while(e!=NULL) {
    	if(e!=env)
    		printf("%s\n===========\n", buffer);
    	printHashtable(e->current, buffer);
    	e = e->previous;
    }
/*
    int i;
    for(i=0;i<env->size;i++) {
        struct bucket *b = env->buckets[i];
        while(b!=NULL) {
            printf("%s=%s\n",b->key, TYPENAME((Res *)b->value));
            b=b->next;
        }
    }
*/
}

void printVarTypeEnvToStdOut(Hashtable *env) {
    int i;
    for(i=0;i<env->size;i++) {
        struct bucket *b = env->buckets[i];
        while(b!=NULL) {
            printf("%s=",b->key);
            printType((ExprType *)b->value, NULL/*env*/);
            printf("\n");
            b=b->next;
        }
    }
}


/**
 * replace type var a with type b in varTypes
 */
/*
void replace(Hashtable *varTypes, int a, ExprType *b) {
	int i;
	for(i=0;i<varTypes->size;i++) {
		struct bucket *bucket= varTypes->buckets[i];
		while(bucket!=NULL) {
			ExprType *t = (ExprType *)bucket->value;
			if(t->t==T_VAR && T_VAR_ID(t) == a) {
				bucket -> value = b;
			}
			bucket = bucket->next;
		}
	}
}
*/

Env* globalEnv(Env *env) {
        Env *global = env;
        while(global->previous!=NULL) {
            global = global->previous;
        }
        return global;
}



int appendToByteBufNew(bytesBuf_t *bytesBuf, char *str) {
  int i,j;
  char *tBuf;

  i = strlen(str);
  j = 0;
  if (bytesBuf->buf == NULL) {
    bytesBuf->buf = malloc (i + 1 + MAX_NAME_LEN * 5);
    memset(bytesBuf->buf, 0, i + 1 + MAX_NAME_LEN * 5);
    strcpy((char *)bytesBuf->buf, str);
    bytesBuf->len = i + 1 + MAX_NAME_LEN * 5; /* allocated length */
  }
  else {
    j = strlen((char *)bytesBuf->buf);
    if ((i + j) < bytesBuf->len) {
      strcat((char *)bytesBuf->buf, str);
    }
    else { /* needs more space */
      tBuf = (char *) malloc(j + i + 1 + (MAX_NAME_LEN * 5));
      strcpy(tBuf,(char *)bytesBuf->buf);
      strcat(tBuf,str);
      free (bytesBuf->buf);
      bytesBuf->len = j + i + 1 + (MAX_NAME_LEN * 5);
      bytesBuf->buf = tBuf;
    }
  }
  return 0;
}

void logErrMsg(rError_t *errmsg, rError_t *system) {
    char errbuf[ERR_MSG_LEN * 16];
    errMsgToString(errmsg, errbuf, ERR_MSG_LEN * 16);
#ifdef DEBUG
    writeToTmp("err.log", "begin errlog\n");
    writeToTmp("err.log", errbuf);
    writeToTmp("err.log", "end errlog\n");
#endif
    if(system!=NULL) {
    	rodsLogAndErrorMsg(LOG_ERROR, system,RE_UNKNOWN_ERROR, "%s", errbuf);
    } else {
    	rodsLog (LOG_ERROR, "%s", errbuf);
    }
}

char *errMsgToString(rError_t *errmsg, char *errbuf, int buflen /* = 0 */) {
    errbuf[0] = '\0';
    int p = 0;
    int i;
    int first = 1;
    int restart = 0;
    for(i=errmsg->len-1;i>=0;i--) {
    	if(strcmp(errmsg->errMsg[i]->msg, ERR_MSG_SEP) == 0) {
    		if(first || restart)
    			continue;
    		else {
    			restart = 1;
    			continue;
    		}
    	}
    	if(restart) {
    		snprintf(errbuf+p, buflen-p, "%s\n", ERR_MSG_SEP);
    		p += strlen(errbuf+p);
    	}
        if(!first && !restart) {
            snprintf(errbuf+p, buflen-p, "caused by: %s\n", errmsg->errMsg[i]->msg);
        } else {
            snprintf(errbuf+p, buflen-p, "%s\n", errmsg->errMsg[i]->msg);
            first = 0;
    		restart = 0;

        }
        p += strlen(errbuf+p);
    }
    return errbuf;

}

void *lookupFromEnv(Env *env, char *key) {
    void* val = lookupFromHashTable(env->current, key);
    if(val==NULL && env->previous!=NULL) {
        val = lookupFromEnv(env->previous, key);
    }
    return val;
}

void updateInEnv(Env *env, char *varName, Res *res) {
    Env *defined = env;

    while(defined  != NULL && lookupFromHashTable(defined->current, varName) == NULL) {
        defined  = defined ->previous;
    }
    if(defined != NULL) {
        updateInHashTable(defined->current, varName, res);
    } else {
        insertIntoHashTable(env->current, varName, res);
    }
}

void freeEnvUninterpretedStructs(Env *e) {
    Hashtable *ht = e->current;
    int i;
    for(i=0;i<ht->size;i++) {
        struct bucket *b = ht->buckets[i];
        while(b!=NULL) {
            Res *res = (Res *) b->value;
            if(TYPE(res) == T_IRODS) {
                if(RES_UNINTER_STRUCT(res)!=NULL) {
                    free(RES_UNINTER_STRUCT(res));
                }
                if(RES_UNINTER_BUFFER(res)!=NULL) {
                    free(RES_UNINTER_BUFFER(res));
                }
            }
            b=b->next;
        }
    }
    if(e->previous!=NULL) {
        freeEnvUninterpretedStructs(e->previous);
    }
}
int isPattern(Node *pattern) {

    if(getNodeType(pattern) == N_APPLICATION || getNodeType(pattern) == N_TUPLE) {
        int i;
        for(i=0;i<pattern->degree;i++) {
            if(!isPattern(pattern->subtrees[i]))
                return 0;
        }
        return 1;
    } else if(getNodeType(pattern) == TK_TEXT || getNodeType(pattern) == TK_VAR || getNodeType(pattern) == TK_STRING
   		 || getNodeType(pattern) == TK_BOOL || getNodeType(pattern) == TK_INT || getNodeType(pattern) == TK_DOUBLE) {
        return 1;
    } else {
        return 0;

    }
}

int isRecursive(Node *rule) {
    return invokedIn(rule->subtrees[0]->text, rule->subtrees[1]) ||
        invokedIn(rule->subtrees[0]->text, rule->subtrees[2]) ||
        invokedIn(rule->subtrees[0]->text, rule->subtrees[3]);

}

int invokedIn(char *fn, Node *expr) {
    int i;
    switch(getNodeType(expr)) {
    	case TK_TEXT:
    		if(strcmp(expr->text, fn) == 0) {
                return 1;
            }
    		break;

    	case N_APPLICATION:
        case N_ACTIONS:
        case N_ACTIONS_RECOVERY:
            for(i=0;i<expr->degree;i++) {
                if(invokedIn(fn, expr->subtrees[i])) {
                    return 1;
                }
            }
            break;
        default:
            break;
    }

    return 0;
}
Node *lookupAVUFromMetadata(Node *metadata, char *a) {
	int i;
	for(i=0;i<metadata->degree;i++) {
		if(strcmp(metadata->subtrees[i]->subtrees[0]->text, a) == 0) {
			return metadata->subtrees[i];
		}
	}
	return NULL;

}

int isRuleGenSyntax(char *expr) {
	char *p = expr;
	int mode = 0;
	while(*p != '\0') {
		switch(mode) {
		case 0:
			if(*p=='#' && *(p+1)=='#') {
				return 0;
			} else if(*p=='#') {
				mode = 1;
			} else if(*p=='\'') {
				mode = 2;
			} else if(*p=='\"') {
				mode = 3;
			} else if(*p=='`') {
				mode = 4;
			}
			break;
		case 1: /* comments */
			if(*p=='\n') {
				mode = 0;
			}
			break;
		case 2: /* single qouted string */
			if(*p=='\\') {
				p++;
				if(*p=='\0') {
					break;
				}
			} else if(*p == '\'') {
				mode = 0;
			}
			break;
		case 3: /* double quoted string */
			if(*p=='\\') {
				p++;
				if(*p=='\0') {
					break;
				}
			} else if(*p == '\"') {
				mode = 0;
			}
			break;
		case 4: /* double backquoted string */
		    if(*p == '`' && *(p+1) == '`') {
				p++;
				mode = 0;
			}
			break;
		}
		p++;
	}
	return 1;
}
#define KEY_INSTANCE
#include "key.instance.h"
#include "restruct.templates.h"
#include "end.instance.h"
void keyNode(Node *node, char *keyBuf) {
	/* memset(keyBuf, 0, KEY_SIZE); */
	if(node->degree>0) {
		snprintf(keyBuf, KEY_SIZE, "%p", node);
	} else {
		char *p = keyBuf;
		int len = snprintf(p, KEY_SIZE, "node::%d::%p::%lld::%p::%d::%s::%s::%d::%f::%lld::%p::%p::%p",
				node->option, node->coercionType, node->expr, node->exprType,
				(int)node->nodeType, node->base, node->text, node->ival, node->dval, node->lval, node->func, node->ruleIndexList, node->param
				);
		if(len >= KEY_SIZE) {
			snprintf(keyBuf, KEY_SIZE, "pointer::%p", node);
			return;
		}
	}
}
void keyBuf(unsigned char *buf, int size, char *keyBuf) {
	if(size * 2 + 1 <= KEY_SIZE) {
		int i;
		char *p = keyBuf;
		for(i=0;i<size;i++) {
			*(p++) = 'A' + (buf[i] & (unsigned char) 0xf);
			*(p++) = 'A' + (buf[i] & (unsigned char) 0xf0);
		}
		*(p++) = '\0';
	} else {
		snprintf(keyBuf, KEY_SIZE, "pointer::%p", buf);

	}
}
#undef KEY_INSTANCE

#include "region.to.region.instance.h"
#include "restruct.templates.h"
#include "end.instance.h"

#include "region.to.region2.instance.h"
#include "restruct.templates.h"
#include "end.instance.h"

#include "to.region.instance.h"
#include "restruct.templates.h"
#include "end.instance.h"

#include "to.memory.instance.h"
#include "restruct.templates.h"
#include "end.instance.h"

#ifdef RE_CACHE_CHECK
#include "cache.check.instance.h"
#include "restruct.templates.h"
#include "end.instance.h"
#endif

#ifdef RE_REGION_CHECK
#include "region.check.instance.h"
#include "restruct.templates.h"
#include "end.instance.h"
#endif

